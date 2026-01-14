#ifndef CODEGEN_LLVM_HPP
#define CODEGEN_LLVM_HPP

#include "ast.hpp"
#include <llvm/Analysis/CGSCCPassManager.h>
#include <llvm/Analysis/LoopAnalysisManager.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Target/TargetMachine.h>
#include <memory>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Forward declarations
struct ClassLayout;
struct RuntimeFunctions;

// Represents a variable in scope
struct LLVMVar {
  llvm::AllocaInst *alloca;
  llvm::Type *type;
  TypePtr astType;
  bool isMutable;
};

// Class layout information for LLVM
struct ClassLayout {
  llvm::StructType *structType;
  std::unordered_map<std::string, size_t> fieldIndices;
  std::unordered_map<std::string, TypePtr> fieldTypes;
  std::vector<std::string> fieldOrder;
  std::string parentClass;
  bool hasVTable;
};

// Runtime support function pointers
struct RuntimeFunctions {
  llvm::FunctionCallee printStr;
  llvm::FunctionCallee printlnStr;
  llvm::FunctionCallee printInt;
  llvm::FunctionCallee printlnInt;
  llvm::FunctionCallee printFloat;
  llvm::FunctionCallee printlnFloat;
  llvm::FunctionCallee printBool;
  llvm::FunctionCallee printlnBool;
  llvm::FunctionCallee readLine;
  llvm::FunctionCallee stringConcat;
  llvm::FunctionCallee stringLength;
  llvm::FunctionCallee stringCompare;
  llvm::FunctionCallee stringSubstr;
  llvm::FunctionCallee intToString;
  llvm::FunctionCallee floatToString;
  llvm::FunctionCallee boolToString;
  llvm::FunctionCallee stringToInt;
  llvm::FunctionCallee stringToFloat;
  llvm::FunctionCallee mallocFunc;
  llvm::FunctionCallee freeFunc;
  llvm::FunctionCallee memcpyFunc;
  llvm::FunctionCallee memsetFunc;
  llvm::FunctionCallee powFunc;
  llvm::FunctionCallee sqrtFunc;
  llvm::FunctionCallee sinFunc;
  llvm::FunctionCallee cosFunc;
  llvm::FunctionCallee tanFunc;
  llvm::FunctionCallee logFunc;
  llvm::FunctionCallee expFunc;
  llvm::FunctionCallee floorFunc;
  llvm::FunctionCallee ceilFunc;
  llvm::FunctionCallee fabsFunc;
  llvm::FunctionCallee arrayCreate;
  llvm::FunctionCallee arrayPush;
  llvm::FunctionCallee arrayPop;
  llvm::FunctionCallee arrayGet;
  llvm::FunctionCallee arraySet;
  llvm::FunctionCallee arrayLength;
  llvm::FunctionCallee optionIsSome;
  llvm::FunctionCallee optionIsNone;
  llvm::FunctionCallee optionUnwrap;
  llvm::FunctionCallee throwError;
};

// Main LLVM Code Generator class
class LLVMCodeGen {
public:
  LLVMCodeGen();
  ~LLVMCodeGen();

  // Main entry point - generates LLVM IR from AST
llvm::Module* generate(const Program &prog, const std::string &moduleName);

  // Output the IR as text
  std::string getIRString() const;

  // Write IR to file
  bool writeIRToFile(const std::string &filename) const;

  // Write object code to file
  bool writeObjectFile(const std::string &filename);

  // Verify the generated module
  bool verify() const;

  // Collect link flags from the program
  std::vector<std::string> collectLinkFlags(const Program &prog);

  // Get the LLVM context
  llvm::LLVMContext &getContext() { return *context; }

  // Get the module
  llvm::Module *getModule() { return module.get(); }

private:
  // LLVM Core components
  std::unique_ptr<llvm::LLVMContext> context;
  std::unique_ptr<llvm::Module> module;
  std::unique_ptr<llvm::IRBuilder<>> builder;
void generateStdCryptoFull();
void generateStdNetworkFull();
  // Runtime support
  RuntimeFunctions runtime;

  // Symbol tables
  std::vector<std::unordered_map<std::string, LLVMVar>> scopeStack;
  std::unordered_map<std::string, llvm::Function *> functions;
  std::unordered_map<std::string, ClassLayout> classLayouts;
  std::unordered_set<std::string> usedModules;
  std::unordered_set<std::string> importedNamespaces;

  // Current context
  llvm::Function *currentFunction;
  std::string currentClassName;
  llvm::Value *currentThis;
  llvm::BasicBlock *currentLoopContinue;
  llvm::BasicBlock *currentLoopBreak;

  // String constant cache
  std::unordered_map<std::string, llvm::GlobalVariable *> stringConstants;

  // Temp counter for unique names
  int tempCounter;

  // =========================================================================
  // Initialization
  // =========================================================================
  void initializeTarget();
  void initializeRuntime();
  void declareRuntimeFunctions();
  void generateRuntimeHelpers();

  // =========================================================================
  // Type conversion
  // =========================================================================
  llvm::Type *toLLVMType(const TypePtr &type);
  llvm::Type *getStringType();
  llvm::Type *getArrayType(const TypePtr &elemType);
  llvm::Type *getMapType(const TypePtr &keyType, const TypePtr &valType);
  llvm::Type *getOptionType(const TypePtr &innerType);
  llvm::Type *getClassType(const std::string &className);
  llvm::FunctionType *getFunctionType(const TypePtr &type);

  // =========================================================================
  // Class generation
  // =========================================================================
  void generateClassLayouts(const std::vector<ClassDecl> &classes);
  void generateClassLayout(const ClassDecl &cls);
  void generateClass(const ClassDecl &cls);
  void generateClassMethods(const ClassDecl &cls);
  void generateClassConstructor(const ClassDecl &cls);
  llvm::Value *createClassInstance(const std::string &className,
                                   const std::vector<llvm::Value *> &args);

  // =========================================================================
  // Function generation
  // =========================================================================
  void generateFunctionDeclarations(const std::vector<FnDecl> &funcs);
  void generateFunctionDeclaration(const FnDecl &fn,
                                   const std::string &className = "");
  void generateFunction(const FnDecl &fn, const std::string &className = "");
  void generateMain(const FnDecl &fn);
  llvm::Function *getCurrentFunction() { return currentFunction; }

  // =========================================================================
  // Statement generation
  // =========================================================================
  void genStmt(const StmtPtr &stmt);
  void genLetStmt(const LetStmt &s);
  void genReturnStmt(const ReturnStmt &s);
  void genExprStmt(const ExprStmt &s);
  void genIfStmt(const IfStmt &s);
  void genWhileStmt(const WhileStmt &s);
  void genForStmt(const ForStmt &s);
  void genMatchStmt(const MatchStmt &s);
  void genBlockStmt(const BlockStmt &s);
  void genCppStmt(const CppStmt &s);

  // =========================================================================
  // Expression generation
  // =========================================================================
  llvm::Value *genExpr(const ExprPtr &expr);
  llvm::Value *genIntLit(const IntLitExpr &e);
  llvm::Value *genFloatLit(const FloatLitExpr &e);
  llvm::Value *genStringLit(const StringLitExpr &e);
  llvm::Value *genBoolLit(const BoolLitExpr &e);
  llvm::Value *genIdent(const IdentExpr &e);
  llvm::Value *genBinary(const BinaryExpr &e);
  llvm::Value *genUnary(const UnaryExpr &e);
  llvm::Value *genCall(const CallExpr &e);
  llvm::Value *genMember(const MemberExpr &e);
  llvm::Value *genIndex(const IndexExpr &e);
  llvm::Value *genAssign(const AssignExpr &e);
  llvm::Value *genLambda(const LambdaExpr &e);
  llvm::Value *genNew(const NewExpr &e);
  llvm::Value *genSome(const SomeExpr &e);
  llvm::Value *genNone(const NoneExpr &e);
  llvm::Value *genThis(const ThisExpr &e);
  llvm::Value *genArray(const ArrayExpr &e);
  llvm::Value *genInterpolatedString(const StringLitExpr &e);

  // =========================================================================
  // Binary operation helpers
  // =========================================================================
  llvm::Value *genIntBinary(const std::string &op, llvm::Value *left,
                            llvm::Value *right);
  llvm::Value *genFloatBinary(const std::string &op, llvm::Value *left,
                              llvm::Value *right);
  llvm::Value *genBoolBinary(const std::string &op, llvm::Value *left,
                             llvm::Value *right);
  llvm::Value *genStringBinary(const std::string &op, llvm::Value *left,
                               llvm::Value *right);
  llvm::Value *genPointerBinary(const std::string &op, llvm::Value *left,
                                llvm::Value *right);

  // =========================================================================
  // Standard library integration
  // =========================================================================
  void generateStdLib();
  void generateStdIO();
  void generateStdMath();
  void generateStdString();
  void generateStdArray();
  void generateStdMap();
  void generateStdOption();
  void generateStdFile();
  void generateStdTime();
  void generateStdRandom();
  void generateStdSystem();
  void generateStdNetwork();
  void generateStdCrypto();
  llvm::Value *callStdLibFunction(const std::string &module,
                                  const std::string &func,
                                  const std::vector<llvm::Value *> &args);

  // =========================================================================
  // Helper functions
  // =========================================================================
  llvm::AllocaInst *createEntryBlockAlloca(llvm::Function *fn,
                                           const std::string &name,
                                           llvm::Type *type);
  llvm::Value *createString(const std::string &str);
  llvm::Value *createStringFromValue(llvm::Value *val, const TypePtr &type);
  llvm::Value *loadIfPointer(llvm::Value *val);
  llvm::Value *castValue(llvm::Value *val, llvm::Type *targetType);
  llvm::Constant *getDefaultValue(llvm::Type *type);
  llvm::Constant *getDefaultValue(const TypePtr &type);
  std::string getMangledName(const std::string &className,
                             const std::string &methodName);
  std::string getUniqueName(const std::string &base);

  // =========================================================================
  // Scope management
  // =========================================================================
  void pushScope();
  void popScope();
  void declareVar(const std::string &name, llvm::AllocaInst *alloca,
                  llvm::Type *type, const TypePtr &astType, bool isMut);
  LLVMVar *lookupVar(const std::string &name);
  llvm::Function *lookupFunction(const std::string &name);

  // =========================================================================
  // Error handling
  // =========================================================================
  void emitError(const std::string &msg);
  llvm::Value *emitRuntimeError(const std::string &msg);
};

// Utility class for managing LLVM optimization passes
class LLVMOptimizer {
public:
  LLVMOptimizer(llvm::Module *mod);
  void optimize(int level = 2);

private:
  llvm::Module *module;
};

#endif // CODEGEN_LLVM_HPP
