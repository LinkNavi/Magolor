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

  // Main entry points
  bool checkProgram(Program &prog);
  bool checkModule(ModulePtr module);
  std::vector<FnDecl *> getVisibleFunctions();
  std::vector<FnDecl *> getVisibleCallables();

  std::string typeToString(TypePtr type);
private:
  ErrorReporter &reporter;
  ModuleRegistry &registry;

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
