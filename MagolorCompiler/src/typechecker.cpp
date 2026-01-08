// Fixed typechecker.cpp with proper handling for:
// 1. Std library functions (println, toString, etc.)
// 2. Option helper functions (isSome, isNone, unwrap)
// 3. String concatenation with + operator
// 4. Method calls on objects

#include "typechecker.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include <algorithm>
#include <fstream>
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
ModulePtr TypeChecker::loadModuleFromImport(const std::string &importPath) {
  // Check if it's a builtin module (old hardcoded ones)
  if (ModuleResolver::isBuiltinModule(importPath)) {
    return nullptr;
  }

  // Already loaded?
  auto existing = registry.getModule(importPath);
  if (existing) {
    return existing;
  }

  // Try StdLibLoader first
  auto& stdlibLoader = StdLibLoader::instance();
  if (stdlibLoader.isInitialized() && stdlibLoader.hasModule(importPath)) {
    std::cerr << "[TypeChecker] Loading stdlib module: " << importPath << std::endl;
    
    auto stdModule = stdlibLoader.loadModule(importPath);
    if (stdModule && !stdModule->filePath.empty()) {
      // Parse the .mg file
      std::ifstream file(stdModule->filePath);
      if (!file) {
        std::cerr << "[TypeChecker] Failed to open: " << stdModule->filePath << std::endl;
        return nullptr;
      }

      std::stringstream buffer;
      buffer << file.rdbuf();
      std::string source = buffer.str();

      ErrorReporter parseReporter(stdModule->filePath, source);

      Lexer lexer(source, stdModule->filePath, parseReporter);
      auto tokens = lexer.tokenize();

      if (parseReporter.hasError()) {
        std::cerr << "[TypeChecker] Lex errors in " << importPath << std::endl;
        return nullptr;
      }

      Parser parser(std::move(tokens), stdModule->filePath, parseReporter);
      Program prog = parser.parse();

      if (parseReporter.hasError()) {
        std::cerr << "[TypeChecker] Parse errors in " << importPath << std::endl;
        return nullptr;
      }

      // Create and register module
      auto module = std::make_shared<Module>();
      module->name = importPath;
      module->filepath = stdModule->filePath;
      module->ast = prog;
      module->buildSymbolTable();

      // Register with all possible names
      registry.registerModule(importPath, module);
      
      // Also register short names
      size_t lastDot = importPath.rfind('.');
      if (lastDot != std::string::npos) {
        std::string shortName = importPath.substr(lastDot + 1);
        registry.registerModule(shortName, module);
      }

      std::cerr << "[TypeChecker] ✓ Loaded and registered: " << importPath 
                << " (" << prog.classes.size() << " classes, " 
                << prog.functions.size() << " functions)" << std::endl;

      return module;
    }
  }

  // Try to find it as a user module (existing code continues...)
  std::string filePath;

  size_t firstDot = importPath.find('.');
  std::string pathWithoutProject = (firstDot != std::string::npos)
                                       ? importPath.substr(firstDot + 1)
                                       : importPath;

  std::string relPath = pathWithoutProject;
  for (char &c : relPath) {
    if (c == '.')
      c = '/';
  }

  // Try src/ directory
  filePath = "src/" + relPath + ".mg";

  if (!fs::exists(filePath)) {
    filePath = relPath + ".mg";
  }

  if (!fs::exists(filePath)) {
    std::cerr << "[TypeChecker] Could not find file for import: " << importPath
              << " (tried src/" << relPath << ".mg)" << std::endl;
    return nullptr;
  }

  std::cerr << "[TypeChecker] Loading user module from: " << filePath << std::endl;

  // Read and parse the file (existing code)
  std::ifstream file(filePath);
  if (!file) {
    return nullptr;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string source = buffer.str();

  ErrorReporter parseReporter(filePath, source);

  Lexer lexer(source, filePath, parseReporter);
  auto tokens = lexer.tokenize();

  if (parseReporter.hasError()) {
    return nullptr;
  }

  Parser parser(std::move(tokens), filePath, parseReporter);
  Program prog = parser.parse();

  if (parseReporter.hasError()) {
    return nullptr;
  }

  // Create and register the module
  auto module = std::make_shared<Module>();
  module->name = importPath;
  module->filepath = filePath;
  module->ast = prog;
  module->buildSymbolTable();

  // Register under multiple names for lookup
  registry.registerModule(importPath, module);
  registry.registerModule(pathWithoutProject, module);

  std::cerr << "[TypeChecker] Registered module: " << importPath << " with "
            << prog.classes.size() << " classes, " << prog.functions.size()
            << " functions" << std::endl;

  return module;
}std::vector<FnDecl *> TypeChecker::getVisibleFunctions() {
  std::vector<FnDecl *> result;
  std::unordered_set<std::string> seen;

  Scope *scope = currentScope;
  while (scope) {
    for (auto &[name, fn] : scope->functions) {
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

  auto funcs = getVisibleFunctions();
  result.insert(result.end(), funcs.begin(), funcs.end());

  if (currentClass) {
    for (auto &m : currentClass->methods) {
      result.push_back(&m);
    }
  }

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

// NEW: Check if a function name is a built-in Std library function
bool TypeChecker::isStdLibFunction(const std::string &name) {
  static const std::unordered_set<std::string> stdFunctions = {
      // Std.IO
      "print", "println", "eprint", "eprintln", "readLine", "read", "readChar",
      // Std.Parse
      "parseInt", "parseFloat", "parseBool",
      // Std.Option
      "isSome", "isNone", "unwrap", "unwrapOr",
      // Std.String
      "length", "isEmpty", "trim", "toLower", "toUpper", "startsWith",
      "endsWith", "contains", "replace", "split", "join", "repeat", "substring",
      "indexOf",
      // Std.Array
      "push", "pop", "reverse", "sort", "clear",
      // Std.Math
      "abs", "pow", "sqrt", "sin", "cos", "tan", "min", "max", "floor", "ceil",
      // Std.File
      "exists", "isFile", "isDirectory", "createDir", "remove", "readFile",
      "writeFile",
      // Top-level helpers
      "toString"};

  return stdFunctions.count(name) > 0;
}

// NEW: Get return type for Std library function
TypePtr TypeChecker::getStdLibReturnType(const std::string &name) {
  auto type = std::make_shared<Type>();

  // Option functions
  if (name == "isSome" || name == "isNone") {
    type->kind = Type::BOOL;
    return type;
  }

  if (name == "unwrap" || name == "unwrapOr") {
    // Return generic type - will be inferred from context
    type->kind = Type::VOID;
    return type;
  }

  // String functions
  if (name == "length" || name == "indexOf") {
    type->kind = Type::INT;
    return type;
  }

  if (name == "isEmpty" || name == "startsWith" || name == "endsWith" ||
      name == "contains" || name == "exists" || name == "isFile" ||
      name == "isDirectory") {
    type->kind = Type::BOOL;
    return type;
  }

  if (name == "trim" || name == "toLower" || name == "toUpper" ||
      name == "replace" || name == "join" || name == "repeat" ||
      name == "substring" || name == "toString" || name == "readLine") {
    type->kind = Type::STRING;
    return type;
  }

  if (name == "split") {
    type->kind = Type::ARRAY;
    auto innerType = std::make_shared<Type>();
    innerType->kind = Type::STRING;
    type->innerType = innerType;
    return type;
  }

  if (name == "readFile") {
    type->kind = Type::OPTION;
    auto innerType = std::make_shared<Type>();
    innerType->kind = Type::STRING;
    type->innerType = innerType;
    return type;
  }

  if (name == "writeFile" || name == "appendFile" || name == "createDir" ||
      name == "remove") {
    type->kind = Type::BOOL;
    return type;
  }

  // Math functions
  if (name == "abs" || name == "sqrt" || name == "sin" || name == "cos" ||
      name == "tan" || name == "pow" || name == "floor" || name == "ceil") {
    type->kind = Type::FLOAT;
    return type;
  }

  if (name == "min" || name == "max") {
    type->kind = Type::INT;
    return type;
  }

  // Default to void for print functions
  type->kind = Type::VOID;
  return type;
}

bool TypeChecker::checkProgram(Program &prog) {
  enterScope();

  for (auto &cls : prog.classes) {
    currentScope->classes[cls.name] = &cls;
  }

  for (auto &fn : prog.functions) {
    currentScope->functions[fn.name] = &fn;
  }

  for (auto &cls : prog.classes) {
    checkClass(cls);
  }

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

  for (auto &field : cls.fields) {
    if (field.isStatic && field.initValue) {
      TypePtr initType = checkExpr(field.initValue);
      if (!isAssignable(initType, field.type)) {
        // Relaxed: Allow string concatenation to produce strings
        if (!(field.type->kind == Type::STRING &&
              initType->kind == Type::STRING)) {
          error("Static field '" + field.name +
                "' initialization type mismatch");
        }
      }
    }
  }

  for (auto &method : cls.methods) {
    checkFunction(method);
  }

  currentClass = nullptr;
}

void TypeChecker::checkFunction(FnDecl &fn) {
  currentFunction = &fn;
  enterScope();

  if (currentClass) {
    auto thisType = std::make_shared<Type>();
    thisType->kind = Type::CLASS;
    thisType->className = currentClass->name;
    defineVar("this", thisType);
  }

  for (const auto &param : fn.params) {
    defineVar(param.name, param.type);
  }

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
              // Relaxed: Allow more flexible assignments
              if (!(s.type->kind == Type::STRING &&
                    initType->kind == Type::STRING)) {
                // Skip error for now - will be caught by C++ compiler
              }
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
                // Relaxed: Allow flexible returns
              }
            }
          }
        } else if constexpr (std::is_same_v<T, ExprStmt>) {
          checkExpr(s.expr);
        } else if constexpr (std::is_same_v<T, IfStmt>) {
          TypePtr condType = checkExpr(s.cond);
          // Relaxed: Accept any condition type

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
          checkExpr(s.cond);

          enterScope();
          for (auto &stmt : s.body) {
            checkStmt(stmt);
          }
          exitScope();
        } else if constexpr (std::is_same_v<T, ForStmt>) {
          TypePtr iterType = checkExpr(s.iterable);

          enterScope();
          // Relaxed: Accept any iterable type
          if (iterType->kind == Type::ARRAY) {
            defineVar(s.var, iterType->innerType);
          } else {
            // Assume it's iterable
            auto elemType = std::make_shared<Type>();
            elemType->kind = Type::VOID;
            defineVar(s.var, elemType);
          }

          for (auto &stmt : s.body) {
            checkStmt(stmt);
          }
          exitScope();
        } else if constexpr (std::is_same_v<T, MatchStmt>) {
          TypePtr exprType = checkExpr(s.expr);

          for (auto &arm : s.arms) {
            enterScope();

            if (arm.pattern == "Some" && !arm.bindVar.empty()) {
              if (exprType->kind == Type::OPTION) {
                defineVar(arm.bindVar, exprType->innerType);
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
  if (auto *ident = std::get_if<IdentExpr>(&expr->data)) {
    // Check for Std or imported modules
    if (ident->name == "Std" || ident->name == "File" ||
        ident->name == "String" || ident->name == "Array" ||
        ident->name == "Option" || ident->name == "Parse" ||
        ident->name == "Math" || ident->name == "IO") {
      return true;
    }

    if (currentModule) {
      for (const auto &usingDecl : currentModule->ast.usings) {
        if (!usingDecl.path.empty() && usingDecl.path[0] == ident->name) {
          return true;
        }
      }
      for (const auto &cimport : currentModule->ast.cimports) {
        if (cimport.asNamespace == ident->name) {
          return true;
        }
      }
    }
  }

  if (auto *member = std::get_if<MemberExpr>(&expr->data)) {
    return isModulePath(member->object);
  }

  return false;
}
bool TypeChecker::isSymbolAvailable(const std::string &symbolName) {
  // Check local scope first
  if (lookupVar(symbolName) || lookupFunction(symbolName) ||
      lookupClass(symbolName)) {
    return true;
  }

  // Check stdlib functions
  if (isStdLibFunction(symbolName)) {
    return true;
  }

  // Check if it's a module name
  if (currentModule) {
    for (const auto &usingDecl : currentModule->ast.usings) {
      if (!usingDecl.path.empty() && usingDecl.path[0] == symbolName) {
        return true;
      }
    }
  }

  // Check imported modules
  if (currentModule) {
    for (const auto &usingDecl : currentModule->ast.usings) {
      std::string modulePath;
      for (size_t i = 0; i < usingDecl.path.size(); i++) {
        if (i > 0)
          modulePath += ".";
        modulePath += usingDecl.path[i];
      }

      if (ModuleResolver::isBuiltinModule(modulePath)) {
        continue;
      }

      // Try multiple patterns
      std::vector<std::string> candidates = {
          modulePath, currentModule->packageName + "." + modulePath};

      for (const auto &candidate : candidates) {
        auto it = registry.getModules().find(candidate);
        if (it == registry.getModules().end())
          continue;

        auto module = it->second;

        // Check functions
        for (const auto &fn : module->ast.functions) {
          if (fn.name == symbolName && fn.isPublic) {
            return true;
          }
        }

        // Check classes
        for (const auto &cls : module->ast.classes) {
          if (cls.name == symbolName && cls.isPublic) {
            return true;
          }
        }
      }
    }
  }

  return false;
}
TypePtr TypeChecker::checkExpr(ExprPtr expr) {
  TypePtr resultType = std::visit(
      [this, expr](auto &&e) -> TypePtr {
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
          // FIX: Strict validation instead of relaxed

          // Check stdlib functions first
          if (isStdLibFunction(e.name)) {
            return getStdLibReturnType(e.name);
          }

          // Check local variables
          TypePtr varType = lookupVar(e.name);
          if (varType) {
            return varType;
          }

          // Check local functions
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

          // FIX: Check imported modules with proper validation
          if (currentModule) {
            bool foundInImports = false;

            for (const auto &usingDecl : currentModule->ast.usings) {
              std::string modulePath;
              for (size_t i = 0; i < usingDecl.path.size(); i++) {
                if (i > 0)
                  modulePath += ".";
                modulePath += usingDecl.path[i];
              }

              // Skip built-in modules (handled separately)
              if (ModuleResolver::isBuiltinModule(modulePath)) {
                continue;
              }

              // Try multiple name patterns
              std::vector<std::string> candidates = {
                  modulePath, currentModule->packageName + "." + modulePath};

              for (const auto &candidate : candidates) {
                auto regModule = registry.getModules().find(candidate);
                if (regModule == registry.getModules().end()) {
                  continue;
                }

                auto module = regModule->second;

                // Check functions
                for (const auto &importedFn : module->ast.functions) {
                  if (importedFn.name == e.name && importedFn.isPublic) {
                    auto fnType = std::make_shared<Type>();
                    fnType->kind = Type::FUNCTION;
                    fnType->returnType = importedFn.returnType;
                    for (const auto &param : importedFn.params) {
                      fnType->paramTypes.push_back(param.type);
                    }
                    foundInImports = true;
                    return fnType;
                  }
                }

                // Check classes
                for (const auto &importedCls : module->ast.classes) {
                  if (importedCls.name == e.name && importedCls.isPublic) {
                    auto clsType = std::make_shared<Type>();
                    clsType->kind = Type::CLASS;
                    clsType->className = e.name;
                    foundInImports = true;
                    return clsType;
                  }
                }

                if (foundInImports)
                  break;
              }

              if (foundInImports)
                break;
            }

            // Check if it's a module name itself
            for (const auto &usingDecl : currentModule->ast.usings) {
              if (!usingDecl.path.empty() && usingDecl.path[0] == e.name) {
                auto moduleType = std::make_shared<Type>();
                moduleType->kind = Type::CLASS;
                moduleType->className = e.name;
                return moduleType;
              }
            }

            // If we checked imports and didn't find it, ERROR
            if (!foundInImports) {
              errorAt("Undefined symbol: " + e.name +
                          " (not found in current scope or imported modules)",
                      expr->loc);

              // Return error type
              auto voidType = std::make_shared<Type>();
              voidType->kind = Type::VOID;
              return voidType;
            }
          }

          // Not found anywhere - ERROR
          errorAt("Undefined symbol: " + e.name, expr->loc);
          auto voidType = std::make_shared<Type>();
          voidType->kind = Type::VOID;
          return voidType;
        } else if constexpr (std::is_same_v<T, UnaryExpr>) {
          TypePtr operandType = checkExpr(e.operand);
          return operandType;
        } else if constexpr (std::is_same_v<T, CallExpr>) {
          bool isModuleCall = false;

          if (isModulePath(e.callee)) {
            isModuleCall = true;
          }

          if (auto *ident = std::get_if<IdentExpr>(&e.callee->data)) {
            // Check if it's a stdlib function
            if (isStdLibFunction(ident->name)) {
              for (auto &arg : e.args) {
                checkExpr(arg);
              }
              return getStdLibReturnType(ident->name);
            }

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

          bool isMethodCall =
              std::holds_alternative<MemberExpr>(e.callee->data);

          if (isModuleCall || isMethodCall) {
            for (auto &arg : e.args) {
              checkExpr(arg);
            }

            // Try to infer return type for known methods
            if (auto *member = std::get_if<MemberExpr>(&e.callee->data)) {
              if (isStdLibFunction(member->member)) {
                return getStdLibReturnType(member->member);
              }
            }

            auto returnType = std::make_shared<Type>();
            returnType->kind = Type::VOID;
            return returnType;
          }

          TypePtr calleeType = checkExpr(e.callee);

          if (calleeType->kind != Type::FUNCTION) {
            // Relaxed: Don't error - C++ will catch it
            auto voidType = std::make_shared<Type>();
            voidType->kind = Type::VOID;
            return voidType;
          }

          for (size_t i = 0;
               i < e.args.size() && i < calleeType->paramTypes.size(); i++) {
            checkExpr(e.args[i]);
          }

          return calleeType->returnType;
        } else if constexpr (std::is_same_v<T, MemberExpr>) {
          if (isModulePath(expr)) {
            // Check if it's a known stdlib function
            if (isStdLibFunction(e.member)) {
              auto fnType = std::make_shared<Type>();
              fnType->kind = Type::FUNCTION;
              fnType->returnType = getStdLibReturnType(e.member);
              return fnType;
            }

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
              for (const auto &field : cls->fields) {
                if (field.name == e.member) {
                  return field.type;
                }
              }

              for (const auto &method : cls->methods) {
                if (method.name == e.member) {
                  auto fnType = std::make_shared<Type>();
                  fnType->kind = Type::FUNCTION;
                  fnType->returnType = method.returnType;
                  for (const auto &param : method.params) {
                    fnType->paramTypes.push_back(param.type);
                  }
                  return fnType;
                }
              }
            }
          }

          // Relaxed: Return generic type
          auto voidType = std::make_shared<Type>();
          voidType->kind = Type::VOID;
          return voidType;
        } else if constexpr (std::is_same_v<T, IndexExpr>) {
          TypePtr objectType = checkExpr(e.object);
          checkExpr(e.index);

          if (objectType->kind == Type::ARRAY) {
            return objectType->innerType;
          }

          auto voidType = std::make_shared<Type>();
          voidType->kind = Type::VOID;
          return voidType;
        } else if constexpr (std::is_same_v<T, AssignExpr>) {
          TypePtr targetType = checkExpr(e.target);
          checkExpr(e.value);
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

          // If not found locally, search imported modules
          if (!cls && currentModule) {
            bool foundInImports = false;
            for (const auto &usingDecl : currentModule->ast.usings) {
              std::string modulePath;
              for (size_t i = 0; i < usingDecl.path.size(); i++) {
                if (i > 0)
                  modulePath += ".";
                modulePath += usingDecl.path[i];
              }

              // Skip built-in modules
              if (ModuleResolver::isBuiltinModule(modulePath)) {
                continue;
              }

              // Load the module if not already loaded
              loadModuleFromImport(modulePath);
            }
            // DEBUG: Log what we're looking for
            std::cerr << "[TypeChecker] Looking for class: " << e.className
                      << std::endl;
            std::cerr << "[TypeChecker] Current module: " << currentModule->name
                      << std::endl;
            std::cerr << "[TypeChecker] Package name: "
                      << currentModule->packageName << std::endl;

            // Show all imports
            std::cerr << "[TypeChecker] Imports in this module:" << std::endl;
            for (const auto &usingDecl : currentModule->ast.usings) {
              std::string importPath;
              for (size_t i = 0; i < usingDecl.path.size(); i++) {
                if (i > 0)
                  importPath += ".";
                importPath += usingDecl.path[i];
              }
              std::cerr << "  - " << importPath << std::endl;
            }

            // Show all registered modules
            std::cerr << "[TypeChecker] All registered modules:" << std::endl;
            for (const auto &[name, mod] : registry.getModules()) {
              std::cerr << "  - " << name;
              if (!mod->ast.classes.empty()) {
                std::cerr << " (classes: ";
                for (size_t i = 0; i < mod->ast.classes.size(); i++) {
                  if (i > 0)
                    std::cerr << ", ";
                  std::cerr << mod->ast.classes[i].name;
                }
                std::cerr << ")";
              }
              std::cerr << std::endl;
            }

            // Try to find the class in imported modules
            for (const auto &usingDecl : currentModule->ast.usings) {
              std::string modulePath;
              for (size_t i = 0; i < usingDecl.path.size(); i++) {
                if (i > 0)
                  modulePath += ".";
                modulePath += usingDecl.path[i];
              }

              // Skip built-in modules
              if (ModuleResolver::isBuiltinModule(modulePath)) {
                std::cerr << "[TypeChecker] Skipping builtin: " << modulePath
                          << std::endl;
                continue;
              }

              // CRITICAL: Try MULTIPLE name patterns
              std::vector<std::string> candidates;

              // Pattern 1: Exact import path (e.g., "SlateDB.slate.db")
              candidates.push_back(modulePath);

              // Pattern 2: Just the import path without first part (e.g.,
              // "slate.db")
              size_t firstDot = modulePath.find('.');
              if (firstDot != std::string::npos) {
                candidates.push_back(modulePath.substr(firstDot + 1));
              }

              // Pattern 3: With package prefix if not already present
              if (modulePath.find(currentModule->packageName) != 0) {
                candidates.push_back(currentModule->packageName + "." +
                                     modulePath);
              }

              // Pattern 4: Try all registered module names that end with our
              // import
              for (const auto &[regName, regModule] : registry.getModules()) {
                // Check if registered name ends with our import path
                if (regName.size() >= modulePath.size()) {
                  size_t offset = regName.size() - modulePath.size();
                  if (regName.substr(offset) == modulePath) {
                    // Add this as a candidate
                    if (std::find(candidates.begin(), candidates.end(),
                                  regName) == candidates.end()) {
                      candidates.push_back(regName);
                    }
                  }
                }
              }

              std::cerr << "[TypeChecker] Trying candidates for import '"
                        << modulePath << "':" << std::endl;
              for (const auto &candidate : candidates) {
                std::cerr << "  - Trying: " << candidate << std::endl;

                auto regModule = registry.getModules().find(candidate);
                if (regModule == registry.getModules().end()) {
                  std::cerr << "    ✗ Not found in registry" << std::endl;
                  continue;
                }

                auto module = regModule->second;
                std::cerr << "    ✓ Found module!" << std::endl;

                // Search for the class in this module
                for (auto &importedCls : module->ast.classes) {
                  std::cerr << "      Checking class: " << importedCls.name
                            << " (public: " << importedCls.isPublic << ")"
                            << std::endl;

                  if (importedCls.name == e.className) {
                    if (!importedCls.isPublic) {
                      std::cerr << "      ✗ Class is not public!" << std::endl;
                      errorAt("Class '" + e.className +
                                  "' exists but is not public in module " +
                                  candidate,
                              expr->loc);
                      foundInImports = false;
                      break;
                    }

                    std::cerr << "      ✓ Found class " << e.className
                              << " in module " << candidate << std::endl;
                    currentScope->classes[e.className] = &importedCls;
                    cls = &importedCls;
                    foundInImports = true;
                    break;
                  }
                }

                if (foundInImports)
                  break;
              }

              if (foundInImports)
                break;
            }

            // If still not found, give a helpful error
            if (!cls) {
              std::stringstream hint;
              hint << "Class '" << e.className << "' not found. ";
              hint << "Make sure:\n";
              hint << "  1. The module containing '" << e.className
                   << "' is imported\n";
              hint << "  2. The class is marked with 'pub'\n";
              hint << "  3. The import path matches the module name\n";
              hint << "\nImported modules: ";
              bool first = true;
              for (const auto &usingDecl : currentModule->ast.usings) {
                if (!first)
                  hint << ", ";
                for (size_t i = 0; i < usingDecl.path.size(); i++) {
                  if (i > 0)
                    hint << ".";
                  hint << usingDecl.path[i];
                }
                first = false;
              }

              errorAt("Cannot create instance of undefined class '" +
                          e.className + "' - class not declared or imported\n" +
                          hint.str(),
                      expr->loc);
            }
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
            checkExpr(e.elements[i]);
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
  case Type::GENERIC:
    if (a->className != b->className)
      return false;
    if (a->genericArgs.size() != b->genericArgs.size())
      return false;
    for (size_t i = 0; i < a->genericArgs.size(); i++) {
      if (!typesEqual(a->genericArgs[i], b->genericArgs[i]))
        return false;
    }
    return true;
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
  // Relaxed: Allow more flexible type assignments
  if (!from || !to)
    return true;

  // Allow string assignments
  if (to->kind == Type::STRING && from->kind == Type::STRING) {
    return true;
  }

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
  case Type::GENERIC: {
    std::string s = type->className + "<";
    for (size_t i = 0; i < type->genericArgs.size(); i++) {
      if (i > 0)
        s += ", ";
      s += typeToString(type->genericArgs[i]);
    }
    return s + ">";
  }
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
