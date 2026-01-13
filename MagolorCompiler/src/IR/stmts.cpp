#include "ir_builder.hpp"
#include <variant>

namespace IR {

// ============================================================================
// Statement builders
// ============================================================================

void IRBuilder::buildStmt(const StmtPtr& stmt) {
    std::visit([this](auto&& s) {
        using T = std::decay_t<decltype(s)>;
        if constexpr (std::is_same_v<T, LetStmt>) {
            buildLetStmt(s);
        } else if constexpr (std::is_same_v<T, ReturnStmt>) {
            buildReturnStmt(s);
        } else if constexpr (std::is_same_v<T, ExprStmt>) {
            buildExprStmt(s);
        } else if constexpr (std::is_same_v<T, IfStmt>) {
            buildIfStmt(s);
        } else if constexpr (std::is_same_v<T, WhileStmt>) {
            buildWhileStmt(s);
        } else if constexpr (std::is_same_v<T, ForStmt>) {
            buildForStmt(s);
        } else if constexpr (std::is_same_v<T, MatchStmt>) {
            buildMatchStmt(s);
        } else if constexpr (std::is_same_v<T, BlockStmt>) {
            buildBlockStmt(s);
        } else if constexpr (std::is_same_v<T, CppStmt>) {
            buildCppStmt(s);
        }
    }, stmt->data);
}

void IRBuilder::buildLetStmt(const LetStmt& let) {
    // Build initializer expression
    IRValuePtr initValue = buildExpr(let.init);
    
    // Determine type
    IRTypePtr varType;
    if (let.type) {
        varType = convertType(let.type);
    } else if (initValue && initValue->type) {
        varType = initValue->type;
    } else {
        varType = IRType::makeAuto();
    }
    
    // Create alloca instruction for the variable
    auto allocaResult = createTemp(IRType::makePointer(varType));
    auto allocaInst = IRInstruction::make(IROpcode::Alloca, allocaResult);
    allocaInst->type = varType;
    allocaInst->stringData = let.name;
    emit(allocaInst);
    
    // Store the initial value
    auto storeInst = IRInstruction::make(IROpcode::Store);
    storeInst->operands.push_back(initValue);
    storeInst->operands.push_back(allocaResult);
    emit(storeInst);
    
    // Register the variable in current scope
    declareVariable(let.name, allocaResult);
}

void IRBuilder::buildReturnStmt(const ReturnStmt& ret) {
    if (ret.value) {
        // Check for 'return this' pattern
        if (std::holds_alternative<ThisExpr>(ret.value->data)) {
            if (!currentClassName.empty()) {
                // Return *this (dereference)
                auto thisInst = IRInstruction::make(IROpcode::This);
                thisInst->result = createTemp(IRType::makePointer(IRType::makeClass(currentClassName)));
                thisInst->className = currentClassName;
                auto thisPtr = emit(thisInst);
                
                auto loadInst = IRInstruction::make(IROpcode::Load);
                loadInst->result = createTemp(IRType::makeClass(currentClassName));
                loadInst->operands.push_back(thisPtr);
                auto thisVal = emit(loadInst);
                
                emitReturn(thisVal);
                return;
            }
        }
        
        IRValuePtr retValue = buildExpr(ret.value);
        emitReturn(retValue);
    } else {
        emitReturn();
    }
}

void IRBuilder::buildExprStmt(const ExprStmt& expr) {
    buildExpr(expr.expr);
}

void IRBuilder::buildIfStmt(const IfStmt& ifStmt) {
    // Build condition
    IRValuePtr condValue = buildExpr(ifStmt.cond);
    
    // Create blocks
    auto thenBlock = createBlock("if.then");
    auto elseBlock = ifStmt.elseBody.empty() ? nullptr : createBlock("if.else");
    auto mergeBlock = createBlock("if.merge");
    
    // Emit conditional branch
    if (elseBlock) {
        emitCondBranch(condValue, thenBlock, elseBlock);
    } else {
        emitCondBranch(condValue, thenBlock, mergeBlock);
    }
    
    // Build then block
    setInsertPoint(thenBlock);
    currentFunction->addBlock(thenBlock);
    pushScope();
    for (const auto& stmt : ifStmt.thenBody) {
        buildStmt(stmt);
    }
    popScope();
    if (!currentBlock->isTerminated()) {
        emitBranch(mergeBlock);
    }
    
    // Build else block if present
    if (elseBlock) {
        setInsertPoint(elseBlock);
        currentFunction->addBlock(elseBlock);
        pushScope();
        for (const auto& stmt : ifStmt.elseBody) {
            buildStmt(stmt);
        }
        popScope();
        if (!currentBlock->isTerminated()) {
            emitBranch(mergeBlock);
        }
    }
    
    // Continue in merge block
    setInsertPoint(mergeBlock);
    currentFunction->addBlock(mergeBlock);
}

void IRBuilder::buildWhileStmt(const WhileStmt& whileStmt) {
    // Create blocks
    auto condBlock = createBlock("while.cond");
    auto bodyBlock = createBlock("while.body");
    auto exitBlock = createBlock("while.exit");
    
    // Push loop context for break/continue
    loopStack.push({exitBlock, condBlock});
    
    // Branch to condition check
    emitBranch(condBlock);
    
    // Build condition block
    setInsertPoint(condBlock);
    currentFunction->addBlock(condBlock);
    IRValuePtr condValue = buildExpr(whileStmt.cond);
    emitCondBranch(condValue, bodyBlock, exitBlock);
    
    // Build body block
    setInsertPoint(bodyBlock);
    currentFunction->addBlock(bodyBlock);
    pushScope();
    for (const auto& stmt : whileStmt.body) {
        buildStmt(stmt);
    }
    popScope();
    if (!currentBlock->isTerminated()) {
        emitBranch(condBlock);
    }
    
    // Pop loop context
    loopStack.pop();
    
    // Continue in exit block
    setInsertPoint(exitBlock);
    currentFunction->addBlock(exitBlock);
}

void IRBuilder::buildForStmt(const ForStmt& forStmt) {
    // Build iterable expression
    IRValuePtr iterableValue = buildExpr(forStmt.iterable);
    
    // Create blocks
    auto initBlock = createBlock("for.init");
    auto condBlock = createBlock("for.cond");
    auto bodyBlock = createBlock("for.body");
    auto incrBlock = createBlock("for.incr");
    auto exitBlock = createBlock("for.exit");
    
    // Push loop context
    loopStack.push({exitBlock, incrBlock});
    
    emitBranch(initBlock);
    
    // Init block - create iterator
    setInsertPoint(initBlock);
    currentFunction->addBlock(initBlock);
    
    // Create index variable (for range-based iteration)
    auto indexType = IRType::makeInt64();
    auto indexPtr = createTemp(IRType::makePointer(indexType));
    auto indexAlloca = IRInstruction::make(IROpcode::Alloca, indexPtr);
    indexAlloca->type = indexType;
    indexAlloca->stringData = "__index";
    emit(indexAlloca);
    
    // Initialize index to 0
    auto zeroVal = createConst(static_cast<int64_t>(0));
    auto storeIdx = IRInstruction::make(IROpcode::Store);
    storeIdx->operands.push_back(zeroVal);
    storeIdx->operands.push_back(indexPtr);
    emit(storeIdx);
    
    // Get array length
    auto lengthInst = IRInstruction::make(IROpcode::ArrayLength);
    lengthInst->result = createTemp(IRType::makeInt64());
    lengthInst->operands.push_back(iterableValue);
    auto lengthVal = emit(lengthInst);
    
    emitBranch(condBlock);
    
    // Condition block - check if index < length
    setInsertPoint(condBlock);
    currentFunction->addBlock(condBlock);
    
    // Load current index
    auto loadIdx = IRInstruction::make(IROpcode::Load);
    loadIdx->result = createTemp(IRType::makeInt64());
    loadIdx->operands.push_back(indexPtr);
    auto currentIdx = emit(loadIdx);
    
    // Compare index < length
    auto compareInst = IRInstruction::make(IROpcode::Lt);
    compareInst->result = createTemp(IRType::makeBool());
    compareInst->operands.push_back(currentIdx);
    compareInst->operands.push_back(lengthVal);
    auto condResult = emit(compareInst);
    
    emitCondBranch(condResult, bodyBlock, exitBlock);
    
    // Body block
    setInsertPoint(bodyBlock);
    currentFunction->addBlock(bodyBlock);
    pushScope();
    
    // Load current index again
    auto loadIdxBody = IRInstruction::make(IROpcode::Load);
    loadIdxBody->result = createTemp(IRType::makeInt64());
    loadIdxBody->operands.push_back(indexPtr);
    auto bodyIdx = emit(loadIdxBody);
    
    // Get element at index
    IRTypePtr elemType = IRType::makeAuto();
    if (iterableValue->type && iterableValue->type->kind == IRTypeKind::Array) {
        elemType = iterableValue->type->elementType;
    }
    
    auto getElemInst = IRInstruction::make(IROpcode::ArrayGet);
    getElemInst->result = createTemp(elemType);
    getElemInst->operands.push_back(iterableValue);
    getElemInst->operands.push_back(bodyIdx);
    auto elemValue = emit(getElemInst);
    
    // Declare loop variable
    auto varPtr = createTemp(IRType::makePointer(elemType));
    auto varAlloca = IRInstruction::make(IROpcode::Alloca, varPtr);
    varAlloca->type = elemType;
    varAlloca->stringData = forStmt.var;
    emit(varAlloca);
    
    auto storeVar = IRInstruction::make(IROpcode::Store);
    storeVar->operands.push_back(elemValue);
    storeVar->operands.push_back(varPtr);
    emit(storeVar);
    
    declareVariable(forStmt.var, varPtr);
    
    // Build body statements
    for (const auto& stmt : forStmt.body) {
        buildStmt(stmt);
    }
    
    popScope();
    
    if (!currentBlock->isTerminated()) {
        emitBranch(incrBlock);
    }
    
    // Increment block
    setInsertPoint(incrBlock);
    currentFunction->addBlock(incrBlock);
    
    // Load, increment, store index
    auto loadIdxIncr = IRInstruction::make(IROpcode::Load);
    loadIdxIncr->result = createTemp(IRType::makeInt64());
    loadIdxIncr->operands.push_back(indexPtr);
    auto incrIdx = emit(loadIdxIncr);
    
    auto oneVal = createConst(static_cast<int64_t>(1));
    auto addInst = IRInstruction::make(IROpcode::Add);
    addInst->result = createTemp(IRType::makeInt64());
    addInst->operands.push_back(incrIdx);
    addInst->operands.push_back(oneVal);
    auto newIdx = emit(addInst);
    
    auto storeNewIdx = IRInstruction::make(IROpcode::Store);
    storeNewIdx->operands.push_back(newIdx);
    storeNewIdx->operands.push_back(indexPtr);
    emit(storeNewIdx);
    
    emitBranch(condBlock);
    
    // Pop loop context
    loopStack.pop();
    
    // Continue in exit block
    setInsertPoint(exitBlock);
    currentFunction->addBlock(exitBlock);
}

void IRBuilder::buildMatchStmt(const MatchStmt& match) {
    // Build match expression
    IRValuePtr matchValue = buildExpr(match.expr);
    
    // Create merge block
    auto mergeBlock = createBlock("match.merge");
    
    // Store match value for use in arms
    auto matchValuePtr = createTemp(IRType::makePointer(matchValue->type));
    auto allocaMatch = IRInstruction::make(IROpcode::Alloca, matchValuePtr);
    allocaMatch->type = matchValue->type;
    allocaMatch->stringData = "_match_val";
    emit(allocaMatch);
    
    auto storeMatch = IRInstruction::make(IROpcode::Store);
    storeMatch->operands.push_back(matchValue);
    storeMatch->operands.push_back(matchValuePtr);
    emit(storeMatch);
    
    declareVariable("_match_val", matchValuePtr);
    
    // Build arms
    IRBasicBlockPtr prevFalseBlock = nullptr;
    
    for (size_t i = 0; i < match.arms.size(); i++) {
        const auto& arm = match.arms[i];
        bool isLast = (i == match.arms.size() - 1);
        
        auto armCondBlock = createBlock("match.arm." + std::to_string(i) + ".cond");
        auto armBodyBlock = createBlock("match.arm." + std::to_string(i) + ".body");
        auto nextBlock = isLast ? mergeBlock : createBlock("match.arm." + std::to_string(i) + ".next");
        
        // Branch to condition
        if (prevFalseBlock) {
            setInsertPoint(prevFalseBlock);
        }
        emitBranch(armCondBlock);
        
        // Condition block
        setInsertPoint(armCondBlock);
        currentFunction->addBlock(armCondBlock);
        
        // Load match value
        auto loadMatch = IRInstruction::make(IROpcode::Load);
        loadMatch->result = createTemp(matchValue->type);
        loadMatch->operands.push_back(matchValuePtr);
        auto currentMatchVal = emit(loadMatch);
        
        IRValuePtr condResult;
        
        if (arm.pattern == "Some") {
            // Check has_value()
            auto isSomeInst = IRInstruction::make(IROpcode::IsSome);
            isSomeInst->result = createTemp(IRType::makeBool());
            isSomeInst->operands.push_back(currentMatchVal);
            condResult = emit(isSomeInst);
        } else if (arm.pattern == "None") {
            // Check !has_value()
            auto isNoneInst = IRInstruction::make(IROpcode::IsNone);
            isNoneInst->result = createTemp(IRType::makeBool());
            isNoneInst->operands.push_back(currentMatchVal);
            condResult = emit(isNoneInst);
        } else {
            // Value comparison
            auto patternVal = createConst(arm.pattern);
            auto eqInst = IRInstruction::make(IROpcode::Eq);
            eqInst->result = createTemp(IRType::makeBool());
            eqInst->operands.push_back(currentMatchVal);
            eqInst->operands.push_back(patternVal);
            condResult = emit(eqInst);
        }
        
        emitCondBranch(condResult, armBodyBlock, nextBlock);
        
        // Body block
        setInsertPoint(armBodyBlock);
        currentFunction->addBlock(armBodyBlock);
        pushScope();
        
        // Handle binding variable for Some(x)
        if (!arm.bindVar.empty() && arm.pattern == "Some") {
            // Unwrap the optional value
            auto loadMatchBind = IRInstruction::make(IROpcode::Load);
            loadMatchBind->result = createTemp(matchValue->type);
            loadMatchBind->operands.push_back(matchValuePtr);
            auto matchValBind = emit(loadMatchBind);
            
            IRTypePtr innerType = IRType::makeAuto();
            if (matchValue->type && matchValue->type->kind == IRTypeKind::Optional) {
                innerType = matchValue->type->elementType;
            }
            
            auto unwrapInst = IRInstruction::make(IROpcode::Unwrap);
            unwrapInst->result = createTemp(innerType);
            unwrapInst->operands.push_back(matchValBind);
            auto unwrappedVal = emit(unwrapInst);
            
            // Declare binding variable
            auto bindPtr = createTemp(IRType::makePointer(innerType));
            auto bindAlloca = IRInstruction::make(IROpcode::Alloca, bindPtr);
            bindAlloca->type = innerType;
            bindAlloca->stringData = arm.bindVar;
            emit(bindAlloca);
            
            auto storeBind = IRInstruction::make(IROpcode::Store);
            storeBind->operands.push_back(unwrappedVal);
            storeBind->operands.push_back(bindPtr);
            emit(storeBind);
            
            declareVariable(arm.bindVar, bindPtr);
        }
        
        // Build arm body
        for (const auto& stmt : arm.body) {
            buildStmt(stmt);
        }
        
        popScope();
        
        if (!currentBlock->isTerminated()) {
            emitBranch(mergeBlock);
        }
        
        prevFalseBlock = isLast ? nullptr : nextBlock;
        if (!isLast) {
            currentFunction->addBlock(nextBlock);
        }
    }
    
    // Continue in merge block
    setInsertPoint(mergeBlock);
    currentFunction->addBlock(mergeBlock);
}

void IRBuilder::buildBlockStmt(const BlockStmt& block) {
    pushScope();
    for (const auto& stmt : block.stmts) {
        buildStmt(stmt);
    }
    popScope();
}

void IRBuilder::buildCppStmt(const CppStmt& cpp) {
    // Emit inline C++ as a special instruction
    auto inst = IRInstruction::make(IROpcode::CppInline);
    inst->stringData = cpp.code;
    emit(inst);
}

} // namespace IR
