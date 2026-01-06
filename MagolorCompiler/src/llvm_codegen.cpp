#include "llvm_codegen.hpp"
#include "stdlib_loader.hpp"
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/IR/LegacyPassManager.h>
#include <variant>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

LLVMCodeGen::LLVMCodeGen(const std::string& moduleName) {
    context = std::make_unique<llvm::LLVMContext>();
    module = std::make_unique<llvm::Module>(moduleName, *context);
    builder = std::make_unique<llvm::IRBuilder<>>(*context);
    
    // Initialize LLVM targets
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
    
    // Find and initialize stdlib
    std::vector<std::string> searchPaths = {
        "./stdlib",
        "/usr/local/share/magolor/stdlib",
        "/usr/share/magolor/stdlib",
        std::string(getenv("HOME") ? getenv("HOME") : "") + "/.magolor/stdlib"
    };
    
    for (const auto& path : searchPaths) {
        if (fs::exists(path)) {
            stdlibPath = path;
            break;
        }
    }
    
    if (!stdlibPath.empty()) {
        StdLibLoader::instance().init(stdlibPath);
        stdlibInitialized = true;
    }
    
    // Declare runtime and stdlib functions
    declareRuntimeFunctions();
    if (stdlibInitialized) {
        declareStdLibFunctions();
    }
}

LLVMCodeGen::~LLVMCodeGen() = default;

void LLVMCodeGen::setStdLibPath(const std::string& path) {
    stdlibPath = path;
    if (fs::exists(path)) {
        StdLibLoader::instance().init(path);
        stdlibInitialized = true;
        declareStdLibFunctions();
    }
}

void LLVMCodeGen::initStdLib() {
    if (stdlibInitialized) return;
    
    if (!stdlibPath.empty() && fs::exists(stdlibPath)) {
        StdLibLoader::instance().init(stdlibPath);
        stdlibInitialized = true;
    }
}

void LLVMCodeGen::declareRuntimeFunctions() {
    auto voidTy = llvm::Type::getVoidTy(*context);
    auto int8PtrTy = llvm::Type::getInt8PtrTy(*context);
    auto int32Ty = llvm::Type::getInt32Ty(*context);
    auto int64Ty = llvm::Type::getInt64Ty(*context);
    auto doubleTy = llvm::Type::getDoubleTy(*context);
    
    // Memory management
    auto mallocType = llvm::FunctionType::get(int8PtrTy, {int64Ty}, false);
    module->getOrInsertFunction("malloc", mallocType);
    
    auto freeType = llvm::FunctionType::get(voidTy, {int8PtrTy}, false);
    module->getOrInsertFunction("free", freeType);
    
    // String operations
    auto stringCreateType = llvm::FunctionType::get(int8PtrTy, {int8PtrTy}, false);
    module->getOrInsertFunction("mg_string_create", stringCreateType);
    
    auto stringDestroyType = llvm::FunctionType::get(voidTy, {int8PtrTy}, false);
    module->getOrInsertFunction("mg_string_destroy", stringDestroyType);
    
    auto stringConcatType = llvm::FunctionType::get(int8PtrTy, {int8PtrTy, int8PtrTy}, false);
    module->getOrInsertFunction("mg_string_concat", stringConcatType);
    
    // I/O operations
    auto printlnType = llvm::FunctionType::get(voidTy, {int8PtrTy}, false);
    module->getOrInsertFunction("mg_println", printlnType);
    module->getOrInsertFunction("mg_print", printlnType);
    
    auto readLineType = llvm::FunctionType::get(int8PtrTy, {}, false);
    module->getOrInsertFunction("mg_readLine", readLineType);
    
    // Type conversions
    auto intToStringType = llvm::FunctionType::get(int8PtrTy, {int32Ty}, false);
    module->getOrInsertFunction("mg_int_to_string", intToStringType);
    
    auto floatToStringType = llvm::FunctionType::get(int8PtrTy, {doubleTy}, false);
    module->getOrInsertFunction("mg_float_to_string", floatToStringType);
    
    auto stringToIntType = llvm::FunctionType::get(int32Ty, {int8PtrTy}, false);
    module->getOrInsertFunction("mg_string_to_int", stringToIntType);
    
    auto stringToFloatType = llvm::FunctionType::get(doubleTy, {int8PtrTy}, false);
    module->getOrInsertFunction("mg_string_to_float", stringToFloatType);
}

void LLVMCodeGen::declareStdLibFunctions() {
    if (!stdlibInitialized) return;
    
    auto voidTy = llvm::Type::getVoidTy(*context);
    auto int8PtrTy = llvm::Type::getInt8PtrTy(*context);
    auto int32Ty = llvm::Type::getInt32Ty(*context);
    auto doubleTy = llvm::Type::getDoubleTy(*context);
    
    // Load common stdlib modules
    std::vector<std::string> modules = {
        "Std.Core.Prelude",
        "Std.IO",
        "Std.String",
        "Std.Array",
        "Std.Math"
    };
    
    for (const auto& modulePath : modules) {
        auto functions = StdLibLoader::instance().getFunctions(modulePath);
        
        for (const auto& func : functions) {
            if (func.isConstant) continue; // Skip constants
            
            // Convert Magolor types to LLVM types
            std::vector<llvm::Type*> paramTypes;
            for (const auto& ptype : func.paramTypes) {
                if (ptype == "int") paramTypes.push_back(int32Ty);
                else if (ptype == "float") paramTypes.push_back(doubleTy);
                else if (ptype == "string") paramTypes.push_back(int8PtrTy);
                else if (ptype == "bool") paramTypes.push_back(llvm::Type::getInt1Ty(*context));
                else paramTypes.push_back(int8PtrTy); // Default to pointer
            }
            
            llvm::Type* returnType = voidTy;
            if (func.returnType == "int") returnType = int32Ty;
            else if (func.returnType == "float") returnType = doubleTy;
            else if (func.returnType == "string") returnType = int8PtrTy;
            else if (func.returnType == "bool") returnType = llvm::Type::getInt1Ty(*context);
            else if (func.returnType != "void") returnType = int8PtrTy;
            
            // Create function declaration
            auto funcType = llvm::FunctionType::get(returnType, paramTypes, false);
            std::string mangledName = "mg_std_" + modulePath + "_" + func.name;
            std::replace(mangledName.begin(), mangledName.end(), '.', '_');
            
            module->getOrInsertFunction(mangledName, funcType);
            functions[func.name] = module->getFunction(mangledName);
        }
    }
}

bool LLVMCodeGen::isStdLibFunction(const std::string& name) {
    if (!stdlibInitialized) return false;
    return StdLibLoader::instance().isStdFunction(name);
}

llvm::Function* LLVMCodeGen::getStdLibFunction(const std::string& name) {
    if (!stdlibInitialized) return nullptr;
    
    auto modulePath = StdLibLoader::instance().getModuleForFunction(name);
    if (modulePath.empty()) return nullptr;
    
    std::string mangledName = "mg_std_" + modulePath + "_" + name;
    std::replace(mangledName.begin(), mangledName.end(), '.', '_');
    
    return module->getFunction(mangledName);
}

llvm::Function* LLVMCodeGen::getRuntimeFunction(const std::string& name) {
    return module->getFunction(name);
}

// ... rest of the implementation from the original llvm_codegen.cpp ...
// (Keep all the existing type conversion, statement generation, expression generation functions)

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

bool LLVMCodeGen::linkExecutable(const std::string& objectFile, const std::string& outputFile) {
    // Compile runtime.c
    std::string runtimeObj = "runtime.o";
    std::string compileRuntime = "gcc -c runtime.c -o " + runtimeObj + " 2>&1";
    if (std::system(compileRuntime.c_str()) != 0) {
        std::cerr << "Failed to compile runtime library\n";
        return false;
    }
    
    // Link everything together
    std::string linkCmd = "gcc " + objectFile + " " + runtimeObj + 
                         " -o " + outputFile + " -lm -lstdc++ 2>&1";
    
    FILE* pipe = popen(linkCmd.c_str(), "r");
    if (!pipe) {
        std::cerr << "Failed to run linker\n";
        return false;
    }
    
    char buffer[128];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    int returnCode = pclose(pipe);
    
    if (returnCode != 0) {
        std::cerr << result;
        std::cerr << "Linking failed\n";
        return false;
    }
    
    // Clean up intermediate files
    fs::remove(runtimeObj);
    
    return true;
}

// Type conversion implementation
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
            return llvm::Type::getInt8PtrTy(*context);
        case Type::VOID:
            return llvm::Type::getVoidTy(*context);
        case Type::CLASS:
            if (classes.count(type->className)) {
                return llvm::PointerType::get(classes[type->className], 0);
            }
            return llvm::PointerType::get(
                llvm::StructType::create(*context, type->className), 0
            );
        case Type::ARRAY:
            return llvm::Type::getInt8PtrTy(*context);
        case Type::OPTION:
            return llvm::Type::getInt8PtrTy(*context);
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
        default:
            return llvm::Type::getInt8PtrTy(*context);
    }
}

llvm::FunctionType* LLVMCodeGen::convertFunctionType(TypePtr returnType, const std::vector<TypePtr>& paramTypes) {
    std::vector<llvm::Type*> llvmParamTypes;
    for (const auto& pt : paramTypes) {
        llvmParamTypes.push_back(convertType(pt));
    }
    return llvm::FunctionType::get(convertType(returnType), llvmParamTypes, false);
}

bool LLVMCodeGen::generate(const Program& prog) {
    initStdLib();
    
    // Declare all classes
    for (const auto& cls : prog.classes) {
        std::vector<llvm::Type*> fieldTypes;
        for (const auto& field : cls.fields) {
            if (!field.isStatic) {
                fieldTypes.push_back(convertType(field.type));
            }
        }
        auto structType = llvm::StructType::create(*context, fieldTypes, cls.name);
        classes[cls.name] = structType;
    }
    
    // Declare all functions
    for (const auto& fn : prog.functions) {
        genFunctionDecl(fn);
    }
    
    // Generate class methods
    for (const auto& cls : prog.classes) {
        genClass(cls);
    }
    
    // Generate function bodies
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
    
    for (const auto& field : cls.fields) {
        if (field.isStatic && field.initValue) {
            genGlobalVar(mangleName(field.name, cls.name), field.type, field.initValue);
        }
    }
    
    for (const auto& method : cls.methods) {
        genFunction(method, mangleName(method.name, cls.name));
    }
    
    currentClass = nullptr;
}

llvm::Function* LLVMCodeGen::genFunctionDecl(const FnDecl& fn, const std::string& mangledName) {
    std::string funcName = mangledName.empty() ? fn.name : mangledName;
    
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
    
    auto entryBB = llvm::BasicBlock::Create(*context, "entry", func);
    builder->SetInsertPoint(entryBB);
    
    currentFunction = func;
    namedValues.clear();
    
    for (auto& arg : func->args()) {
        auto alloca = createEntryBlockAlloca(func, std::string(arg.getName()), arg.getType());
        builder->CreateStore(&arg, alloca);
        namedValues[std::string(arg.getName())] = alloca;
    }
    
    for (const auto& stmt : fn.body) {
        genStmt(stmt);
    }
    
    if (!builder->GetInsertBlock()->getTerminator()) {
        if (fn.returnType->kind == Type::VOID) {
            builder->CreateRetVoid();
        } else {
            builder->CreateRet(llvm::Constant::getNullValue(convertType(fn.returnType)));
        }
    }
    
    currentFunction = nullptr;
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

std::string LLVMCodeGen::mangleName(const std::string& name, const std::string& className) {
    if (className.empty()) {
        return name;
    }
    return className + "_" + name;
}

// Statement generation (keeping the implementation from original file)
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
    
    builder->SetInsertPoint(thenBB);
    for (const auto& s : stmt.thenBody) {
        genStmt(s);
    }
    if (!builder->GetInsertBlock()->getTerminator()) {
        builder->CreateBr(mergeBB);
    }
    
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

void LLVMCodeGen::genForStmt(const ForStmt&) {
    // Simplified for now
}

// Expression generation
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
        } else if constexpr (std::is_same_v<T, CallExpr>) {
            return genCallExpr(e);
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
        } else {
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
    }
    
    return nullptr;
}

llvm::Value* LLVMCodeGen::genCallExpr(const CallExpr& expr) {
    llvm::Function* func = nullptr;
    
    if (auto* ident = std::get_if<IdentExpr>(&expr.callee->data)) {
        func = getStdLibFunction(ident->name);
        if (!func) {
            func = getRuntimeFunction("mg_" + ident->name);
        }
        if (!func) {
            func = functions[ident->name];
        }
    }
    
    if (!func) {
        return nullptr;
    }
    
    std::vector<llvm::Value*> args;
    for (const auto& arg : expr.args) {
        args.push_back(genExpr(arg));
    }
    
    return builder->CreateCall(func, args, "calltmp");
}

void LLVMCodeGen::genGlobalVar(const std::string& name, TypePtr type, ExprPtr) {
    // Simplified
}

void LLVMCodeGen::genIncRef(llvm::Value*) {
    // Simplified
}

void LLVMCodeGen::genDecRef(llvm::Value*) {
    // Simplified
}
