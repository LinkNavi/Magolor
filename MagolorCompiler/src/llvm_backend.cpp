#include "llvm_backend.hpp"
#include <llvm/IR/Verifier.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <stdexcept>

namespace IR {

LLVMBackend::LLVMBackend() {
    // Initialize LLVM
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
    
    // Create LLVM objects
    context = std::make_unique<llvm::LLVMContext>();
    module = std::make_unique<llvm::Module>("magolor_module", *context);
    builder = std::make_unique<llvm::IRBuilder<>>(*context);
}

LLVMBackend::~LLVMBackend() = default;

std::unique_ptr<llvm::Module> LLVMBackend::generate(const IRModulePtr& irModule) {
    // Set module name
    module->setModuleIdentifier(irModule->name);
    
    // Declare standard library functions (println, etc.)
    declareStdLibFunctions();
    
    // Generate in order: classes, function declarations, function bodies
    generateClasses(irModule);
    generateFunctionDeclarations(irModule);
    generateFunctionBodies(irModule);
    
    // Verify the module
    std::string error;
    llvm::raw_string_ostream errorStream(error);
    if (llvm::verifyModule(*module, &errorStream)) {
        throw std::runtime_error("LLVM module verification failed:\n" + error);
    }
    
    return std::move(module);
}

llvm::Type* LLVMBackend::convertType(const IRTypePtr& type) {
    if (!type) return builder->getVoidTy();
    
    switch (type->kind) {
        case IRTypeKind::Void:
            return builder->getVoidTy();
            
        case IRTypeKind::Bool:
            return builder->getInt1Ty();
            
        case IRTypeKind::Int64:
            return builder->getInt64Ty();
            
        case IRTypeKind::Float64:
            return builder->getDoubleTy();
            
        case IRTypeKind::String:
            // String is represented as i8* (pointer to char array)
            return builder->getInt8PtrTy();
            
        case IRTypeKind::Pointer:
            return llvm::PointerType::get(convertType(type->elementType), 0);
            
        case IRTypeKind::Array:
        case IRTypeKind::Map:
        case IRTypeKind::Set:
            // Complex types represented as opaque pointers (managed by runtime)
            return llvm::PointerType::get(llvm::StructType::get(*context), 0);
            
        case IRTypeKind::Optional:
            // Optional<T> represented as struct { i1 hasValue, T value }
            {
                llvm::Type* innerType = convertType(type->elementType);
                std::vector<llvm::Type*> fields = {builder->getInt1Ty(), innerType};
                return llvm::StructType::get(*context, fields);
            }
            
        case IRTypeKind::Class:
            {
                auto it = classMap.find(type->name);
                if (it != classMap.end()) {
                    return llvm::PointerType::get(it->second, 0);
                }
                // Opaque struct for unknown classes
                return llvm::PointerType::get(llvm::StructType::get(*context), 0);
            }
            
        case IRTypeKind::Function:
            {
                llvm::Type* retType = convertType(type->returnType);
                std::vector<llvm::Type*> paramTypes;
                for (const auto& param : type->paramTypes) {
                    paramTypes.push_back(convertType(param));
                }
                llvm::FunctionType* funcType = llvm::FunctionType::get(retType, paramTypes, false);
                return llvm::PointerType::get(funcType, 0);
            }
            
        case IRTypeKind::Auto:
            return builder->getInt64Ty();  // Default to int64
            
        default:
            return builder->getVoidTy();
    }
}

llvm::FunctionType* LLVMBackend::getFunctionType(const IRFunctionPtr& func) {
    llvm::Type* retType = convertType(func->returnType);
    std::vector<llvm::Type*> paramTypes;
    
    for (const auto& param : func->parameters) {
        paramTypes.push_back(convertType(param.type));
    }
    
    return llvm::FunctionType::get(retType, paramTypes, false);
}

void LLVMBackend::generateClasses(const IRModulePtr& irModule) {
    // Generate struct types for classes
    for (const auto& cls : irModule->classes) {
        generateClass(cls);
    }
}

void LLVMBackend::generateClass(const IRClassPtr& cls) {
    // Create opaque struct type
    llvm::StructType* structType = llvm::StructType::create(*context, cls->name);
    classMap[cls->name] = structType;
    
    // Collect field types
    std::vector<llvm::Type*> fieldTypes;
    for (const auto& field : cls->fields) {
        if (!field.isStatic) {
            fieldTypes.push_back(convertType(field.type));
        }
    }
    
    // Set body
    if (!fieldTypes.empty()) {
        structType->setBody(fieldTypes);
    }
}

void LLVMBackend::generateFunctionDeclarations(const IRModulePtr& irModule) {
    // Generate forward declarations for all functions
    for (const auto& cls : irModule->classes) {
        for (const auto& method : cls->methods) {
            llvm::FunctionType* funcType = getFunctionType(method);
            std::string funcName = cls->name + "_" + method->name;
            
            llvm::Function* llvmFunc = llvm::Function::Create(
                funcType,
                llvm::Function::ExternalLinkage,
                funcName,
                module.get()
            );
            
            functionMap[funcName] = llvmFunc;
        }
    }
    
    for (const auto& func : irModule->functions) {
        llvm::FunctionType* funcType = getFunctionType(func);
        
        llvm::Function* llvmFunc = llvm::Function::Create(
            funcType,
            llvm::Function::ExternalLinkage,
            func->name,
            module.get()
        );
        
        functionMap[func->name] = llvmFunc;
    }
}

void LLVMBackend::generateFunctionBodies(const IRModulePtr& irModule) {
    // Generate function bodies
    for (const auto& cls : irModule->classes) {
        for (const auto& method : cls->methods) {
            generateFunction(method);
        }
    }
    
    for (const auto& func : irModule->functions) {
        generateFunction(func);
    }
}

void LLVMBackend::generateFunction(const IRFunctionPtr& func) {
    // Get or create LLVM function
    std::string funcName = func->className.empty() ? func->name : func->className + "_" + func->name;
    llvm::Function* llvmFunc = functionMap[funcName];
    
    if (!llvmFunc) {
        llvmFunc = llvm::Function::Create(
            getFunctionType(func),
            llvm::Function::ExternalLinkage,
            funcName,
            module.get()
        );
        functionMap[funcName] = llvmFunc;
    }
    
    currentFunction = llvmFunc;
    
    // Clear mappings for new function
    valueMap.clear();
    blockMap.clear();
    
    // Map parameters to LLVM arguments
    size_t paramIdx = 0;
    for (auto& arg : llvmFunc->args()) {
        if (paramIdx < func->parameters.size()) {
            const auto& param = func->parameters[paramIdx];
            arg.setName(param.name);
            if (param.value) {
                valueMap[param.value->id] = &arg;
            }
            paramIdx++;
        }
    }
    
    // Create basic blocks
    for (const auto& block : func->blocks) {
        llvm::BasicBlock* llvmBlock = llvm::BasicBlock::Create(
            *context,
            block->name,
            llvmFunc
        );
        blockMap[block] = llvmBlock;
    }
    
    // Generate instructions in each block
    for (const auto& block : func->blocks) {
        generateBasicBlock(block);
    }
    
    currentFunction = nullptr;
}

void LLVMBackend::generateBasicBlock(const IRBasicBlockPtr& block) {
    llvm::BasicBlock* llvmBlock = getBasicBlock(block);
    builder->SetInsertPoint(llvmBlock);
    
    for (const auto& inst : block->instructions) {
        generateInstruction(inst);
    }
}

void LLVMBackend::generateInstruction(const IRInstructionPtr& inst) {
    switch (inst->opcode) {
        case IROpcode::Alloca:
            genAlloca(inst);
            break;
            
        case IROpcode::Load:
            genLoad(inst);
            break;
            
        case IROpcode::Store:
            genStore(inst);
            break;
            
        case IROpcode::Add:
        case IROpcode::Sub:
        case IROpcode::Mul:
        case IROpcode::Div:
        case IROpcode::Mod:
        case IROpcode::And:
        case IROpcode::Or:
            genBinaryOp(inst);
            break;
            
        case IROpcode::Neg:
        case IROpcode::Not:
            genUnaryOp(inst);
            break;
            
        case IROpcode::Eq:
        case IROpcode::Ne:
        case IROpcode::Lt:
        case IROpcode::Le:
        case IROpcode::Gt:
        case IROpcode::Ge:
            genCompare(inst);
            break;
            
        case IROpcode::Call:
        case IROpcode::CallMethod:
        case IROpcode::CallStatic:
            genCall(inst);
            break;
            
        case IROpcode::Branch:
            genBranch(inst);
            break;
            
        case IROpcode::CondBranch:
            genCondBranch(inst);
            break;
            
        case IROpcode::Return:
            genReturn(inst);
            break;
            
        case IROpcode::ArrayCreate:
        case IROpcode::ArrayGet:
        case IROpcode::ArraySet:
        case IROpcode::ArrayLength:
            genArrayOps(inst);
            break;
            
        case IROpcode::StringConcat:
        case IROpcode::ToString:
            genStringOps(inst);
            break;
            
        case IROpcode::Cast:
            genCast(inst);
            break;
            
        case IROpcode::CppInline:
            // Skip inline C++ - not supported in LLVM backend
            break;
            
        default:
            throw std::runtime_error("Unsupported IR opcode: " + std::to_string((int)inst->opcode));
    }
}

void LLVMBackend::genAlloca(const IRInstructionPtr& inst) {
    llvm::Type* allocType = convertType(inst->type);
    llvm::Value* alloca = builder->CreateAlloca(allocType, nullptr, inst->stringData);
    
    if (inst->result) {
        valueMap[inst->result->id] = alloca;
    }
}

void LLVMBackend::genLoad(const IRInstructionPtr& inst) {
    llvm::Value* ptr = getValue(inst->operands[0]);
    llvm::Type* loadType = convertType(inst->result->type);
    llvm::Value* loaded = builder->CreateLoad(loadType, ptr, "load");
    
    if (inst->result) {
        valueMap[inst->result->id] = loaded;
    }
}

void LLVMBackend::genStore(const IRInstructionPtr& inst) {
    llvm::Value* value = getValue(inst->operands[0]);
    llvm::Value* ptr = getValue(inst->operands[1]);
    builder->CreateStore(value, ptr);
}

void LLVMBackend::genBinaryOp(const IRInstructionPtr& inst) {
    llvm::Value* left = getValue(inst->operands[0]);
    llvm::Value* right = getValue(inst->operands[1]);
    llvm::Value* result = nullptr;
    
    // Determine if we're doing integer or float arithmetic
    bool isFloat = left->getType()->isFloatingPointTy();
    
    switch (inst->opcode) {
        case IROpcode::Add:
            result = isFloat ? builder->CreateFAdd(left, right, "fadd") 
                             : builder->CreateAdd(left, right, "add");
            break;
        case IROpcode::Sub:
            result = isFloat ? builder->CreateFSub(left, right, "fsub")
                             : builder->CreateSub(left, right, "sub");
            break;
        case IROpcode::Mul:
            result = isFloat ? builder->CreateFMul(left, right, "fmul")
                             : builder->CreateMul(left, right, "mul");
            break;
        case IROpcode::Div:
            result = isFloat ? builder->CreateFDiv(left, right, "fdiv")
                             : builder->CreateSDiv(left, right, "div");
            break;
        case IROpcode::Mod:
            result = isFloat ? builder->CreateFRem(left, right, "frem")
                             : builder->CreateSRem(left, right, "mod");
            break;
        case IROpcode::And:
            result = builder->CreateAnd(left, right, "and");
            break;
        case IROpcode::Or:
            result = builder->CreateOr(left, right, "or");
            break;
        default:
            throw std::runtime_error("Unknown binary operation");
    }
    
    if (inst->result && result) {
        valueMap[inst->result->id] = result;
    }
}

void LLVMBackend::genUnaryOp(const IRInstructionPtr& inst) {
    llvm::Value* operand = getValue(inst->operands[0]);
    llvm::Value* result = nullptr;
    
    switch (inst->opcode) {
        case IROpcode::Neg:
            if (operand->getType()->isFloatingPointTy()) {
                result = builder->CreateFNeg(operand, "fneg");
            } else {
                result = builder->CreateNeg(operand, "neg");
            }
            break;
        case IROpcode::Not:
            result = builder->CreateNot(operand, "not");
            break;
        default:
            throw std::runtime_error("Unknown unary operation");
    }
    
    if (inst->result && result) {
        valueMap[inst->result->id] = result;
    }
}

void LLVMBackend::genCompare(const IRInstructionPtr& inst) {
    llvm::Value* left = getValue(inst->operands[0]);
    llvm::Value* right = getValue(inst->operands[1]);
    llvm::Value* result = nullptr;
    
    bool isFloat = left->getType()->isFloatingPointTy();
    
    switch (inst->opcode) {
        case IROpcode::Eq:
            result = isFloat ? builder->CreateFCmpOEQ(left, right, "fcmp_eq")
                             : builder->CreateICmpEQ(left, right, "cmp_eq");
            break;
        case IROpcode::Ne:
            result = isFloat ? builder->CreateFCmpONE(left, right, "fcmp_ne")
                             : builder->CreateICmpNE(left, right, "cmp_ne");
            break;
        case IROpcode::Lt:
            result = isFloat ? builder->CreateFCmpOLT(left, right, "fcmp_lt")
                             : builder->CreateICmpSLT(left, right, "cmp_lt");
            break;
        case IROpcode::Le:
            result = isFloat ? builder->CreateFCmpOLE(left, right, "fcmp_le")
                             : builder->CreateICmpSLE(left, right, "cmp_le");
            break;
        case IROpcode::Gt:
            result = isFloat ? builder->CreateFCmpOGT(left, right, "fcmp_gt")
                             : builder->CreateICmpSGT(left, right, "cmp_gt");
            break;
        case IROpcode::Ge:
            result = isFloat ? builder->CreateFCmpOGE(left, right, "fcmp_ge")
                             : builder->CreateICmpSGE(left, right, "cmp_ge");
            break;
        default:
            throw std::runtime_error("Unknown comparison operation");
    }
    
    if (inst->result && result) {
        valueMap[inst->result->id] = result;
    }
}

void LLVMBackend::genCall(const IRInstructionPtr& inst) {
    llvm::Function* callee = nullptr;
    std::vector<llvm::Value*> args;
    
    // Find the function to call
    if (inst->opcode == IROpcode::CallStatic && !inst->className.empty()) {
        std::string funcName = inst->className + "_" + inst->methodName;
        callee = functionMap[funcName];
    } else {
        callee = functionMap[inst->methodName];
    }
    
    if (!callee) {
        // Try to find as external/stdlib function
        callee = module->getFunction(inst->methodName);
        if (!callee) {
            throw std::runtime_error("Unknown function: " + inst->methodName);
        }
    }
    
    // Get arguments
    for (const auto& operand : inst->operands) {
        args.push_back(getValue(operand));
    }
    
    llvm::Value* result = builder->CreateCall(callee, args, "call");
    
    if (inst->result) {
        valueMap[inst->result->id] = result;
    }
}

void LLVMBackend::genBranch(const IRInstructionPtr& inst) {
    llvm::BasicBlock* target = getBasicBlock(inst->targetBlock);
    builder->CreateBr(target);
}

void LLVMBackend::genCondBranch(const IRInstructionPtr& inst) {
    llvm::Value* cond = getValue(inst->operands[0]);
    llvm::BasicBlock* trueBlock = getBasicBlock(inst->trueBlock);
    llvm::BasicBlock* falseBlock = getBasicBlock(inst->falseBlock);
    
    builder->CreateCondBr(cond, trueBlock, falseBlock);
}

void LLVMBackend::genReturn(const IRInstructionPtr& inst) {
    if (inst->operands.empty()) {
        builder->CreateRetVoid();
    } else {
        llvm::Value* retVal = getValue(inst->operands[0]);
        builder->CreateRet(retVal);
    }
}

void LLVMBackend::genArrayOps(const IRInstructionPtr& inst) {
    // Arrays are managed by runtime - generate calls to runtime functions
    // For now, simplified implementation
    
    switch (inst->opcode) {
        case IROpcode::ArrayCreate: {
            // Call runtime array creation function
            llvm::Function* arrayNew = module->getFunction("_mg_array_new");
            if (!arrayNew) {
                // Declare it
                llvm::FunctionType* ft = llvm::FunctionType::get(
                    builder->getInt8PtrTy(), {}, false
                );
                arrayNew = llvm::Function::Create(
                    ft, llvm::Function::ExternalLinkage, "_mg_array_new", module.get()
                );
            }
            llvm::Value* result = builder->CreateCall(arrayNew, {}, "array");
            if (inst->result) {
                valueMap[inst->result->id] = result;
            }
            break;
        }
        default:
            // Other array operations need runtime support
            break;
    }
}

void LLVMBackend::genStringOps(const IRInstructionPtr& inst) {
    // String operations handled by runtime
    switch (inst->opcode) {
        case IROpcode::StringConcat: {
            llvm::Value* left = getValue(inst->operands[0]);
            llvm::Value* right = getValue(inst->operands[1]);
            
            // Call runtime string concatenation
            llvm::Function* strConcat = module->getFunction("_mg_string_concat");
            if (!strConcat) {
                llvm::FunctionType* ft = llvm::FunctionType::get(
                    builder->getInt8PtrTy(),
                    {builder->getInt8PtrTy(), builder->getInt8PtrTy()},
                    false
                );
                strConcat = llvm::Function::Create(
                    ft, llvm::Function::ExternalLinkage, "_mg_string_concat", module.get()
                );
            }
            
            llvm::Value* result = builder->CreateCall(strConcat, {left, right}, "concat");
            if (inst->result) {
                valueMap[inst->result->id] = result;
            }
            break;
        }
        default:
            break;
    }
}

void LLVMBackend::genCast(const IRInstructionPtr& inst) {
    llvm::Value* value = getValue(inst->operands[0]);
    llvm::Type* targetType = convertType(inst->result->type);
    
    llvm::Value* result = builder->CreateBitCast(value, targetType, "cast");
    if (inst->result) {
        valueMap[inst->result->id] = result;
    }
}

llvm::Value* LLVMBackend::getValue(const IRValuePtr& value) {
    if (!value) {
        throw std::runtime_error("Null IR value");
    }
    
    // Check if it's a constant
    if (value->kind == IRValueKind::Constant) {
        return getConstant(value);
    }
    
    // Look up in value map
    auto it = valueMap.find(value->id);
    if (it != valueMap.end()) {
        return it->second;
    }
    
    // Check if it's a global function
    if (value->kind == IRValueKind::Global) {
        llvm::Function* func = module->getFunction(value->name);
        if (func) return func;
        
        // Try with class prefix
        for (const auto& [name, func] : functionMap) {
            if (name.find(value->name) != std::string::npos) {
                return func;
            }
        }
    }
    
    throw std::runtime_error("Unknown IR value: " + value->name + " (id: " + std::to_string(value->id) + ")");
}

llvm::Value* LLVMBackend::getConstant(const IRValuePtr& value) {
    if (!value->constant) {
        return llvm::ConstantInt::get(builder->getInt64Ty(), 0);
    }
    
    const auto& c = value->constant.value();
    
    if (c.isNone) {
        // Return null/zero for None
        return llvm::ConstantInt::get(builder->getInt1Ty(), 0);
    }
    
    if (std::holds_alternative<int64_t>(c.value)) {
        return llvm::ConstantInt::get(builder->getInt64Ty(), std::get<int64_t>(c.value));
    }
    
    if (std::holds_alternative<double>(c.value)) {
        return llvm::ConstantFP::get(builder->getDoubleTy(), std::get<double>(c.value));
    }
    
    if (std::holds_alternative<bool>(c.value)) {
        return llvm::ConstantInt::get(builder->getInt1Ty(), std::get<bool>(c.value));
    }
    
    if (std::holds_alternative<std::string>(c.value)) {
        // Create global string constant
        std::string str = std::get<std::string>(c.value);
        return builder->CreateGlobalStringPtr(str, "str");
    }
    
    return llvm::ConstantInt::get(builder->getInt64Ty(), 0);
}

llvm::BasicBlock* LLVMBackend::getBasicBlock(const IRBasicBlockPtr& block) {
    auto it = blockMap.find(block);
    if (it != blockMap.end()) {
        return it->second;
    }
    throw std::runtime_error("Unknown basic block: " + block->name);
}

void LLVMBackend::declareStdLibFunctions() {
    // Declare println(i8*) -> void
    {
        llvm::FunctionType* ft = llvm::FunctionType::get(
            builder->getVoidTy(),
            {builder->getInt8PtrTy()},
            false
        );
        llvm::Function::Create(
            ft,
            llvm::Function::ExternalLinkage,
            "println",
            module.get()
        );
    }
    
    // Declare print(i8*) -> void
    {
        llvm::FunctionType* ft = llvm::FunctionType::get(
            builder->getVoidTy(),
            {builder->getInt8PtrTy()},
            false
        );
        llvm::Function::Create(
            ft,
            llvm::Function::ExternalLinkage,
            "print",
            module.get()
        );
    }
}

void LLVMBackend::writeObjectFile(const std::string& filename) {
    setupTargetMachine();
    
    auto targetTriple = llvm::sys::getDefaultTargetTriple();
    module->setTargetTriple(targetTriple);
    
    std::string error;
    const llvm::Target* target = llvm::TargetRegistry::lookupTarget(targetTriple, error);
    
    if (!target) {
        throw std::runtime_error("Failed to lookup target: " + error);
    }
    
    llvm::TargetOptions opt;
    auto relocModel = llvm::Optional<llvm::Reloc::Model>();
    llvm::TargetMachine* targetMachine = target->createTargetMachine(
        targetTriple, "generic", "", opt, relocModel
    );
    
    module->setDataLayout(targetMachine->createDataLayout());
    
    std::error_code EC;
    llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::OF_None);
    
    if (EC) {
        throw std::runtime_error("Failed to open file: " + EC.message());
    }
    
    llvm::legacy::PassManager pass;
    if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, llvm::CGFT_ObjectFile)) {
        throw std::runtime_error("TargetMachine can't emit object file");
    }
    
    pass.run(*module);
    dest.flush();
}

void LLVMBackend::writeBitcode(const std::string& filename) {
    std::error_code EC;
    llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::OF_None);
    
    if (EC) {
        throw std::runtime_error("Failed to open file: " + EC.message());
    }
    
    llvm::WriteBitcodeToFile(*module, dest);
    dest.flush();
}

void LLVMBackend::writeIR(const std::string& filename) {
    std::error_code EC;
    llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::OF_None);
    
    if (EC) {
        throw std::runtime_error("Failed to open file: " + EC.message());
    }
    
    module->print(dest, nullptr);
    dest.flush();
}

std::string LLVMBackend::getIRString() {
    std::string str;
    llvm::raw_string_ostream os(str);
    module->print(os, nullptr);
    return os.str();
}

void LLVMBackend::setupTargetMachine() {
    // Already initialized in constructor
}

} // namespace IR
