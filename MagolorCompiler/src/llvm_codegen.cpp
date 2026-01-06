#include "llvm_codegen.hpp"
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/IR/LegacyPassManager.h>
#include <variant>
#include <iostream>

LLVMCodeGen::LLVMCodeGen(const std::string& moduleName) {
    context = std::make_unique<llvm::LLVMContext>();
    module = std::make_unique<llvm::Module>(moduleName, *context);
    builder = std::make_unique<llvm::IRBuilder<>>(*context);
    
    // Initialize LLVM targets
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
    
    // Declare runtime and stdlib functions
    declareRuntimeFunctions();
    declareStdLibFunctions();
}

LLVMCodeGen::~LLVMCodeGen() = default;

llvm::Type* LLVMCodeGen::convertType(TypePtr type) {
    if (!type) return llvm::Type::getVoidTy(*context);
    
    switch (type->kind) {
        case Type::INT:
            return llvm::Type::getInt32Ty(*context);
        case Type::FLOAT:
            return llvm::Type::getDoubleTy(*context);
        case Type::BOOL:
            return llvm::Type::getInt1Ty(*context);
        case Type::STRING:
            // String is represented as { i32, i8* } (length + data pointer)
            return llvm::StructType::get(
                llvm::Type::getInt32Ty(*context),
                llvm::Type::getInt8PtrTy(*context)
            );
        case Type::VOID:
            return llvm::Type::getVoidTy(*context);
        case Type::CLASS:
            if (classes.count(type->className)) {
                return llvm::PointerType::get(classes[type->className], 0);
            }
            // Forward reference - create opaque struct
            return llvm::PointerType::get(
                llvm::StructType::create(*context, type->className), 0
            );
        case Type::ARRAY:
            // Array is { i32, T* } (length + data pointer)
            return llvm::StructType::get(
                llvm::Type::getInt32Ty(*context),
                llvm::PointerType::get(convertType(type->innerType), 0)
            );
        case Type::OPTION:
            // Option is { i1, T } (has_value flag + value)
            return llvm::StructType::get(
                llvm::Type::getInt1Ty(*context),
                convertType(type->innerType)
            );
        case Type::FUNCTION: {
            auto retType = convertType(type->returnType);
            std::vector<llvm::Type*> paramTypes;
            for (const auto& pt : type->paramTypes) {
                paramTypes.push_back(convertType(pt));
            }
            return llvm::PointerType::get(
                llvm::FunctionType::get(retType, paramTypes, false), 0
            );
        }
        case Type::GENERIC:
            // For now, treat generics as opaque pointers
            return llvm::Type::getInt8PtrTy(*context);
    }
    
    return llvm::Type::getVoidTy(*context);
}

llvm::FunctionType* LLVMCodeGen::convertFunctionType(TypePtr returnType, const std::vector<TypePtr>& paramTypes) {
    std::vector<llvm::Type*> llvmParamTypes;
    for (const auto& pt : paramTypes) {
        llvmParamTypes.push_back(convertType(pt));
    }
    return llvm::FunctionType::get(convertType(returnType), llvmParamTypes, false);
}

bool LLVMCodeGen::generate(const Program& prog) {
    // First pass: declare all classes and functions
    for (const auto& cls : prog.classes) {
        // Create struct type for class
        std::vector<llvm::Type*> fieldTypes;
        for (const auto& field : cls.fields) {
            if (!field.isStatic) {
                fieldTypes.push_back(convertType(field.type));
            }
        }
        
        auto structType = llvm::StructType::create(*context, fieldTypes, cls.name);
        classes[cls.name] = structType;
    }
    
    for (const auto& fn : prog.functions) {
        genFunctionDecl(fn);
    }
    
    // Second pass: generate class methods
    for (const auto& cls : prog.classes) {
        genClass(cls);
    }
    
    // Third pass: generate function bodies
    for (const auto& fn : prog.functions) {
        genFunction(fn);
    }
    
    // Verify module
    std::string error;
    llvm::raw_string_ostream errorStream(error);
    if (llvm::verifyModule(*module, &errorStream)) {
        std::cerr << "Module verification failed:\n" << error << std::endl;
        return false;
    }
    
    return true;
}

void LLVMCodeGen::genClass(const ClassDecl& cls) {
    currentClass = const_cast<ClassDecl*>(&cls);
    
    // Generate static fields as globals
    for (const auto& field : cls.fields) {
        if (field.isStatic) {
            genGlobalVar(mangleName(field.name, cls.name), field.type, field.initValue);
        }
    }
    
    // Generate methods
    for (const auto& method : cls.methods) {
        genFunction(method, mangleName(method.name, cls.name));
    }
    
    currentClass = nullptr;
}

llvm::Function* LLVMCodeGen::genFunctionDecl(const FnDecl& fn, const std::string& mangledName) {
    std::string funcName = mangledName.empty() ? fn.name : mangledName;
    
    // Check if already declared
    if (functions.count(funcName)) {
        return functions[funcName];
    }
    
    std::vector<TypePtr> paramTypes;
    for (const auto& param : fn.params) {
        paramTypes.push_back(param.type);
    }
    
    auto funcType = convertFunctionType(fn.returnType, paramTypes);
    auto func = llvm::Function::Create(
        funcType,
        llvm::Function::ExternalLinkage,
        funcName,
        module.get()
    );
    
    // Set parameter names
    size_t idx = 0;
    for (auto& arg : func->args()) {
        arg.setName(fn.params[idx++].name);
    }
    
    functions[funcName] = func;
    return func;
}

void LLVMCodeGen::genFunction(const FnDecl& fn, const std::string& mangledName) {
    std::string funcName = mangledName.empty() ? fn.name : mangledName;
    auto func = functions[funcName];
    
    if (!func) {
        func = genFunctionDecl(fn, mangledName);
    }
    
    // Create entry basic block
    auto entryBB = llvm::BasicBlock::Create(*context, "entry", func);
    builder->SetInsertPoint(entryBB);
    
    currentFunction = func;
    namedValues.clear();
    
    // Allocate space for parameters
    for (auto& arg : func->args()) {
        auto alloca = createEntryBlockAlloca(func, std::string(arg.getName()), arg.getType());
        builder->CreateStore(&arg, alloca);
        namedValues[std::string(arg.getName())] = alloca;
    }
    
    // Generate function body
    for (const auto& stmt : fn.body) {
        genStmt(stmt);
    }
    
    // Add return if missing
    if (!builder->GetInsertBlock()->getTerminator()) {
        if (fn.returnType->kind == Type::VOID) {
            builder->CreateRetVoid();
        } else {
            // Return zero value
            builder->CreateRet(llvm::Constant::getNullValue(convertType(fn.returnType)));
        }
    }
    
    currentFunction = nullptr;
}

void LLVMCodeGen::genStmt(const StmtPtr& stmt) {
    std::visit([this](auto&& s) {
        using T = std::decay_t<decltype(s)>;
        
        if constexpr (std::is_same_v<T, LetStmt>) {
            genLetStmt(s);
        } else if constexpr (std::is_same_v<T, ReturnStmt>) {
            genReturnStmt(s);
        } else if constexpr (std::is_same_v<T, ExprStmt>) {
            genExpr(s.expr);
        } else if constexpr (std::is_same_v<T, IfStmt>) {
            genIfStmt(s);
        } else if constexpr (std::is_same_v<T, WhileStmt>) {
            genWhileStmt(s);
        } else if constexpr (std::is_same_v<T, ForStmt>) {
            genForStmt(s);
        } else if constexpr (std::is_same_v<T, MatchStmt>) {
            genMatchStmt(s);
        }
    }, stmt->data);
}

void LLVMCodeGen::genLetStmt(const LetStmt& stmt) {
    auto initVal = genExpr(stmt.init);
    auto alloca = createEntryBlockAlloca(currentFunction, stmt.name, initVal->getType());
    builder->CreateStore(initVal, alloca);
    namedValues[stmt.name] = alloca;
}

void LLVMCodeGen::genReturnStmt(const ReturnStmt& stmt) {
    if (stmt.value) {
        auto retVal = genExpr(stmt.value);
        builder->CreateRet(retVal);
    } else {
        builder->CreateRetVoid();
    }
}

void LLVMCodeGen::genIfStmt(const IfStmt& stmt) {
    auto condVal = genExpr(stmt.cond);
    
    // Convert condition to bool if needed
    if (condVal->getType() != llvm::Type::getInt1Ty(*context)) {
        condVal = builder->CreateICmpNE(
            condVal,
            llvm::Constant::getNullValue(condVal->getType()),
            "ifcond"
        );
    }
    
    auto thenBB = llvm::BasicBlock::Create(*context, "then", currentFunction);
    auto elseBB = llvm::BasicBlock::Create(*context, "else");
    auto mergeBB = llvm::BasicBlock::Create(*context, "ifcont");
    
    builder->CreateCondBr(condVal, thenBB, elseBB);
    
    // Then block
    builder->SetInsertPoint(thenBB);
    for (const auto& s : stmt.thenBody) {
        genStmt(s);
    }
    if (!builder->GetInsertBlock()->getTerminator()) {
        builder->CreateBr(mergeBB);
    }
    
    // Else block
    currentFunction->insert(currentFunction->end(), elseBB);
    builder->SetInsertPoint(elseBB);
    if (!stmt.elseBody.empty()) {
        for (const auto& s : stmt.elseBody) {
            genStmt(s);
        }
    }
    if (!builder->GetInsertBlock()->getTerminator()) {
        builder->CreateBr(mergeBB);
    }
    
    // Merge block
    currentFunction->insert(currentFunction->end(), mergeBB);
    builder->SetInsertPoint(mergeBB);
}

void LLVMCodeGen::genWhileStmt(const WhileStmt& stmt) {
    auto condBB = llvm::BasicBlock::Create(*context, "whilecond", currentFunction);
    auto loopBB = llvm::BasicBlock::Create(*context, "whileloop");
    auto afterBB = llvm::BasicBlock::Create(*context, "afterloop");
    
    builder->CreateBr(condBB);
    builder->SetInsertPoint(condBB);
    
    auto condVal = genExpr(stmt.cond);
    if (condVal->getType() != llvm::Type::getInt1Ty(*context)) {
        condVal = builder->CreateICmpNE(
            condVal,
            llvm::Constant::getNullValue(condVal->getType()),
            "loopcond"
        );
    }
    
    builder->CreateCondBr(condVal, loopBB, afterBB);
    
    currentFunction->insert(currentFunction->end(), loopBB);
    builder->SetInsertPoint(loopBB);
    
    for (const auto& s : stmt.body) {
        genStmt(s);
    }
    
    builder->CreateBr(condBB);
    
    currentFunction->insert(currentFunction->end(), afterBB);
    builder->SetInsertPoint(afterBB);
}

void LLVMCodeGen::genForStmt(const ForStmt& stmt) {
    // For now, simplified implementation
    // TODO: Implement proper iterator protocol
    std::cerr << "For loops not fully implemented yet\n";
}

void LLVMCodeGen::genMatchStmt(const MatchStmt& stmt) {
    // Match on Option types
    auto matchVal = genExpr(stmt.expr);
    
    // TODO: Implement full pattern matching
    std::cerr << "Match statements not fully implemented yet\n";
}

llvm::Value* LLVMCodeGen::genExpr(const ExprPtr& expr) {
    return std::visit([this](auto&& e) -> llvm::Value* {
        using T = std::decay_t<decltype(e)>;
        
        if constexpr (std::is_same_v<T, IntLitExpr>) {
            return genIntLit(e);
        } else if constexpr (std::is_same_v<T, FloatLitExpr>) {
            return genFloatLit(e);
        } else if constexpr (std::is_same_v<T, StringLitExpr>) {
            return genStringLit(e);
        } else if constexpr (std::is_same_v<T, BoolLitExpr>) {
            return genBoolLit(e);
        } else if constexpr (std::is_same_v<T, IdentExpr>) {
            return genIdent(e);
        } else if constexpr (std::is_same_v<T, BinaryExpr>) {
            return genBinaryExpr(e);
        } else if constexpr (std::is_same_v<T, UnaryExpr>) {
            return genUnaryExpr(e);
        } else if constexpr (std::is_same_v<T, CallExpr>) {
            return genCallExpr(e);
        } else if constexpr (std::is_same_v<T, AssignExpr>) {
            return genAssignExpr(e);
        } else if constexpr (std::is_same_v<T, ArrayExpr>) {
            return genArrayExpr(e);
        }
        
        return llvm::Constant::getNullValue(llvm::Type::getInt32Ty(*context));
    }, expr->data);
}

llvm::Value* LLVMCodeGen::genIntLit(const IntLitExpr& expr) {
    return llvm::ConstantInt::get(*context, llvm::APInt(32, expr.value, true));
}

llvm::Value* LLVMCodeGen::genFloatLit(const FloatLitExpr& expr) {
    return llvm::ConstantFP::get(*context, llvm::APFloat(expr.value));
}

llvm::Value* LLVMCodeGen::genStringLit(const StringLitExpr& expr) {
    return createString(expr.value);
}

llvm::Value* LLVMCodeGen::genBoolLit(const BoolLitExpr& expr) {
    return llvm::ConstantInt::get(*context, llvm::APInt(1, expr.value ? 1 : 0));
}

llvm::Value* LLVMCodeGen::genIdent(const IdentExpr& expr) {
    auto val = namedValues[expr.name];
    if (!val) {
        std::cerr << "Unknown variable: " << expr.name << std::endl;
        return nullptr;
    }
    return builder->CreateLoad(val->getAllocatedType(), val, expr.name);
}

llvm::Value* LLVMCodeGen::genBinaryExpr(const BinaryExpr& expr) {
    auto L = genExpr(expr.left);
    auto R = genExpr(expr.right);
    
    if (!L || !R) return nullptr;
    
    if (expr.op == "+") {
        if (L->getType()->isIntegerTy()) {
            return builder->CreateAdd(L, R, "addtmp");
        } else if (L->getType()->isFloatingPointTy()) {
            return builder->CreateFAdd(L, R, "addtmp");
        }
    } else if (expr.op == "-") {
        if (L->getType()->isIntegerTy()) {
            return builder->CreateSub(L, R, "subtmp");
        } else {
            return builder->CreateFSub(L, R, "subtmp");
        }
    } else if (expr.op == "*") {
        if (L->getType()->isIntegerTy()) {
            return builder->CreateMul(L, R, "multmp");
        } else {
            return builder->CreateFMul(L, R, "multmp");
        }
    } else if (expr.op == "/") {
        if (L->getType()->isIntegerTy()) {
            return builder->CreateSDiv(L, R, "divtmp");
        } else {
            return builder->CreateFDiv(L, R, "divtmp");
        }
    } else if (expr.op == "<") {
        if (L->getType()->isIntegerTy()) {
            return builder->CreateICmpSLT(L, R, "cmptmp");
        } else {
            return builder->CreateFCmpOLT(L, R, "cmptmp");
        }
    } else if (expr.op == "==") {
        if (L->getType()->isIntegerTy()) {
            return builder->CreateICmpEQ(L, R, "cmptmp");
        } else {
            return builder->CreateFCmpOEQ(L, R, "cmptmp");
        }
    }
    
    return nullptr;
}

llvm::Value* LLVMCodeGen::genUnaryExpr(const UnaryExpr& expr) {
    auto operand = genExpr(expr.operand);
    if (!operand) return nullptr;
    
    if (expr.op == "-") {
        if (operand->getType()->isIntegerTy()) {
            return builder->CreateNeg(operand, "negtmp");
        } else {
            return builder->CreateFNeg(operand, "negtmp");
        }
    } else if (expr.op == "!") {
        return builder->CreateNot(operand, "nottmp");
    }
    
    return nullptr;
}

llvm::Value* LLVMCodeGen::genCallExpr(const CallExpr& expr) {
    // Get function
    llvm::Function* func = nullptr;
    
    if (auto* ident = std::get_if<IdentExpr>(&expr.callee->data)) {
        func = getStdLibFunction(ident->name);
        if (!func) {
            func = functions[ident->name];
        }
    }
    
    if (!func) {
        std::cerr << "Unknown function\n";
        return nullptr;
    }
    
    // Generate arguments
    std::vector<llvm::Value*> args;
    for (const auto& arg : expr.args) {
        args.push_back(genExpr(arg));
    }
    
    return builder->CreateCall(func, args, "calltmp");
}

llvm::Value* LLVMCodeGen::genAssignExpr(const AssignExpr& expr) {
    auto val = genExpr(expr.value);
    
    if (auto* ident = std::get_if<IdentExpr>(&expr.target->data)) {
        auto var = namedValues[ident->name];
        if (var) {
            builder->CreateStore(val, var);
            return val;
        }
    }
    
    return nullptr;
}

llvm::Value* LLVMCodeGen::genArrayExpr(const ArrayExpr& expr) {
    // TODO: Implement array creation
    return nullptr;
}

llvm::AllocaInst* LLVMCodeGen::createEntryBlockAlloca(llvm::Function* fn, const std::string& varName, llvm::Type* type) {
    llvm::IRBuilder<> tmpBuilder(&fn->getEntryBlock(), fn->getEntryBlock().begin());
    return tmpBuilder.CreateAlloca(type, nullptr, varName);
}

llvm::Value* LLVMCodeGen::createString(const std::string& str) {
    auto strConstant = llvm::ConstantDataArray::getString(*context, str);
    auto strGlobal = new llvm::GlobalVariable(
        *module,
        strConstant->getType(),
        true,
        llvm::GlobalValue::PrivateLinkage,
        strConstant,
        ".str"
    );
    
    return builder->CreatePointerCast(strGlobal, llvm::Type::getInt8PtrTy(*context));
}

void LLVMCodeGen::declareRuntimeFunctions() {
    // Memory management
    auto voidPtrTy = llvm::Type::getInt8PtrTy(*context);
    auto int32Ty = llvm::Type::getInt32Ty(*context);
    
    // malloc
    llvm::FunctionType* mallocType = llvm::FunctionType::get(voidPtrTy, {int32Ty}, false);
    module->getOrInsertFunction("malloc", mallocType);
    
    // free
    llvm::FunctionType* freeType = llvm::FunctionType::get(llvm::Type::getVoidTy(*context), {voidPtrTy}, false);
    module->getOrInsertFunction("free", freeType);
}

void LLVMCodeGen::declareStdLibFunctions() {
    // println
    auto int8PtrTy = llvm::Type::getInt8PtrTy(*context);
    auto voidTy = llvm::Type::getVoidTy(*context);
    
    llvm::FunctionType* printlnType = llvm::FunctionType::get(voidTy, {int8PtrTy}, false);
    module->getOrInsertFunction("mg_println", printlnType);
    
    llvm::FunctionType* printType = llvm::FunctionType::get(voidTy, {int8PtrTy}, false);
    module->getOrInsertFunction("mg_print", printType);
}

llvm::Function* LLVMCodeGen::getStdLibFunction(const std::string& name) {
    if (name == "println") {
        return module->getFunction("mg_println");
    } else if (name == "print") {
        return module->getFunction("mg_print");
    }
    return nullptr;
}

bool LLVMCodeGen::emitObjectFile(const std::string& filename) {
    // Initialize target
    auto targetTriple = llvm::sys::getDefaultTargetTriple();
    module->setTargetTriple(targetTriple);
    
    std::string error;
    auto target = llvm::TargetRegistry::lookupTarget(targetTriple, error);
    if (!target) {
        std::cerr << "Target lookup error: " << error << std::endl;
        return false;
    }
    
    llvm::TargetOptions opt;
    auto targetMachine = target->createTargetMachine(
        targetTriple, "generic", "", opt, llvm::Reloc::PIC_
    );
    
    module->setDataLayout(targetMachine->createDataLayout());
    
    std::error_code EC;
    llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::OF_None);
    
    if (EC) {
        std::cerr << "Could not open file: " << EC.message() << std::endl;
        return false;
    }
    
    llvm::legacy::PassManager pass;
    if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, llvm::CodeGenFileType::ObjectFile)) {
        std::cerr << "TargetMachine can't emit object file\n";
        return false;
    }
    
    pass.run(*module);
    dest.flush();
    
    return true;
}

bool LLVMCodeGen::emitLLVMIR(const std::string& filename) {
    std::error_code EC;
    llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::OF_None);
    
    if (EC) {
        std::cerr << "Could not open file: " << EC.message() << std::endl;
        return false;
    }
    
    module->print(dest, nullptr);
    return true;
}

std::string LLVMCodeGen::mangleName(const std::string& name, const std::string& className) {
    if (className.empty()) {
        return name;
    }
    return "_ZN" + std::to_string(className.length()) + className + 
           std::to_string(name.length()) + name + "E";
}
