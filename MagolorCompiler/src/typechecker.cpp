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
    if (!currentScope) return;
    auto old = currentScope;
    currentScope = currentScope->parent;
    delete old;
}

std::vector<FnDecl*> TypeChecker::getVisibleFunctions() {
    std::vector<FnDecl*> result;
    std::unordered_set<std::string> seen;

    Scope* scope = currentScope;
    while (scope) {
        for (auto& [name, fn] : scope->functions) {
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
std::vector<FnDecl*> TypeChecker::getVisibleCallables() {
    std::vector<FnDecl*> result;

    // 1. Free functions
    auto funcs = getVisibleFunctions();
    result.insert(result.end(), funcs.begin(), funcs.end());

    // 2. Methods from current class
    if (currentClass) {
        for (auto& m : currentClass->methods) {
            result.push_back(&m);
        }
    }

    // 3. Static methods from other classes (optional)
    for (auto& [_, cls] : currentScope->classes) {
        for (auto& m : cls->methods) {
            if (m.isStatic && m.isPublic) {
                result.push_back(&m);
            }
        }
    }

    return result;
}
void TypeChecker::defineVar(const std::string& name, TypePtr type) {
    if (currentScope) {
        currentScope->variables[name] = type;
    }
}

TypePtr TypeChecker::lookupVar(const std::string& name) {
    Scope* scope = currentScope;
    while (scope) {
        auto it = scope->variables.find(name);
        if (it != scope->variables.end()) {
            return it->second;
        }
        scope = scope->parent;
    }
    return nullptr;
}

FnDecl* TypeChecker::lookupFunction(const std::string& name) {
    Scope* scope = currentScope;
    while (scope) {
        auto it = scope->functions.find(name);
        if (it != scope->functions.end()) {
            return it->second;
        }
        scope = scope->parent;
    }
    return nullptr;
}

ClassDecl* TypeChecker::lookupClass(const std::string& name) {
    Scope* scope = currentScope;
    while (scope) {
        auto it = scope->classes.find(name);
        if (it != scope->classes.end()) {
            return it->second;
        }
        scope = scope->parent;
    }
    return nullptr;
}

bool TypeChecker::checkProgram(Program& prog) {
    enterScope();
    
    // Register all classes
    for (auto& cls : prog.classes) {
        currentScope->classes[cls.name] = &cls;
    }
    
    // Register all functions
    for (auto& fn : prog.functions) {
        currentScope->functions[fn.name] = &fn;
    }
    
    // Check classes
    for (auto& cls : prog.classes) {
        checkClass(cls);
    }
    
    // Check functions
    for (auto& fn : prog.functions) {
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

void TypeChecker::checkClass(ClassDecl& cls) {
    currentClass = &cls;
    
    // Check fields
    for (auto& field : cls.fields) {
        if (field.isStatic && field.initValue) {
            TypePtr initType = checkExpr(field.initValue);
            if (!isAssignable(initType, field.type)) {
                error("Static field '" + field.name + "' initialization type mismatch");
            }
        }
    }
    
    // Check methods
    for (auto& method : cls.methods) {
        checkFunction(method);
    }
    
    currentClass = nullptr;
}

void TypeChecker::checkFunction(FnDecl& fn) {
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
    for (const auto& param : fn.params) {
        defineVar(param.name, param.type);
    }
    
    // Check body
    for (auto& stmt : fn.body) {
        checkStmt(stmt);
    }
    
    exitScope();
    currentFunction = nullptr;
}

void TypeChecker::checkStmt(StmtPtr stmt) {
    std::visit([this](auto&& s) {
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
        }
        else if constexpr (std::is_same_v<T, ReturnStmt>) {
            if (s.value) {
                TypePtr returnType = checkExpr(s.value);
                if (currentFunction && currentFunction->returnType) {
                    if (!isAssignable(returnType, currentFunction->returnType)) {
                        typeError("return type " + typeToString(currentFunction->returnType),
                                typeToString(returnType));
                    }
                }
            } else if (currentFunction && currentFunction->returnType && 
                      currentFunction->returnType->kind != Type::VOID) {
                error("Non-void function must return a value");
            }
        }
        else if constexpr (std::is_same_v<T, ExprStmt>) {
            checkExpr(s.expr);
        }
        else if constexpr (std::is_same_v<T, IfStmt>) {
            TypePtr condType = checkExpr(s.cond);
            if (!isBoolean(condType)) {
                typeError("bool", typeToString(condType));
            }
            
            enterScope();
            for (auto& stmt : s.thenBody) {
                checkStmt(stmt);
            }
            exitScope();
            
            if (!s.elseBody.empty()) {
                enterScope();
                for (auto& stmt : s.elseBody) {
                    checkStmt(stmt);
                }
                exitScope();
            }
        }
        else if constexpr (std::is_same_v<T, WhileStmt>) {
            TypePtr condType = checkExpr(s.cond);
            if (!isBoolean(condType)) {
                typeError("bool", typeToString(condType));
            }
            
            enterScope();
            for (auto& stmt : s.body) {
                checkStmt(stmt);
            }
            exitScope();
        }
        else if constexpr (std::is_same_v<T, ForStmt>) {
            TypePtr iterType = checkExpr(s.iterable);
            
            if (iterType->kind != Type::ARRAY) {
                error("For loop requires array type, got " + typeToString(iterType));
            } else {
                enterScope();
                defineVar(s.var, iterType->innerType);
                for (auto& stmt : s.body) {
                    checkStmt(stmt);
                }
                exitScope();
            }
        }
        else if constexpr (std::is_same_v<T, MatchStmt>) {
            TypePtr exprType = checkExpr(s.expr);
            
            for (auto& arm : s.arms) {
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
                
                for (auto& stmt : arm.body) {
                    checkStmt(stmt);
                }
                
                exitScope();
            }
        }
        else if constexpr (std::is_same_v<T, BlockStmt>) {
            enterScope();
            for (auto& stmt : s.stmts) {
                checkStmt(stmt);
            }
            exitScope();
        }
        else if constexpr (std::is_same_v<T, CppStmt>) {
            // Can't type check raw C++
        }
    }, stmt->data);
}

TypePtr TypeChecker::checkExpr(ExprPtr expr) {
    TypePtr resultType = std::visit([this](auto&& e) -> TypePtr {
        using T = std::decay_t<decltype(e)>;
        
        if constexpr (std::is_same_v<T, IntLitExpr>) {
            auto type = std::make_shared<Type>();
            type->kind = Type::INT;
            return type;
        }
        else if constexpr (std::is_same_v<T, FloatLitExpr>) {
            auto type = std::make_shared<Type>();
            type->kind = Type::FLOAT;
            return type;
        }
        else if constexpr (std::is_same_v<T, StringLitExpr>) {
            auto type = std::make_shared<Type>();
            type->kind = Type::STRING;
            return type;
        }
        else if constexpr (std::is_same_v<T, BoolLitExpr>) {
            auto type = std::make_shared<Type>();
            type->kind = Type::BOOL;
            return type;
        }
        else if constexpr (std::is_same_v<T, IdentExpr>) {
            TypePtr varType = lookupVar(e.name);
            if (!varType) {
                // Try functions
                FnDecl* fn = lookupFunction(e.name);
                if (fn) {
                    auto fnType = std::make_shared<Type>();
                    fnType->kind = Type::FUNCTION;
                    fnType->returnType = fn->returnType;
                    for (const auto& param : fn->params) {
                        fnType->paramTypes.push_back(param.type);
                    }
                    return fnType;
                }
                error("Undefined variable: " + e.name);
                auto errorType = std::make_shared<Type>();
                errorType->kind = Type::VOID;
                return errorType;
            }
            return varType;
        }
        else if constexpr (std::is_same_v<T, BinaryExpr>) {
            TypePtr leftType = checkExpr(e.left);
            TypePtr rightType = checkExpr(e.right);
            
            // Arithmetic operators
            if (e.op == "+" || e.op == "-" || e.op == "*" || e.op == "/" || e.op == "%") {
                if (!isNumeric(leftType) || !isNumeric(rightType)) {
                    error("Arithmetic operator requires numeric types");
                }
                if (!typesEqual(leftType, rightType)) {
                    error("Binary operator requires same types");
                }
                return leftType;
            }
            // Comparison operators
            else if (e.op == "==" || e.op == "!=" || e.op == "<" || 
                     e.op == ">" || e.op == "<=" || e.op == ">=") {
                auto boolType = std::make_shared<Type>();
                boolType->kind = Type::BOOL;
                return boolType;
            }
            // Logical operators
            else if (e.op == "&&" || e.op == "||") {
                if (!isBoolean(leftType) || !isBoolean(rightType)) {
                    error("Logical operator requires boolean operands");
                }
                return leftType;
            }
            
            return leftType;
        }
        else if constexpr (std::is_same_v<T, UnaryExpr>) {
            TypePtr operandType = checkExpr(e.operand);
            
            if (e.op == "-") {
                if (!isNumeric(operandType)) {
                    error("Unary minus requires numeric type");
                }
                return operandType;
            }
            else if (e.op == "!") {
                if (!isBoolean(operandType)) {
                    error("Logical NOT requires boolean type");
                }
                return operandType;
            }
            
            return operandType;
        }
        else if constexpr (std::is_same_v<T, CallExpr>) {
            TypePtr calleeType = checkExpr(e.callee);
            
            if (calleeType->kind != Type::FUNCTION) {
                error("Cannot call non-function");
                auto voidType = std::make_shared<Type>();
                voidType->kind = Type::VOID;
                return voidType;
            }
            
            if (e.args.size() != calleeType->paramTypes.size()) {
                error("Wrong number of arguments");
            }
            
            for (size_t i = 0; i < e.args.size() && i < calleeType->paramTypes.size(); i++) {
                TypePtr argType = checkExpr(e.args[i]);
                if (!isAssignable(argType, calleeType->paramTypes[i])) {
                    error("Argument type mismatch");
                }
            }
            
            return calleeType->returnType;
        }
        else if constexpr (std::is_same_v<T, MemberExpr>) {
            TypePtr objectType = checkExpr(e.object);
            
            if (objectType->kind == Type::CLASS) {
                ClassDecl* cls = lookupClass(objectType->className);
                if (cls) {
                    // Check fields
                    for (const auto& field : cls->fields) {
                        if (field.name == e.member) {
                            if (!field.isPublic && currentClass != cls) {
                                error("Cannot access private member: " + e.member);
                            }
                            return field.type;
                        }
                    }
                    
                    // Check methods
                    for (const auto& method : cls->methods) {
                        if (method.name == e.member) {
                            if (!method.isPublic && currentClass != cls) {
                                error("Cannot access private method: " + e.member);
                            }
                            auto fnType = std::make_shared<Type>();
                            fnType->kind = Type::FUNCTION;
                            fnType->returnType = method.returnType;
                            for (const auto& param : method.params) {
                                fnType->paramTypes.push_back(param.type);
                            }
                            return fnType;
                        }
                    }
                    
                    error("Member not found: " + e.member);
                }
            }
            
            auto voidType = std::make_shared<Type>();
            voidType->kind = Type::VOID;
            return voidType;
        }
        else if constexpr (std::is_same_v<T, IndexExpr>) {
            TypePtr objectType = checkExpr(e.object);
            TypePtr indexType = checkExpr(e.index);
            
            if (objectType->kind == Type::ARRAY) {
                if (indexType->kind != Type::INT) {
                    error("Array index must be integer");
                }
                return objectType->innerType;
            }
            
            error("Index operator requires array type");
            auto voidType = std::make_shared<Type>();
            voidType->kind = Type::VOID;
            return voidType;
        }
        else if constexpr (std::is_same_v<T, AssignExpr>) {
            TypePtr targetType = checkExpr(e.target);
            TypePtr valueType = checkExpr(e.value);
            
            if (!isAssignable(valueType, targetType)) {
                typeError(typeToString(targetType), typeToString(valueType));
            }
            
            return targetType;
        }
        else if constexpr (std::is_same_v<T, LambdaExpr>) {
            auto fnType = std::make_shared<Type>();
            fnType->kind = Type::FUNCTION;
            fnType->returnType = e.returnType;
            for (const auto& param : e.params) {
                fnType->paramTypes.push_back(param.type);
            }
            return fnType;
        }
        else if constexpr (std::is_same_v<T, NewExpr>) {
            ClassDecl* cls = lookupClass(e.className);
            if (!cls) {
                error("Unknown class: " + e.className);
                auto voidType = std::make_shared<Type>();
                voidType->kind = Type::VOID;
                return voidType;
            }
            
            auto classType = std::make_shared<Type>();
            classType->kind = Type::CLASS;
            classType->className = e.className;
            return classType;
        }
        else if constexpr (std::is_same_v<T, SomeExpr>) {
            TypePtr valueType = checkExpr(e.value);
            auto optionType = std::make_shared<Type>();
            optionType->kind = Type::OPTION;
            optionType->innerType = valueType;
            return optionType;
        }
        else if constexpr (std::is_same_v<T, NoneExpr>) {
            auto optionType = std::make_shared<Type>();
            optionType->kind = Type::OPTION;
            auto voidInner = std::make_shared<Type>();
            voidInner->kind = Type::VOID;
            optionType->innerType = voidInner;
            return optionType;
        }
        else if constexpr (std::is_same_v<T, ThisExpr>) {
            if (currentClass) {
                auto classType = std::make_shared<Type>();
                classType->kind = Type::CLASS;
                classType->className = currentClass->name;
                return classType;
            }
            error("'this' can only be used in class methods");
            auto voidType = std::make_shared<Type>();
            voidType->kind = Type::VOID;
            return voidType;
        }
        else if constexpr (std::is_same_v<T, ArrayExpr>) {
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
                    error("Array elements must have same type");
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
    }, expr->data);
    
    expr->type = resultType;
    return resultType;
}

bool TypeChecker::typesEqual(TypePtr a, TypePtr b) {
    if (!a || !b) return false;
    if (a->kind != b->kind) return false;
    
    switch (a->kind) {
        case Type::CLASS:
            return a->className == b->className;
        case Type::OPTION:
        case Type::ARRAY:
            return typesEqual(a->innerType, b->innerType);
        case Type::FUNCTION:
            if (!typesEqual(a->returnType, b->returnType)) return false;
            if (a->paramTypes.size() != b->paramTypes.size()) return false;
            for (size_t i = 0; i < a->paramTypes.size(); i++) {
                if (!typesEqual(a->paramTypes[i], b->paramTypes[i])) return false;
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
    if (typesEqual(a, b)) return a;
    return nullptr;
}

bool TypeChecker::isNumeric(TypePtr type) {
    return type && (type->kind == Type::INT || type->kind == Type::FLOAT);
}

bool TypeChecker::isBoolean(TypePtr type) {
    return type && type->kind == Type::BOOL;
}

bool TypeChecker::checkMemberAccess(const std::string& className, 
                                   const std::string& memberName,
                                   bool isStatic) {
    (void)className;
    (void)memberName;
    (void)isStatic;
    return true;
}

bool TypeChecker::isVisible(const std::string& modulePath, 
                           const std::string& symbolName) {
    (void)modulePath;
    (void)symbolName;
    return true;
}

void TypeChecker::error(const std::string& msg) {
    SourceLocation loc;
    loc.file = currentModule ? currentModule->filepath : "<unknown>";
    loc.line = 0;
    loc.col = 0;
    loc.length = 0;
    reporter.error(msg, loc);
}

void TypeChecker::typeError(const std::string& expected, const std::string& actual) {
    error("Type error: expected " + expected + ", got " + actual);
}

std::string TypeChecker::typeToString(TypePtr type) {
    if (!type) return "unknown";
    
    switch (type->kind) {
        case Type::INT: return "int";
        case Type::FLOAT: return "float";
        case Type::STRING: return "string";
        case Type::BOOL: return "bool";
        case Type::VOID: return "void";
        case Type::CLASS: return type->className;
        case Type::OPTION: return "Option<" + typeToString(type->innerType) + ">";
        case Type::ARRAY: return "Array<" + typeToString(type->innerType) + ">";
        case Type::FUNCTION: {
            std::string s = "fn(";
            for (size_t i = 0; i < type->paramTypes.size(); i++) {
                if (i > 0) s += ", ";
                s += typeToString(type->paramTypes[i]);
            }
            s += ") -> " + typeToString(type->returnType);
            return s;
        }
    }
    return "unknown";
}
