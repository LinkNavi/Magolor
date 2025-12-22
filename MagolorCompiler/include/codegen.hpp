#pragma once
#include "ast.hpp"
#include <string>
#include <sstream>
#include <unordered_set>

class CodeGen {
public:
    std::string generate(const Program& prog);
    
private:
    std::stringstream out;
    int indent = 0;
    std::unordered_set<std::string> usedModules;
    std::unordered_set<std::string> capturedVars;
    
    void emit(const std::string& s);
    void emitLine(const std::string& s);
    void emitIndent();
    
    void genUsing(const UsingDecl& u);
    void genClass(const ClassDecl& cls);
    void genFunction(const FnDecl& fn, const std::string& className = "");
    void genStmt(const StmtPtr& stmt);
    void genExpr(const ExprPtr& expr);
    std::string typeToString(const TypePtr& type);
    void genStdLib();
    void collectCaptures(const std::vector<StmtPtr>& body, const std::vector<Param>& params);
};
