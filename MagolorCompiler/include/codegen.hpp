#pragma once
#include "ast.hpp"
#include "stdlib.hpp"
#include <string>
#include <sstream>
#include <unordered_set>
#include <unordered_map>

class CodeGen {
public:
    std::string generate(const Program& prog);
    
private:
    std::stringstream out;
    int indent = 0;
    std::unordered_set<std::string> usedModules;
    std::unordered_set<std::string> capturedVars;
    std::unordered_set<std::string> importedNamespaces;
    std::unordered_set<std::string> knownClassNames; // NEW: Track known class names
      std::string paramTypeToString(const TypePtr &type);
    // Track variables for @cpp block sharing
    struct VarInfo {
        std::string type;
        bool isMutable;
    };
    std::unordered_map<std::string, VarInfo> scopeVars;
    
    void emit(const std::string& s);
    void emitLine(const std::string& s);
    void emitIndent();
    
    void genUsing(const UsingDecl& u);
    void genCImports(const std::vector<CImportDecl>& cimports);
    void genClass(const ClassDecl& cls);
    void genFunction(const FnDecl& fn, const std::string& className = "");
    void genStmt(const StmtPtr& stmt);
    void genExpr(const ExprPtr& expr);
    std::string typeToString(const TypePtr& type);
    void genStdLib();
    void collectCaptures(const std::vector<StmtPtr>& body, const std::vector<Param>& params);
    
    // NEW: Helper to check if a name is a class
    bool isClassName(const std::string& name) const;
    
    void enterScope();
    void exitScope();
    void registerVar(const std::string& name, const std::string& type, bool isMut);
};
