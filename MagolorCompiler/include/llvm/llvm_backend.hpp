#pragma once

#include "../ir_types.hpp"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <memory>
#include <unordered_map>
#include <string>

namespace IR {

class LLVMBackend {
public:
    LLVMBackend();
    ~LLVMBackend();
    
    // Main entry point - generate LLVM Module from IR Module
    std::unique_ptr<llvm::Module> generate(const IRModulePtr& irModule);
    
    // Output methods
    void writeObjectFile(const std::string& filename);
    void writeBitcode(const std::string& filename);
    void writeIR(const std::string& filename);
    std::string getIRString();
    
    // Get the generated module (for inspection)
    llvm::Module* getModule() { return module.get(); }
    
private:
    // LLVM core objects
    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;
    
    // Mapping tables
    std::unordered_map<uint64_t, llvm::Value*> valueMap;  // IR Value ID -> LLVM Value
    std::unordered_map<std::string, llvm::Function*> functionMap;  // Function name -> LLVM Function
    std::unordered_map<std::string, llvm::StructType*> classMap;  // Class name -> LLVM Struct
    std::unordered_map<IRBasicBlockPtr, llvm::BasicBlock*> blockMap;  // IR Block -> LLVM Block
    
    // Current generation context
    llvm::Function* currentFunction = nullptr;
    
    // Type conversion
    llvm::Type* convertType(const IRTypePtr& type);
    llvm::FunctionType* getFunctionType(const IRFunctionPtr& func);
    
    // Top-level generation
    void generateClasses(const IRModulePtr& irModule);
    void generateFunctionDeclarations(const IRModulePtr& irModule);
    void generateFunctionBodies(const IRModulePtr& irModule);
    void generateClass(const IRClassPtr& cls);
    void generateFunction(const IRFunctionPtr& func);
    
    // Block and instruction generation
    void generateBasicBlock(const IRBasicBlockPtr& block);
    void generateInstruction(const IRInstructionPtr& inst);
    
    // Value generation
    llvm::Value* getValue(const IRValuePtr& value);
    llvm::Value* getConstant(const IRValuePtr& value);
    
    // Instruction handlers
    void genAlloca(const IRInstructionPtr& inst);
    void genLoad(const IRInstructionPtr& inst);
    void genStore(const IRInstructionPtr& inst);
    void genBinaryOp(const IRInstructionPtr& inst);
    void genUnaryOp(const IRInstructionPtr& inst);
    void genCompare(const IRInstructionPtr& inst);
    void genCall(const IRInstructionPtr& inst);
    void genBranch(const IRInstructionPtr& inst);
    void genCondBranch(const IRInstructionPtr& inst);
    void genReturn(const IRInstructionPtr& inst);
    void genArrayOps(const IRInstructionPtr& inst);
    void genStringOps(const IRInstructionPtr& inst);
    void genCast(const IRInstructionPtr& inst);
    
    // Helper methods
    llvm::BasicBlock* getBasicBlock(const IRBasicBlockPtr& block);
    void declareStdLibFunctions();
    void setupTargetMachine();
};

} // namespace IR
