#pragma once
#include "ast.hpp"
#include "stdlib_loader.hpp"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Verifier.h>
#include <unordered_map>
#include <memory>
#include <string>

class LLVMCodeGen {
public:
    LLVMCodeGen(const std::string& moduleName);
    ~LLVMCodeGen();
    
    // Main generation entry point
    bool generate(const Program& prog);
    
    // Output generation
    bool emitObjectFile(const std::string& filename);
    bool emitLLVMIR(const std::string& filename);
    bool emitAssembly(const std::string& filename);
    
    // Link with runtime and produce executable
    bool linkExecutable(const std::string& objectFile, const std::string& outputFile);
    
    llvm::Module* getModule() { return module.get(); }
    
    // Stdlib integration
    void setStdLibPath(const std::string& path);
    
private:
    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;
    
    // Stdlib loader
    std::string stdlibPath;
    bool stdlibInitialized = false;
    
    // Symbol tables
    std::unordered_map<std::string, llvm::Value*> namedValues;
    std::unordered_map<std::string, llvm::Function*> functions;
    std::unordered_map<std::string, llvm::StructType*> classes;
    std::unordered_map<std::string, llvm::GlobalVariable*> globals;
    
    // Current context
    llvm::Function* currentFunction = nullptr;
    ClassDecl* currentClass = nullptr;
    
    // Type conversion
    llvm::Type* convertType(TypePtr type);
    llvm::FunctionType* convertFunctionType(TypePtr returnType, const std::vector<TypePtr>& paramTypes);
    
    // Declaration generation
    void genClass(const ClassDecl& cls);
    llvm::Function* genFunctionDecl(const FnDecl& fn, const std::string& mangledName = "");
    void genFunction(const FnDecl& fn, const std::string& mangledName = "");
    void genGlobalVar(const std::string& name, TypePtr type, ExprPtr initExpr);
    
    // Statement generation
    void genStmt(const StmtPtr& stmt);
    void genLetStmt(const LetStmt& stmt);
    void genReturnStmt(const ReturnStmt& stmt);
    void genIfStmt(const IfStmt& stmt);
    void genWhileStmt(const WhileStmt& stmt);
    void genForStmt(const ForStmt& stmt);
    void genMatchStmt(const MatchStmt& stmt);
    
    // Expression generation
    llvm::Value* genExpr(const ExprPtr& expr);
    llvm::Value* genIntLit(const IntLitExpr& expr);
    llvm::Value* genFloatLit(const FloatLitExpr& expr);
    llvm::Value* genStringLit(const StringLitExpr& expr);
    llvm::Value* genBoolLit(const BoolLitExpr& expr);
    llvm::Value* genIdent(const IdentExpr& expr);
    llvm::Value* genBinaryExpr(const BinaryExpr& expr);
    llvm::Value* genUnaryExpr(const UnaryExpr& expr);
    llvm::Value* genCallExpr(const CallExpr& expr);
    llvm::Value* genMemberExpr(const MemberExpr& expr);
    llvm::Value* genIndexExpr(const IndexExpr& expr);
    llvm::Value* genAssignExpr(const AssignExpr& expr);
    llvm::Value* genLambdaExpr(const LambdaExpr& expr);
    llvm::Value* genNewExpr(const NewExpr& expr);
    llvm::Value* genArrayExpr(const ArrayExpr& expr);
    llvm::Value* genThisExpr(const ThisExpr& expr);
    
    // Helper functions
    llvm::AllocaInst* createEntryBlockAlloca(llvm::Function* fn, const std::string& varName, llvm::Type* type);
    llvm::Value* createString(const std::string& str);
    std::string mangleName(const std::string& name, const std::string& className = "");
    
    // Runtime function declarations
    void declareRuntimeFunctions();
    llvm::Function* getRuntimeFunction(const std::string& name);
    
    // Standard library function declarations
    void initStdLib();
    void declareStdLibFunctions();
    llvm::Function* getStdLibFunction(const std::string& name);
    bool isStdLibFunction(const std::string& name);
    
    // GC support (simplified reference counting)
    void genIncRef(llvm::Value* ptr);
    void genDecRef(llvm::Value* ptr);
};
