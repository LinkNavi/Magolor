#pragma once
#include "ast.hpp"
#include "error.hpp"
#include "module.hpp"
#include <memory>
#include <unordered_map>
#include <unordered_set>

class TypeChecker {
public:
  TypeChecker(ErrorReporter &reporter, ModuleRegistry &registry)
      : reporter(reporter), registry(registry) {}

  ~TypeChecker();
TypePtr checkIdentExpr(const IdentExpr& e) {
        auto resolution = resolveSymbol(e.name);
        
        if (!resolution.found) {
            // Only error if it's not a potential module path
            if (!isPotentialModulePath(e.name)) {
                errorAt("Undefined symbol: " + e.name, SourceLoc{});
            }
            
            // Return void type as fallback
            auto voidType = std::make_shared<Type>();
            voidType->kind = Type::VOID;
            return voidType;
        }
        
        return resolution.type;
    }
    
    bool isPotentialModulePath(const std::string& name) {
        // Check if this looks like a module name
        if (name == "Std" || name == "Array" || name == "Map" || 
            name == "String" || name == "File" || name == "Math" ||
            name == "IO" || name == "Option" || name == "Parse") {
            return true;
        }
        
        // Check if it's in our imported modules
        if (currentModule) {
            for (const auto& imported : currentModule->importedModules) {
                if (imported == name || imported.find(name + ".") == 0) {
                    return true;
                }
            }
        }
        
        return false;
    }
  // Main entry points
  bool checkProgram(Program &prog);
  bool checkModule(ModulePtr module);
  std::vector<FnDecl *> getVisibleFunctions();
  std::vector<FnDecl *> getVisibleCallables();
ModulePtr loadModuleFromImport(const std::string& importPath); 
  std::string typeToString(TypePtr type);
private:
  ErrorReporter &reporter;
  ModuleRegistry &registry;
  bool isSymbolAvailable(const std::string& symbolName);
	  struct SymbolResolutionResult {
        bool found;
        TypePtr type;
        std::string source; // "local", "current_module", module name, or "stdlib"
    };
SymbolResolutionResult resolveSymbol(const std::string& symbolName) {
        SymbolResolutionResult result;
        result.found = false;
        
        // 1. Check local scope (current function/class)
        TypePtr localVar = lookupVar(symbolName);
        if (localVar) {
            result.found = true;
            result.type = localVar;
            result.source = "local";
            return result;
        }
        
        // 2. Check current module's functions/classes
        if (currentModule) {
            for (const auto& fn : currentModule->ast.functions) {
                if (fn.name == symbolName) {
                    result.found = true;
                    result.type = functionDeclToType(&fn);
                    result.source = "current_module";
                    return result;
                }
            }
            
            for (const auto& cls : currentModule->ast.classes) {
                if (cls.name == symbolName) {
                    result.found = true;
                    result.type = classDeclToType(&cls);
                    result.source = "current_module";
                    return result;
                }
            }
        }
        
        // 3. Check imported modules (with proper visibility)
        if (currentModule) {
            for (const auto& importedName : currentModule->importedModules) {
                // Skip built-in modules (handled separately)
                if (ModuleResolver::isBuiltinModule(importedName)) {
                    continue;
                }
                
                auto importedModule = ModuleRegistry::instance().getModule(importedName);
                if (!importedModule) continue;
                
                // Check if symbol is exported from imported module
                if (!importedModule->isSymbolExported(symbolName)) {
                    continue;
                }
                
                // Found in imported module!
                for (const auto& fn : importedModule->ast.functions) {
                    if (fn.name == symbolName && fn.isPublic) {
                        result.found = true;
                        result.type = functionDeclToType(&fn);
                        result.source = importedName;
                        return result;
                    }
                }
                
                for (const auto& cls : importedModule->ast.classes) {
                    if (cls.name == symbolName && cls.isPublic) {
                        result.found = true;
                        result.type = classDeclToType(&cls);
                        result.source = importedName;
                        return result;
                    }
                }
            }
        }
        
        // 4. Check built-in Std library
        if (isStdLibFunction(symbolName)) {
            result.found = true;
            result.type = getStdLibReturnType(symbolName);
            result.source = "stdlib";
            return result;
        }
        
        return result;
    }
    
 
    
    TypePtr functionDeclToType(const FnDecl* fn) {
        auto fnType = std::make_shared<Type>();
        fnType->kind = Type::FUNCTION;
        fnType->returnType = fn->returnType;
        for (const auto& param : fn->params) {
            fnType->paramTypes.push_back(param.type);
        }
        return fnType;
    }
    
    TypePtr classDeclToType(const ClassDecl* cls) {
        auto clsType = std::make_shared<Type>();
        clsType->kind = Type::CLASS;
        clsType->className = cls->name;
        return clsType;
    }
    
  // Scope management
  struct Scope {
    std::unordered_map<std::string, TypePtr> variables;
    std::unordered_map<std::string, FnDecl *> functions;
    std::unordered_map<std::string, ClassDecl *> classes;
    Scope *parent = nullptr;
  };

  Scope *currentScope = nullptr;
  FnDecl *currentFunction = nullptr;
  ClassDecl *currentClass = nullptr;
  ModulePtr currentModule = nullptr;

  // Scope operations
  void enterScope();
  void exitScope();
  void defineVar(const std::string &name, TypePtr type);
  TypePtr lookupVar(const std::string &name);
  FnDecl *lookupFunction(const std::string &name);
  ClassDecl *lookupClass(const std::string &name);

  // NEW: Stdlib function helpers
  bool isStdLibFunction(const std::string &name);
  TypePtr getStdLibReturnType(const std::string &name);

  // Type checking
  TypePtr checkExpr(ExprPtr expr);
  void checkStmt(StmtPtr stmt);
  void checkFunction(FnDecl &fn);
  void checkClass(ClassDecl &cls);

  // Type operations
  bool typesEqual(TypePtr a, TypePtr b);
  bool isAssignable(TypePtr from, TypePtr to);
  TypePtr commonType(TypePtr a, TypePtr b);
  bool isNumeric(TypePtr type);
  bool isBoolean(TypePtr type);

  // Member access
  bool checkMemberAccess(const std::string &className,
                         const std::string &memberName, bool isStatic);

  // Visibility
  bool isVisible(const std::string &modulePath, const std::string &symbolName);

  // Error reporting
  void error(const std::string &msg);
  void errorAt(const std::string &msg, const SourceLoc &loc);
  void typeError(const std::string &expected, const std::string &actual);
  
  // Helper to check if expression is a module path
  bool isModulePath(ExprPtr expr);
};
