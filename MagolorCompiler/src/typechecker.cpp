#include "typechecker.hpp"
#include <algorithm>
#include <sstream>

TypeChecker::~TypeChecker() {
  while (currentScope) {
    exitScope();
  }
}

void TypeChecker::enterScope() {
  auto newScope = new Scope();
  newScope->parent = currentScope;
  currentScope = newScope;
}

void TypeChecker::exitScope() {
  if (!currentScope)
    return;
  auto old = currentScope;
  currentScope = currentScope->parent;
  delete old;
}

std::vector<FnDecl *> TypeChecker::getVisibleFunctions() {
  std::vector<FnDecl *> result;
  std::unordered_set<std::string> seen;

  Scope *scope = currentScope;
  while (scope) {
    for (auto &[name, fn] : scope->functions) {
      // shadowing rules: inner scopes win
      if (!seen.count(name)) {
        result.push_back(fn);
        seen.insert(name);
      }
    }
    scope = scope->parent;
  }

  return result;
}
std::vector<FnDecl *> TypeChecker::getVisibleCallables() {
  std::vector<FnDecl *> result;

  // 1. Free functions
  auto funcs = getVisibleFunctions();
  result.insert(result.end(), funcs.begin(), funcs.end());

  // 2. Methods from current class
  if (currentClass) {
    for (auto &m : currentClass->methods) {
      result.push_back(&m);
    }
  }

  // 3. Static methods from other classes (optional)
  for (auto &[_, cls] : currentScope->classes) {
    for (auto &m : cls->methods) {
      if (m.isStatic && m.isPublic) {
        result.push_back(&m);
      }
    }
  }

  return result;
}
void TypeChecker::defineVar(const std::string &name, TypePtr type) {
  if (currentScope) {
    currentScope->variables[name] = type;
  }
}

TypePtr TypeChecker::lookupVar(const std::string &name) {
  Scope *scope = currentScope;
  while (scope) {
    auto it = scope->variables.find(name);
    if (it != scope->variables.end()) {
      return it->second;
    }
    scope = scope->parent;
  }
  return nullptr;
}

FnDecl *TypeChecker::lookupFunction(const std::string &name) {
  Scope *scope = currentScope;
  while (scope) {
    auto it = scope->functions.find(name);
    if (it != scope->functions.end()) {
      return it->second;
    }
    scope = scope->parent;
  }
  return nullptr;
}

ClassDecl *TypeChecker::lookupClass(const std::string &name) {
  Scope *scope = currentScope;
  while (scope) {
    auto it = scope->classes.find(name);
    if (it != scope->classes.end()) {
      return it->second;
    }
    scope = scope->parent;
  }
  return nullptr;
}

bool TypeChecker::checkProgram(Program &prog) {
  enterScope();

  // Register all classes
  for (auto &cls : prog.classes) {
    currentScope->classes[cls.name] = &cls;
  }

  // Register all functions
  for (auto &fn : prog.functions) {
    currentScope->functions[fn.name] = &fn;
  }

  // Check classes
  for (auto &cls : prog.classes) {
    checkClass(cls);
  }

  // Check functions
  for (auto &fn : prog.functions) {
    checkFunction(fn);
  }

  exitScope();
  return !reporter.hasError();
}

bool TypeChecker::checkModule(ModulePtr module) {
  currentModule = module;
  bool result = checkProgram(module->ast);
  currentModule = nullptr;
  return result;
}

void TypeChecker::checkClass(ClassDecl &cls) {
  currentClass = &cls;

  // Check fields
  for (auto &field : cls.fields) {
    if (field.isStatic && field.initValue) {
      TypePtr initType = checkExpr(field.initValue);
      if (!isAssignable(initType, field.type)) {
        error("Static field '" + field.name + "' initialization type mismatch");
      }
    }
  }

  // Check methods
  for (auto &method : cls.methods) {
    checkFunction(method);
  }

  currentClass = nullptr;
}

void TypeChecker::checkFunction(FnDecl &fn) {
  currentFunction = &fn;
  enterScope();

  // Add 'this' if we're in a class method
  if (currentClass) {
    auto thisType = std::make_shared<Type>();
    thisType->kind = Type::CLASS;
    thisType->className = currentClass->name;
    defineVar("this", thisType);
  }

  // Add parameters
  for (const auto &param : fn.params) {
    defineVar(param.name, param.type);
  }

  // Check body
  for (auto &stmt : fn.body) {
    checkStmt(stmt);
  }

  exitScope();
  currentFunction = nullptr;
}

void TypeChecker::checkStmt(StmtPtr stmt) {
  std::visit(
      [this](auto &&s) {
        using T = std::decay_t<decltype(s)>;

        if constexpr (std::is_same_v<T, LetStmt>) {
          TypePtr initType = checkExpr(s.init);

          if (s.type) {
            if (!isAssignable(initType, s.type)) {
              typeError(typeToString(s.type), typeToString(initType));
            }
            defineVar(s.name, s.type);
          } else {
            defineVar(s.name, initType);
          }
        } else if constexpr (std::is_same_v<T, ReturnStmt>) {
          if (s.value) {
            TypePtr returnType = checkExpr(s.value);
            if (currentFunction && currentFunction->returnType) {
              if (!isAssignable(returnType, currentFunction->returnType)) {
                typeError("return type " +
                              typeToString(currentFunction->returnType),
                          typeToString(returnType));
              }
            }
          } else if (currentFunction && currentFunction->returnType &&
                     currentFunction->returnType->kind != Type::VOID) {
            error("Non-void function must return a value");
          }
        } else if constexpr (std::is_same_v<T, ExprStmt>) {
          checkExpr(s.expr);
        } else if constexpr (std::is_same_v<T, IfStmt>) {
          TypePtr condType = checkExpr(s.cond);
          if (!isBoolean(condType)) {
            typeError("bool", typeToString(condType));
          }

          enterScope();
          for (auto &stmt : s.thenBody) {
            checkStmt(stmt);
          }
          exitScope();

          if (!s.elseBody.empty()) {
            enterScope();
            for (auto &stmt : s.elseBody) {
              checkStmt(stmt);
            }
            exitScope();
          }
        } else if constexpr (std::is_same_v<T, WhileStmt>) {
          TypePtr condType = checkExpr(s.cond);
          if (!isBoolean(condType)) {
            typeError("bool", typeToString(condType));
          }

          enterScope();
          for (auto &stmt : s.body) {
            checkStmt(stmt);
          }
          exitScope();
        } else if constexpr (std::is_same_v<T, ForStmt>) {
          TypePtr iterType = checkExpr(s.iterable);

          if (iterType->kind != Type::ARRAY) {
            error("For loop requires array type, got " +
                  typeToString(iterType));
          } else {
            enterScope();
            defineVar(s.var, iterType->innerType);
            for (auto &stmt : s.body) {
              checkStmt(stmt);
            }
            exitScope();
          }
        } else if constexpr (std::is_same_v<T, MatchStmt>) {
          TypePtr exprType = checkExpr(s.expr);

          for (auto &arm : s.arms) {
            enterScope();

            if (arm.pattern == "Some" && !arm.bindVar.empty()) {
              if (exprType->kind == Type::OPTION) {
                defineVar(arm.bindVar, exprType->innerType);
              } else {
                error("Some pattern requires Option type");
              }
            } else if (arm.pattern == "None") {
              if (exprType->kind != Type::OPTION) {
                error("None pattern requires Option type");
              }
            }

            for (auto &stmt : arm.body) {
              checkStmt(stmt);
            }

            exitScope();
          }
        } else if constexpr (std::is_same_v<T, BlockStmt>) {
          enterScope();
          for (auto &stmt : s.stmts) {
            checkStmt(stmt);
          }
          exitScope();
        } else if constexpr (std::is_same_v<T, CppStmt>) {
          // Can't type check raw C++
        }
      },
      stmt->data);
}
bool TypeChecker::isModulePath(ExprPtr expr) {
  // Check if this is Std, Std.IO, Std.Network, etc.
  if (auto *ident = std::get_if<IdentExpr>(&expr->data)) {
    if (currentModule) {
      // Check using declarations
      for (const auto &usingDecl : currentModule->ast.usings) {
        if (!usingDecl.path.empty() && usingDecl.path[0] == ident->name) {
          return true;
        }
      }
      // Check cimports
      for (const auto &cimport : currentModule->ast.cimports) {
        if (cimport.asNamespace == ident->name) {
          return true;
        }
      }
    }
  }

  // Check if it's a nested member like Std.Network
  if (auto *member = std::get_if<MemberExpr>(&expr->data)) {
    return isModulePath(member->object);
  }

  return false;
}
TypePtr TypeChecker::checkExpr(ExprPtr expr) {
  TypePtr resultType = std::visit(
      [this, expr](auto &&e) -> TypePtr { // Capture expr!
        using T = std::decay_t<decltype(e)>;

        if constexpr (std::is_same_v<T, IntLitExpr>) {
          auto type = std::make_shared<Type>();
          type->kind = Type::INT;
          return type;
        } else if constexpr (std::is_same_v<T, FloatLitExpr>) {
          auto type = std::make_shared<Type>();
          type->kind = Type::FLOAT;
          return type;
        } else if constexpr (std::is_same_v<T, StringLitExpr>) {
          auto type = std::make_shared<Type>();
          type->kind = Type::STRING;
          return type;
        } else if constexpr (std::is_same_v<T, BoolLitExpr>) {
          auto type = std::make_shared<Type>();
          type->kind = Type::BOOL;
          return type;
        } else if constexpr (std::is_same_v<T, IdentExpr>) {
          TypePtr varType = lookupVar(e.name);
          if (!varType) {
            // Check if it's a namespace/module from using or cimport
            if (currentModule) {
              // Check using declarations
              for (const auto &usingDecl : currentModule->ast.usings) {
                if (!usingDecl.path.empty() && usingDecl.path[0] == e.name) {
                  // This is a module/namespace
                  auto moduleType = std::make_shared<Type>();
                  moduleType->kind = Type::CLASS;
                  moduleType->className = e.name;
                  return moduleType;
                }
              }

              // Check cimports
              for (const auto &importedModuleName :
                   currentModule->importedModules) {
                if (ModuleResolver::isBuiltinModule(importedModuleName)) {
                  continue; // Skip built-in modules
                }

                ModulePtr importedModule =
                    ModuleRegistry::instance().getModule(importedModuleName);
                if (importedModule) {
                  // Check functions in imported module
                  for (const auto &fn : importedModule->ast.functions) {
                    if (fn.name == e.name && fn.isPublic) {
                      // Found it! Create function type
                      auto fnType = std::make_shared<Type>();
                      fnType->kind = Type::FUNCTION;
                      fnType->returnType = fn.returnType;
                      for (const auto &param : fn.params) {
                        fnType->paramTypes.push_back(param.type);
                      }
                      return fnType;
                    }
                  }
                }
              }
            } // Try functions
            FnDecl *fn = lookupFunction(e.name);
            if (fn) {
              auto fnType = std::make_shared<Type>();
              fnType->kind = Type::FUNCTION;
              fnType->returnType = fn->returnType;
              for (const auto &param : fn->params) {
                fnType->paramTypes.push_back(param.type);
              }
              return fnType;
            }

            errorAt("Undefined variable: " + e.name, expr->loc);
            auto errorType = std::make_shared<Type>();
            errorType->kind = Type::VOID;
            return errorType;
          }
          return varType;
        } else if constexpr (std::is_same_v<T, BinaryExpr>) {
          TypePtr leftType = checkExpr(e.left);
          TypePtr rightType = checkExpr(e.right);

          // Arithmetic operators
          if (e.op == "+" || e.op == "-" || e.op == "*" || e.op == "/" ||
              e.op == "%") {
            if (!isNumeric(leftType) || !isNumeric(rightType)) {
              errorAt("Arithmetic operator requires numeric types", expr->loc);
            }
            if (!typesEqual(leftType, rightType)) {
              errorAt("Binary operator requires same types", expr->loc);
            }
            return leftType;
          }
          // Comparison operators
          else if (e.op == "==" || e.op == "!=" || e.op == "<" || e.op == ">" ||
                   e.op == "<=" || e.op == ">=") {
            auto boolType = std::make_shared<Type>();
            boolType->kind = Type::BOOL;
            return boolType;
          }
          // Logical operators
          else if (e.op == "&&" || e.op == "||") {
            if (!isBoolean(leftType) || !isBoolean(rightType)) {
              errorAt("Logical operator requires boolean operands", expr->loc);
            }
            return leftType;
          }

          return leftType;
        } else if constexpr (std::is_same_v<T, UnaryExpr>) {
          TypePtr operandType = checkExpr(e.operand);

          if (e.op == "-") {
            if (!isNumeric(operandType)) {
              errorAt("Unary minus requires numeric type", expr->loc);
            }
            return operandType;
          } else if (e.op == "!") {
            if (!isBoolean(operandType)) {
              errorAt("Logical NOT requires boolean type", expr->loc);
            }
            return operandType;
          }

          return operandType;
        } else if constexpr (std::is_same_v<T, CallExpr>) {
          // Check if this is a call to a module/namespace function
          bool isModuleCall = false;

          // Use the helper to check if callee is a module path
          if (isModulePath(e.callee)) {
            isModuleCall = true;
          }

          // Also check for direct cimport function calls (printf, sqrt if
          // imported directly)
          if (auto *ident = std::get_if<IdentExpr>(&e.callee->data)) {
            if (currentModule) {
              for (const auto &cimport : currentModule->ast.cimports) {
                if (std::find(cimport.symbols.begin(), cimport.symbols.end(),
                              ident->name) != cimport.symbols.end()) {
                  isModuleCall = true;
                  break;
                }
              }
            }
          }

          // Skip validation for module calls - they're validated by C++
          // compiler
          // Check if this is a method call (callee is a MemberExpr)
          bool isMethodCall =
              std::holds_alternative<MemberExpr>(e.callee->data);

          // Skip validation for module calls OR method calls - they're
          // validated by C++ compiler
          if (isModuleCall ||
              isMethodCall) { // Still type check the arguments for side effects
            for (auto &arg : e.args) {
              checkExpr(arg);
            }

            auto returnType = std::make_shared<Type>();
            returnType->kind = Type::VOID;
            return returnType;
          }

          TypePtr calleeType = checkExpr(e.callee);

          if (calleeType->kind != Type::FUNCTION) {
            errorAt("Cannot call non-function", expr->loc);
            auto voidType = std::make_shared<Type>();
            voidType->kind = Type::VOID;
            return voidType;
          }

          if (e.args.size() != calleeType->paramTypes.size()) {
            errorAt("Wrong number of arguments", expr->loc);
          }

          for (size_t i = 0;
               i < e.args.size() && i < calleeType->paramTypes.size(); i++) {
            TypePtr argType = checkExpr(e.args[i]);
            if (!isAssignable(argType, calleeType->paramTypes[i])) {
              errorAt("Argument type mismatch", expr->loc);
            }
          }

          return calleeType->returnType;
        } else if constexpr (std::is_same_v<T, MemberExpr>) {
          // Check if this is a module/namespace member access
          if (isModulePath(expr)) {
            // This is accessing a module member (Std.IO,
            // Std.Network.HttpServer, etc.) Return a generic function type -
            // validation happens at C++ compile time
            auto moduleType = std::make_shared<Type>();
            moduleType->kind = Type::FUNCTION;
            moduleType->returnType = std::make_shared<Type>();
            moduleType->returnType->kind = Type::VOID;
            return moduleType;
          }

          TypePtr objectType = checkExpr(e.object);

          if (objectType->kind == Type::CLASS) {
            ClassDecl *cls = lookupClass(objectType->className);
            if (cls) {
              // Check fields
              for (const auto &field : cls->fields) {
                if (field.name == e.member) {
                  if (!field.isPublic && currentClass != cls) {
                    errorAt("Cannot access private member: " + e.member,
                            expr->loc);
                  }
                  return field.type;
                }
              }

              // Check methods
              for (const auto &method : cls->methods) {
                if (method.name == e.member) {
                  if (!method.isPublic && currentClass != cls) {
                    errorAt("Cannot access private method: " + e.member,
                            expr->loc);
                  }
                  auto fnType = std::make_shared<Type>();
                  fnType->kind = Type::FUNCTION;
                  fnType->returnType = method.returnType;
                  for (const auto &param : method.params) {
                    fnType->paramTypes.push_back(param.type);
                  }
                  return fnType;
                }
              }

              errorAt("Member not found: " + e.member, expr->loc);
            }
          }

          auto voidType = std::make_shared<Type>();
          voidType->kind = Type::VOID;
          return voidType;
        } else if constexpr (std::is_same_v<T, IndexExpr>) {
          TypePtr objectType = checkExpr(e.object);
          TypePtr indexType = checkExpr(e.index);

          if (objectType->kind == Type::ARRAY) {
            if (indexType->kind != Type::INT) {
              errorAt("Array index must be integer", expr->loc);
            }
            return objectType->innerType;
          }

          errorAt("Index operator requires array type", expr->loc);
          auto voidType = std::make_shared<Type>();
          voidType->kind = Type::VOID;
          return voidType;
        } else if constexpr (std::is_same_v<T, AssignExpr>) {
          TypePtr targetType = checkExpr(e.target);
          TypePtr valueType = checkExpr(e.value);

          if (!isAssignable(valueType, targetType)) {
            errorAt("Type mismatch in assignment", expr->loc);
          }

          return targetType;
        } else if constexpr (std::is_same_v<T, LambdaExpr>) {
          auto fnType = std::make_shared<Type>();
          fnType->kind = Type::FUNCTION;
          fnType->returnType = e.returnType;
          for (const auto &param : e.params) {
            fnType->paramTypes.push_back(param.type);
          }
          return fnType;
        } else if constexpr (std::is_same_v<T, NewExpr>) {
          ClassDecl *cls = lookupClass(e.className);
          if (!cls) {
            errorAt("Unknown class: " + e.className, expr->loc);
            auto voidType = std::make_shared<Type>();
            voidType->kind = Type::VOID;
            return voidType;
          }

          auto classType = std::make_shared<Type>();
          classType->kind = Type::CLASS;
          classType->className = e.className;
          return classType;
        } else if constexpr (std::is_same_v<T, SomeExpr>) {
          TypePtr valueType = checkExpr(e.value);
          auto optionType = std::make_shared<Type>();
          optionType->kind = Type::OPTION;
          optionType->innerType = valueType;
          return optionType;
        } else if constexpr (std::is_same_v<T, NoneExpr>) {
          auto optionType = std::make_shared<Type>();
          optionType->kind = Type::OPTION;
          auto voidInner = std::make_shared<Type>();
          voidInner->kind = Type::VOID;
          optionType->innerType = voidInner;
          return optionType;
        } else if constexpr (std::is_same_v<T, ThisExpr>) {
          if (currentClass) {
            auto classType = std::make_shared<Type>();
            classType->kind = Type::CLASS;
            classType->className = currentClass->name;
            return classType;
          }
          errorAt("'this' can only be used in class methods", expr->loc);
          auto voidType = std::make_shared<Type>();
          voidType->kind = Type::VOID;
          return voidType;
        } else if constexpr (std::is_same_v<T, ArrayExpr>) {
          if (e.elements.empty()) {
            auto arrayType = std::make_shared<Type>();
            arrayType->kind = Type::ARRAY;
            arrayType->innerType = std::make_shared<Type>();
            arrayType->innerType->kind = Type::VOID;
            return arrayType;
          }

          TypePtr elemType = checkExpr(e.elements[0]);
          for (size_t i = 1; i < e.elements.size(); i++) {
            TypePtr t = checkExpr(e.elements[i]);
            if (!typesEqual(elemType, t)) {
              errorAt("Array elements must have same type", expr->loc);
            }
          }

          auto arrayType = std::make_shared<Type>();
          arrayType->kind = Type::ARRAY;
          arrayType->innerType = elemType;
          return arrayType;
        }

        auto voidType = std::make_shared<Type>();
        voidType->kind = Type::VOID;
        return voidType;
      },
      expr->data);

  expr->type = resultType;
  return resultType;
}
void TypeChecker::errorAt(const std::string &msg, const SourceLoc &loc) {
  SourceLocation srcLoc;
  srcLoc.file = currentModule ? currentModule->filepath : "<unknown>";
  srcLoc.line = loc.line;
  srcLoc.col = loc.col;
  srcLoc.length = loc.length;
  reporter.error(msg, srcLoc);
}
bool TypeChecker::typesEqual(TypePtr a, TypePtr b) {
  if (!a || !b)
    return false;
  if (a->kind != b->kind)
    return false;

  switch (a->kind) {
  case Type::CLASS:
    return a->className == b->className;
  case Type::OPTION:
  case Type::ARRAY:
    return typesEqual(a->innerType, b->innerType);
  case Type::FUNCTION:
    if (!typesEqual(a->returnType, b->returnType))
      return false;
    if (a->paramTypes.size() != b->paramTypes.size())
      return false;
    for (size_t i = 0; i < a->paramTypes.size(); i++) {
      if (!typesEqual(a->paramTypes[i], b->paramTypes[i]))
        return false;
    }
    return true;
  default:
    return true;
  }
}

bool TypeChecker::isAssignable(TypePtr from, TypePtr to) {
  return typesEqual(from, to);
}

TypePtr TypeChecker::commonType(TypePtr a, TypePtr b) {
  if (typesEqual(a, b))
    return a;
  return nullptr;
}

bool TypeChecker::isNumeric(TypePtr type) {
  return type && (type->kind == Type::INT || type->kind == Type::FLOAT);
}

bool TypeChecker::isBoolean(TypePtr type) {
  return type && type->kind == Type::BOOL;
}

bool TypeChecker::checkMemberAccess(const std::string &className,
                                    const std::string &memberName,
                                    bool isStatic) {
  (void)className;
  (void)memberName;
  (void)isStatic;
  return true;
}

bool TypeChecker::isVisible(const std::string &modulePath,
                            const std::string &symbolName) {
  (void)modulePath;
  (void)symbolName;
  return true;
}

void TypeChecker::error(const std::string &msg) {
  SourceLocation loc;
  loc.file = currentModule ? currentModule->filepath : "<unknown>";
  loc.line = 0;
  loc.col = 0;
  loc.length = 0;
  reporter.error(msg, loc);
}

void TypeChecker::typeError(const std::string &expected,
                            const std::string &actual) {
  error("Type error: expected " + expected + ", got " + actual);
}

std::string TypeChecker::typeToString(TypePtr type) {
  if (!type)
    return "unknown";

  switch (type->kind) {
  case Type::INT:
    return "int";
  case Type::FLOAT:
    return "float";
  case Type::STRING:
    return "string";
  case Type::BOOL:
    return "bool";
  case Type::VOID:
    return "void";
  case Type::CLASS:
    return type->className;
  case Type::OPTION:
    return "Option<" + typeToString(type->innerType) + ">";
  case Type::ARRAY:
    return "Array<" + typeToString(type->innerType) + ">";
  case Type::FUNCTION: {
    std::string s = "fn(";
    for (size_t i = 0; i < type->paramTypes.size(); i++) {
      if (i > 0)
        s += ", ";
      s += typeToString(type->paramTypes[i]);
    }
    s += ") -> " + typeToString(type->returnType);
    return s;
  }
  }
  return "unknown";
}
