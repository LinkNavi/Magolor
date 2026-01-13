#pragma once

#include "ir_types.hpp"
#include "../ast.hpp"
#include <stack>
#include <functional>

namespace IR {

// ============================================================================
// IRBuilder - Constructs IR from AST
// ============================================================================

class IRBuilder {
public:
    IRBuilder();
    
    // Build IR from a complete program
    IRModulePtr build(const Program& prog);
    
    // Current context accessors
    IRModulePtr getModule() const { return module; }
    IRFunctionPtr getCurrentFunction() const { return currentFunction; }
    IRBasicBlockPtr getCurrentBlock() const { return currentBlock; }
    
private:
    // Module being built
    IRModulePtr module;
    
    // Current context
    IRFunctionPtr currentFunction;
    IRBasicBlockPtr currentBlock;
    std::string currentClassName;
    
    // Value ID counter for SSA
    uint64_t nextValueId = 0;
    
    // Symbol tables (scoped)
    struct Scope {
        std::unordered_map<std::string, IRValuePtr> variables;
    };
    std::vector<Scope> scopes;
    
    // Known class names for static method resolution
    std::unordered_set<std::string> knownClassNames;
    
    // Imported namespaces
    std::unordered_set<std::string> importedNamespaces;
    
    // Break/continue targets for loops
    struct LoopContext {
        IRBasicBlockPtr breakBlock;
        IRBasicBlockPtr continueBlock;
    };
    std::stack<LoopContext> loopStack;
    
    // ========================================================================
    // Type conversion
    // ========================================================================
    IRTypePtr convertType(const TypePtr& astType);
    
    // ========================================================================
    // Value creation
    // ========================================================================
    IRValuePtr createTemp(IRTypePtr type);
    IRValuePtr createConst(int64_t val);
    IRValuePtr createConst(double val);
    IRValuePtr createConst(bool val);
    IRValuePtr createConst(const std::string& val);
    IRValuePtr createNone(IRTypePtr optType = nullptr);
    
    // ========================================================================
    // Instruction emission
    // ========================================================================
    IRValuePtr emit(IRInstructionPtr inst);
    void emitBranch(IRBasicBlockPtr target);
    void emitCondBranch(IRValuePtr cond, IRBasicBlockPtr trueBlock, IRBasicBlockPtr falseBlock);
    void emitReturn(IRValuePtr value = nullptr);
    
    // ========================================================================
    // Block management
    // ========================================================================
    IRBasicBlockPtr createBlock(const std::string& name);
    void setInsertPoint(IRBasicBlockPtr block);
    void linkBlocks(IRBasicBlockPtr from, IRBasicBlockPtr to);
    
    // ========================================================================
    // Scope management
    // ========================================================================
    void pushScope();
    void popScope();
    void declareVariable(const std::string& name, IRValuePtr value);
    IRValuePtr lookupVariable(const std::string& name);
    
    // ========================================================================
    // Top-level builders
    // ========================================================================
    void buildUsings(const std::vector<UsingDecl>& usings);
    void buildCImports(const std::vector<CImportDecl>& cimports);
    void buildCppHeaders(const std::vector<CppHeaderDecl>& headers);
    void buildLinkDecls(const std::vector<LinkDecl>& links);
    void buildIncludeDecls(const std::vector<IncludeDecl>& includes);
    IRClassPtr buildClass(const ClassDecl& cls);
    IRFunctionPtr buildFunction(const FnDecl& fn, const std::string& className = "");
    
    // ========================================================================
    // Statement builders
    // ========================================================================
    void buildStmt(const StmtPtr& stmt);
    void buildLetStmt(const LetStmt& let);
    void buildReturnStmt(const ReturnStmt& ret);
    void buildExprStmt(const ExprStmt& expr);
    void buildIfStmt(const IfStmt& ifStmt);
    void buildWhileStmt(const WhileStmt& whileStmt);
    void buildForStmt(const ForStmt& forStmt);
    void buildMatchStmt(const MatchStmt& match);
    void buildBlockStmt(const BlockStmt& block);
    void buildCppStmt(const CppStmt& cpp);
    
    // ========================================================================
    // Expression builders
    // ========================================================================
    IRValuePtr buildExpr(const ExprPtr& expr);
    IRValuePtr buildIntLit(const IntLitExpr& lit);
    IRValuePtr buildFloatLit(const FloatLitExpr& lit);
    IRValuePtr buildStringLit(const StringLitExpr& lit);
    IRValuePtr buildBoolLit(const BoolLitExpr& lit);
    IRValuePtr buildIdent(const IdentExpr& ident);
    IRValuePtr buildBinary(const BinaryExpr& binary);
    IRValuePtr buildUnary(const UnaryExpr& unary);
    IRValuePtr buildCall(const CallExpr& call);
    IRValuePtr buildMember(const MemberExpr& member);
    IRValuePtr buildIndex(const IndexExpr& index);
    IRValuePtr buildAssign(const AssignExpr& assign);
    IRValuePtr buildLambda(const LambdaExpr& lambda);
    IRValuePtr buildNew(const NewExpr& newExpr);
    IRValuePtr buildSome(const SomeExpr& some);
    IRValuePtr buildNone(const NoneExpr& none);
    IRValuePtr buildThis(const ThisExpr& thisExpr);
    IRValuePtr buildArray(const ArrayExpr& array);
    
    // ========================================================================
    // Helper methods
    // ========================================================================
    bool isClassName(const std::string& name) const;
    bool isStdModule(const std::string& name) const;
    bool isNamespacePath(const ExprPtr& expr) const;
    std::vector<std::string> extractNamespacePath(const ExprPtr& expr);
    IRValuePtr buildInterpolatedString(const std::string& str);
    
    // Convert binary operator to IR opcode
    IROpcode binaryOpToOpcode(const std::string& op, IRTypePtr leftType);
};

} // namespace IR
