// codegen_llvm_funcs.cpp - LLVM IR Code Generator for Magolor
// Part 2: Function and Statement Generation

#include "codegen_llvm.hpp"
#include <llvm/TargetParser/Host.h>
#include <sstream>

// =============================================================================
// Main Generation Entry Point
// =============================================================================

llvm::Module* LLVMCodeGen::generate(const Program& prog, 
                                                    const std::string& moduleName) {
    // Create the module
 module = std::make_unique<llvm::Module>(moduleName, *context);
    module->setTargetTriple(llvm::Triple(llvm::sys::getDefaultTargetTriple()));
    
    // Initialize runtime functions
    initializeRuntime();
    
    // ALWAYS generate core stdlib functions (they're lightweight)
    generateStdIO();
    generateStdMath();
    generateStdString();
    generateStdArray();
    generateStdOption();
    
    // Track used modules
    for (const auto& usingDecl : prog.usings) {
        std::string modulePath;
        for (size_t i = 0; i < usingDecl.path.size(); i++) {
            if (i > 0) modulePath += ".";
            modulePath += usingDecl.path[i];
        }
        usedModules.insert(modulePath);
    }
    
    // Generate additional stdlib based on what's used
    for (const auto& mod : usedModules) {
        if (mod.find("File") != std::string::npos) generateStdFile();
        if (mod.find("Time") != std::string::npos) generateStdTime();
        if (mod.find("Random") != std::string::npos) generateStdRandom();
        if (mod.find("System") != std::string::npos) generateStdSystem();
        if (mod.find("Network") != std::string::npos) generateStdNetwork();
        if (mod.find("Crypto") != std::string::npos) generateStdCrypto();
        if (mod.find("Map") != std::string::npos) generateStdMap();
    }
    
    // Generate class layouts first (forward declarations)
    generateClassLayouts(prog.classes);
    
    // Generate function declarations
 for (const auto& fn : prog.functions) {
    if (fn.name != "main") {
        generateFunctionDeclaration(fn);
    }
}
    
    // Generate class implementations
    for (const auto& cls : prog.classes) {
        generateClass(cls);
    }
    
    // Generate function implementations

for (const auto& fn : prog.functions) {
    if (fn.name == "main") {
        generateMain(fn);
    } else {
        generateFunction(fn);
    }
}

    
    // Verify module before returning
    if (!verify()) {
        llvm::errs() << "Module verification failed!\n";
        return nullptr;
    }
    
   return module.get();

}

// =============================================================================
// Function Generation
// =============================================================================

void LLVMCodeGen::generateFunctionDeclarations(const std::vector<FnDecl>& funcs) {
    for (const auto& fn : funcs) {
        generateFunctionDeclaration(fn);
    }
}

void LLVMCodeGen::generateFunctionDeclaration(const FnDecl& fn, 
                                              const std::string& className) {
    std::vector<llvm::Type*> paramTypes;
    
    // Methods get 'this' pointer as first parameter
    if (!className.empty()) {
        auto& layout = classLayouts[className];
        paramTypes.push_back(llvm::PointerType::get(*context, 0));
	}
    
    // Add declared parameters
    for (const auto& param : fn.params) {
        paramTypes.push_back(toLLVMType(param.type));
    }
    
    auto* retType = toLLVMType(fn.returnType);
    
    // Main function returns int
    if (fn.name == "main" && className.empty()) {
        retType = llvm::Type::getInt32Ty(*context);
    }
    
    auto* fnType = llvm::FunctionType::get(retType, paramTypes, false);
    
    std::string fnName = className.empty() ? fn.name : getMangledName(className, fn.name);
    
    // Don't redeclare if already exists
    if (module->getFunction(fnName)) {
        return;
    }
    
    auto* llvmFn = llvm::Function::Create(fnType, llvm::Function::ExternalLinkage, 
                                          fnName, module.get());
    
    // Name the parameters
    size_t idx = 0;
    if (!className.empty()) {
        llvmFn->getArg(idx++)->setName("this");
    }
    for (const auto& param : fn.params) {
        llvmFn->getArg(idx++)->setName(param.name);
    }
    
    functions[fnName] = llvmFn;
}

void LLVMCodeGen::generateFunction(const FnDecl& fn, const std::string& className) {
    std::string fnName = className.empty() ? fn.name : getMangledName(className, fn.name);
    
    if (fn.name == "create" && !className.empty()) {
        return;
    }
    
    // Skip if function already exists (e.g., from stdlib codegen)
    auto* existingFn = module->getFunction(fnName);
    if (existingFn && !existingFn->empty()) {
        // Function already has a body, don't regenerate
        return;
    }
    
    // Get or create the function
    auto* llvmFn = module->getFunction(fnName);
    if (!llvmFn) {
        generateFunctionDeclaration(fn, className);
        llvmFn = module->getFunction(fnName);
    }
    

    
    if (!llvmFn) {
        emitError("Failed to create function: " + fnName);
        return;
    }
    
    // Create entry block
    auto* entry = llvm::BasicBlock::Create(*context, "entry", llvmFn);
    builder->SetInsertPoint(entry);
    
    currentFunction = llvmFn;
    currentClassName = className;
    
    pushScope();
    
    // Set up 'this' pointer for methods
    size_t argIdx = 0;
    if (!className.empty()) {
        currentThis = llvmFn->getArg(argIdx++);
    }
    
    // Allocate and store parameters
    for (const auto& param : fn.params) {
        auto* arg = llvmFn->getArg(argIdx++);
        auto* alloca = createEntryBlockAlloca(llvmFn, param.name, arg->getType());
        builder->CreateStore(arg, alloca);
        declareVar(param.name, alloca, arg->getType(), param.type, false);
    }
    
    // Generate function body
    for (const auto& stmt : fn.body) {
        genStmt(stmt);
    }
    
    // Add implicit return if needed
    if (!builder->GetInsertBlock()->getTerminator()) {
        auto* retType = llvmFn->getReturnType();
        if (retType->isVoidTy()) {
            builder->CreateRetVoid();
        } else {
            builder->CreateRet(getDefaultValue(retType));
        }
    }
    
    popScope();
    
    currentFunction = nullptr;
    currentClassName.clear();
    currentThis = nullptr;
}

void LLVMCodeGen::generateMain(const FnDecl& fn) {
    auto* i32Ty = llvm::Type::getInt32Ty(*context);
    auto* fnType = llvm::FunctionType::get(i32Ty, {}, false);
    
    auto* mainFn = llvm::Function::Create(fnType, llvm::Function::ExternalLinkage, 
                                          "main", module.get());
    
    auto* entry = llvm::BasicBlock::Create(*context, "entry", mainFn);
    builder->SetInsertPoint(entry);
    
    currentFunction = mainFn;
    
    pushScope();
    
    // Generate function body
    for (const auto& stmt : fn.body) {
        genStmt(stmt);
    }
    
    // Ensure we return 0
    if (!builder->GetInsertBlock()->getTerminator()) {
        builder->CreateRet(llvm::ConstantInt::get(i32Ty, 0));
    }
    
    popScope();
    
    currentFunction = nullptr;
    functions["main"] = mainFn;
}

// =============================================================================
// Statement Generation
// =============================================================================

void LLVMCodeGen::genStmt(const StmtPtr& stmt) {
    std::visit([this](auto&& s) {
        using T = std::decay_t<decltype(s)>;
        if constexpr (std::is_same_v<T, LetStmt>) {
            genLetStmt(s);
        } else if constexpr (std::is_same_v<T, ReturnStmt>) {
            genReturnStmt(s);
        } else if constexpr (std::is_same_v<T, ExprStmt>) {
            genExprStmt(s);
        } else if constexpr (std::is_same_v<T, IfStmt>) {
            genIfStmt(s);
        } else if constexpr (std::is_same_v<T, WhileStmt>) {
            genWhileStmt(s);
        } else if constexpr (std::is_same_v<T, ForStmt>) {
            genForStmt(s);
        } else if constexpr (std::is_same_v<T, MatchStmt>) {
            genMatchStmt(s);
        } else if constexpr (std::is_same_v<T, BlockStmt>) {
            genBlockStmt(s);
        } else if constexpr (std::is_same_v<T, CppStmt>) {
            genCppStmt(s);
        }
    }, stmt->data);
}

void LLVMCodeGen::genLetStmt(const LetStmt& s) {
    llvm::Type* type;
    llvm::Value* initVal = nullptr;
    
    if (s.init) {
        initVal = genExpr(s.init);
        type = initVal->getType();
    } else if (s.type) {
        type = toLLVMType(s.type);
        initVal = getDefaultValue(type);
    } else {
        type = llvm::Type::getInt64Ty(*context);
        initVal = getDefaultValue(type);
    }
    
    auto* alloca = createEntryBlockAlloca(currentFunction, s.name, type);
    builder->CreateStore(initVal, alloca);
    
    declareVar(s.name, alloca, type, s.type, s.isMut);
}

void LLVMCodeGen::genReturnStmt(const ReturnStmt& s) {
    if (s.value) {
        // Handle 'return this' for method chaining
        if (std::holds_alternative<ThisExpr>(s.value->data) && !currentClassName.empty()) {
            builder->CreateRet(currentThis);
        } else {
            auto* val = genExpr(s.value);
            
            // Cast to return type if needed
            auto* retType = currentFunction->getReturnType();
            if (val->getType() != retType && !retType->isVoidTy()) {
                val = castValue(val, retType);
            }
            
            builder->CreateRet(val);
        }
    } else {
        builder->CreateRetVoid();
    }
}

void LLVMCodeGen::genExprStmt(const ExprStmt& s) {
    genExpr(s.expr);
}

void LLVMCodeGen::genIfStmt(const IfStmt& s) {
    auto* cond = genExpr(s.cond);
    
    // Convert to i1 if not already
    if (!cond->getType()->isIntegerTy(1)) {
        cond = builder->CreateICmpNE(cond, 
            llvm::Constant::getNullValue(cond->getType()), "ifcond");
    }
    
    auto* thenBB = llvm::BasicBlock::Create(*context, "then", currentFunction);
    auto* elseBB = llvm::BasicBlock::Create(*context, "else", currentFunction);
    auto* mergeBB = llvm::BasicBlock::Create(*context, "ifcont", currentFunction);
    
    builder->CreateCondBr(cond, thenBB, elseBB);
    
    // Generate then block
    builder->SetInsertPoint(thenBB);
    pushScope();
    for (const auto& stmt : s.thenBody) {
        genStmt(stmt);
    }
    popScope();
    if (!builder->GetInsertBlock()->getTerminator()) {
        builder->CreateBr(mergeBB);
    }
    
    // Generate else block
    builder->SetInsertPoint(elseBB);
    pushScope();
    for (const auto& stmt : s.elseBody) {
        genStmt(stmt);
    }
    popScope();
    if (!builder->GetInsertBlock()->getTerminator()) {
        builder->CreateBr(mergeBB);
    }
    
    // Continue after if
    builder->SetInsertPoint(mergeBB);
}

void LLVMCodeGen::genWhileStmt(const WhileStmt& s) {
    auto* condBB = llvm::BasicBlock::Create(*context, "while.cond", currentFunction);
    auto* bodyBB = llvm::BasicBlock::Create(*context, "while.body", currentFunction);
    auto* exitBB = llvm::BasicBlock::Create(*context, "while.exit", currentFunction);
    
    // Save loop context for break/continue
    auto* prevContinue = currentLoopContinue;
    auto* prevBreak = currentLoopBreak;
    currentLoopContinue = condBB;
    currentLoopBreak = exitBB;
    
    builder->CreateBr(condBB);
    
    // Condition
    builder->SetInsertPoint(condBB);
    auto* cond = genExpr(s.cond);
    if (!cond->getType()->isIntegerTy(1)) {
        cond = builder->CreateICmpNE(cond, 
            llvm::Constant::getNullValue(cond->getType()), "loopcond");
    }
    builder->CreateCondBr(cond, bodyBB, exitBB);
    
    // Body
    builder->SetInsertPoint(bodyBB);
    pushScope();
    for (const auto& stmt : s.body) {
        genStmt(stmt);
    }
    popScope();
    if (!builder->GetInsertBlock()->getTerminator()) {
        builder->CreateBr(condBB);
    }
    
    // Exit
    builder->SetInsertPoint(exitBB);
    
    // Restore loop context
    currentLoopContinue = prevContinue;
    currentLoopBreak = prevBreak;
}

void LLVMCodeGen::genForStmt(const ForStmt& s) {
    // Generate the iterable
    auto* iterable = genExpr(s.iterable);
    
    // For arrays, we iterate with an index
    auto* i64Ty = llvm::Type::getInt64Ty(*context);
    
    // Create loop variable and index
    auto* indexAlloca = createEntryBlockAlloca(currentFunction, "__idx", i64Ty);
    builder->CreateStore(llvm::ConstantInt::get(i64Ty, 0), indexAlloca);
    
    // Get array length
    auto* lengthCall = builder->CreateCall(runtime.stringLength, {iterable});
    
    auto* condBB = llvm::BasicBlock::Create(*context, "for.cond", currentFunction);
    auto* bodyBB = llvm::BasicBlock::Create(*context, "for.body", currentFunction);
    auto* incBB = llvm::BasicBlock::Create(*context, "for.inc", currentFunction);
    auto* exitBB = llvm::BasicBlock::Create(*context, "for.exit", currentFunction);
    
    auto* prevContinue = currentLoopContinue;
    auto* prevBreak = currentLoopBreak;
    currentLoopContinue = incBB;
    currentLoopBreak = exitBB;
    
    builder->CreateBr(condBB);
    
    // Condition: index < length
    builder->SetInsertPoint(condBB);
    auto* index = builder->CreateLoad(i64Ty, indexAlloca);
    auto* cond = builder->CreateICmpSLT(index, lengthCall, "forcond");
    builder->CreateCondBr(cond, bodyBB, exitBB);
    
    // Body
    builder->SetInsertPoint(bodyBB);
    pushScope();
    
    // Get element at current index
    // For simplicity, treat as array access
    auto* elemPtr = builder->CreateGEP(llvm::Type::getInt8Ty(*context), iterable, index);
    auto* elemType = toLLVMType(s.iterable->type ? s.iterable->type->innerType : nullptr);
    if (!elemType) elemType = llvm::Type::getInt64Ty(*context);
    
    auto* varAlloca = createEntryBlockAlloca(currentFunction, s.var, elemType);
    
    // Load element value appropriately
    if (elemType->isIntegerTy(64)) {
        auto* castPtr = builder->CreateBitCast(elemPtr, 
          llvm::PointerType::get(*context, 0));
        auto* elem = builder->CreateLoad(i64Ty, castPtr);
        builder->CreateStore(elem, varAlloca);
    } else {
        builder->CreateStore(builder->CreateLoad(elemType, elemPtr), varAlloca);
    }
    
    declareVar(s.var, varAlloca, elemType, nullptr, false);
    
    for (const auto& stmt : s.body) {
        genStmt(stmt);
    }
    
    popScope();
    if (!builder->GetInsertBlock()->getTerminator()) {
        builder->CreateBr(incBB);
    }
    
    // Increment
    builder->SetInsertPoint(incBB);
    auto* newIndex = builder->CreateAdd(
        builder->CreateLoad(i64Ty, indexAlloca),
        llvm::ConstantInt::get(i64Ty, 1)
    );
    builder->CreateStore(newIndex, indexAlloca);
    builder->CreateBr(condBB);
    
    // Exit
    builder->SetInsertPoint(exitBB);
    
    currentLoopContinue = prevContinue;
    currentLoopBreak = prevBreak;
}

void LLVMCodeGen::genMatchStmt(const MatchStmt& s) {
    auto* matchVal = genExpr(s.expr);
    
    // Create blocks for each arm and the exit
    std::vector<llvm::BasicBlock*> armBlocks;
    for (size_t i = 0; i < s.arms.size(); i++) {
        armBlocks.push_back(llvm::BasicBlock::Create(*context, 
            "match.arm" + std::to_string(i), currentFunction));
    }
    auto* exitBB = llvm::BasicBlock::Create(*context, "match.exit", currentFunction);
    
    // For Option types, check Some/None patterns
    bool isOption = false;
    if (s.expr->type && (s.expr->type->kind == Type::OPTION || 
        (s.expr->type->kind == Type::GENERIC && s.expr->type->className == "Option"))) {
        isOption = true;
    }
    
    // Generate condition checks
    llvm::BasicBlock* currentCheckBB = builder->GetInsertBlock();
    
    for (size_t i = 0; i < s.arms.size(); i++) {
        const auto& arm = s.arms[i];
        auto* armBB = armBlocks[i];
        auto* nextCheckBB = (i + 1 < s.arms.size()) ? 
            llvm::BasicBlock::Create(*context, "match.check" + std::to_string(i+1), currentFunction) :
            exitBB;
        
        builder->SetInsertPoint(currentCheckBB);
        
        llvm::Value* cond = nullptr;
        
        if (arm.pattern == "Some") {
            if (isOption) {
                // Check has_value field
                auto* hasValPtr = builder->CreateStructGEP(matchVal->getType(), matchVal, 0);
                cond = builder->CreateLoad(llvm::Type::getInt1Ty(*context), hasValPtr);
            } else {
                // Non-null check for pointers
                cond = builder->CreateICmpNE(matchVal, 
                    llvm::Constant::getNullValue(matchVal->getType()));
            }
        } else if (arm.pattern == "None") {
            if (isOption) {
                auto* hasValPtr = builder->CreateStructGEP(matchVal->getType(), matchVal, 0);
                auto* hasVal = builder->CreateLoad(llvm::Type::getInt1Ty(*context), hasValPtr);
                cond = builder->CreateNot(hasVal);
            } else {
                cond = builder->CreateICmpEQ(matchVal, 
                    llvm::Constant::getNullValue(matchVal->getType()));
            }
        } else if (arm.pattern == "_") {
            // Wildcard - always matches
            cond = llvm::ConstantInt::getTrue(*context);
        } else {
            // Value comparison
            // Try to parse as integer
            try {
                int64_t patternVal = std::stoll(arm.pattern);
                cond = builder->CreateICmpEQ(matchVal, 
                    llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), patternVal));
            } catch (...) {
                // String comparison
                auto* patternStr = createString(arm.pattern);
                auto* strcmp = module->getFunction("strcmp");
                auto* cmpResult = builder->CreateCall(strcmp, {matchVal, patternStr});
                cond = builder->CreateICmpEQ(cmpResult, 
                    llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0));
            }
        }
        
        builder->CreateCondBr(cond, armBB, nextCheckBB);
        
        // Generate arm body
        builder->SetInsertPoint(armBB);
        pushScope();
        
        // Bind variable if present
        if (!arm.bindVar.empty()) {
            if (isOption && arm.pattern == "Some") {
                // Extract value from Option
                auto* valPtr = builder->CreateStructGEP(matchVal->getType(), matchVal, 1);
                auto* innerType = matchVal->getType()->getStructElementType(1);
                auto* val = builder->CreateLoad(innerType, valPtr);
                auto* alloca = createEntryBlockAlloca(currentFunction, arm.bindVar, innerType);
                builder->CreateStore(val, alloca);
                declareVar(arm.bindVar, alloca, innerType, nullptr, false);
            } else {
                auto* alloca = createEntryBlockAlloca(currentFunction, arm.bindVar, 
                                                      matchVal->getType());
                builder->CreateStore(matchVal, alloca);
                declareVar(arm.bindVar, alloca, matchVal->getType(), nullptr, false);
            }
        }
        
        for (const auto& stmt : arm.body) {
            genStmt(stmt);
        }
        
        popScope();
        
        if (!builder->GetInsertBlock()->getTerminator()) {
            builder->CreateBr(exitBB);
        }
        
        currentCheckBB = nextCheckBB;
    }
    
    // Continue after match
    builder->SetInsertPoint(exitBB);
}

void LLVMCodeGen::genBlockStmt(const BlockStmt& s) {
    pushScope();
    for (const auto& stmt : s.stmts) {
        genStmt(stmt);
    }
    popScope();
}

void LLVMCodeGen::genCppStmt(const CppStmt& s) {
    // @cpp blocks contain raw C++ code
    // In LLVM IR mode, we emit this as inline assembly or skip it
    // For now, we'll create a comment and potentially call external functions
    
    // Note: True inline C++ is not possible in pure LLVM IR
    // The code would need to be compiled separately and linked
    
    // Create a runtime warning that @cpp blocks need special handling
    auto* warnMsg = createString("Warning: @cpp block encountered - requires external compilation");
    builder->CreateCall(runtime.printlnStr, {warnMsg});
}
