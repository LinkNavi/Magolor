#include "ir_builder.hpp"
#include <variant>
#include <sstream>

namespace IR {

// ============================================================================
// Expression builders
// ============================================================================

IRValuePtr IRBuilder::buildExpr(const ExprPtr& expr) {
    return std::visit([this, &expr](auto&& e) -> IRValuePtr {
        using T = std::decay_t<decltype(e)>;
        if constexpr (std::is_same_v<T, IntLitExpr>) {
            return buildIntLit(e);
        } else if constexpr (std::is_same_v<T, FloatLitExpr>) {
            return buildFloatLit(e);
        } else if constexpr (std::is_same_v<T, StringLitExpr>) {
            return buildStringLit(e);
        } else if constexpr (std::is_same_v<T, BoolLitExpr>) {
            return buildBoolLit(e);
        } else if constexpr (std::is_same_v<T, IdentExpr>) {
            return buildIdent(e);
        } else if constexpr (std::is_same_v<T, BinaryExpr>) {
            return buildBinary(e);
        } else if constexpr (std::is_same_v<T, UnaryExpr>) {
            return buildUnary(e);
        } else if constexpr (std::is_same_v<T, CallExpr>) {
            return buildCall(e);
        } else if constexpr (std::is_same_v<T, MemberExpr>) {
            return buildMember(e);
        } else if constexpr (std::is_same_v<T, IndexExpr>) {
            return buildIndex(e);
        } else if constexpr (std::is_same_v<T, AssignExpr>) {
            return buildAssign(e);
        } else if constexpr (std::is_same_v<T, LambdaExpr>) {
            return buildLambda(e);
        } else if constexpr (std::is_same_v<T, NewExpr>) {
            return buildNew(e);
        } else if constexpr (std::is_same_v<T, SomeExpr>) {
            return buildSome(e);
        } else if constexpr (std::is_same_v<T, NoneExpr>) {
            return buildNone(e);
        } else if constexpr (std::is_same_v<T, ThisExpr>) {
            return buildThis(e);
        } else if constexpr (std::is_same_v<T, ArrayExpr>) {
            return buildArray(e);
        } else {
            return nullptr;
        }
    }, expr->data);
}

IRValuePtr IRBuilder::buildIntLit(const IntLitExpr& lit) {
    return createConst(static_cast<int64_t>(lit.value));
}

IRValuePtr IRBuilder::buildFloatLit(const FloatLitExpr& lit) {
    return createConst(lit.value);
}

IRValuePtr IRBuilder::buildStringLit(const StringLitExpr& lit) {
    if (lit.interpolated) {
        return buildInterpolatedString(lit.value);
    }
    return createConst(lit.value);
}

IRValuePtr IRBuilder::buildBoolLit(const BoolLitExpr& lit) {
    return createConst(lit.value);
}

IRValuePtr IRBuilder::buildIdent(const IdentExpr& ident) {
    // Look up variable in scope
    IRValuePtr varPtr = lookupVariable(ident.name);
    if (varPtr) {
        // Load the value from the variable pointer
        auto loadInst = IRInstruction::make(IROpcode::Load);
        
        // Determine the type to load
        IRTypePtr loadType = IRType::makeAuto();
        if (varPtr->type && varPtr->type->kind == IRTypeKind::Pointer) {
            loadType = varPtr->type->elementType;
        }
        
        loadInst->result = createTemp(loadType);
        loadInst->operands.push_back(varPtr);
        return emit(loadInst);
    }
    
    // Could be a global or class reference
    // Return as a global value reference
    return IRValue::makeGlobal(ident.name, IRType::makeAuto());
}

IRValuePtr IRBuilder::buildBinary(const BinaryExpr& binary) {
    IRValuePtr left = buildExpr(binary.left);
    IRValuePtr right = buildExpr(binary.right);
    
    // Determine result type
    IRTypePtr resultType;
    IROpcode opcode = binaryOpToOpcode(binary.op, left->type);
    
    // Comparison operators always return bool
    if (opcode == IROpcode::Eq || opcode == IROpcode::Ne ||
        opcode == IROpcode::Lt || opcode == IROpcode::Le ||
        opcode == IROpcode::Gt || opcode == IROpcode::Ge ||
        opcode == IROpcode::And || opcode == IROpcode::Or) {
        resultType = IRType::makeBool();
    } else if (binary.op == "+" && left->type && 
               left->type->kind == IRTypeKind::String) {
        // String concatenation
        resultType = IRType::makeString();
        opcode = IROpcode::StringConcat;
    } else {
        // Use left operand type for arithmetic
        resultType = left->type ? left->type : IRType::makeAuto();
    }
    
    auto inst = IRInstruction::make(opcode);
    inst->result = createTemp(resultType);
    inst->operands.push_back(left);
    inst->operands.push_back(right);
    return emit(inst);
}

IRValuePtr IRBuilder::buildUnary(const UnaryExpr& unary) {
    IRValuePtr operand = buildExpr(unary.operand);
    
    IROpcode opcode;
    if (unary.op == "-") {
        opcode = IROpcode::Neg;
    } else if (unary.op == "!") {
        opcode = IROpcode::Not;
    } else {
        opcode = IROpcode::Nop;
    }
    
    IRTypePtr resultType = operand->type;
    if (opcode == IROpcode::Not) {
        resultType = IRType::makeBool();
    }
    
    auto inst = IRInstruction::make(opcode);
    inst->result = createTemp(resultType);
    inst->operands.push_back(operand);
    return emit(inst);
}

IRValuePtr IRBuilder::buildCall(const CallExpr& call) {
    // Check if callee is a member expression (method call)
    if (auto* memberExpr = std::get_if<MemberExpr>(&call.callee->data)) {
        // Check for std module calls
        if (auto* objIdent = std::get_if<IdentExpr>(&memberExpr->object->data)) {
            if (isStdModule(objIdent->name)) {
                // Static call to std module
                auto inst = IRInstruction::make(IROpcode::CallStatic);
                inst->className = objIdent->name;
                inst->methodName = memberExpr->member;
                inst->result = createTemp(IRType::makeAuto());
                
                for (const auto& arg : call.args) {
                    inst->operands.push_back(buildExpr(arg));
                }
                
                return emit(inst);
            }
            
            if (isClassName(objIdent->name)) {
                // Static method call on a class
                auto inst = IRInstruction::make(IROpcode::CallStatic);
                inst->className = objIdent->name;
                inst->methodName = memberExpr->member;
                inst->result = createTemp(IRType::makeAuto());
                
                for (const auto& arg : call.args) {
                    inst->operands.push_back(buildExpr(arg));
                }
                
                return emit(inst);
            }
        }
        
        // Check for namespace path
        if (isNamespacePath(memberExpr->object)) {
            auto path = extractNamespacePath(memberExpr->object);
            std::string nsPath;
            for (const auto& p : path) {
                if (!nsPath.empty()) nsPath += "::";
                nsPath += p;
            }
            
            auto inst = IRInstruction::make(IROpcode::CallStatic);
            inst->className = nsPath;
            inst->methodName = memberExpr->member;
            inst->result = createTemp(IRType::makeAuto());
            
            for (const auto& arg : call.args) {
                inst->operands.push_back(buildExpr(arg));
            }
            
            return emit(inst);
        }
        
        // Check for this->method()
        if (std::holds_alternative<ThisExpr>(memberExpr->object->data)) {
            auto inst = IRInstruction::make(IROpcode::CallMethod);
            inst->methodName = memberExpr->member;
            inst->result = createTemp(IRType::makeAuto());
            
            // Add 'this' as first operand
            auto thisInst = IRInstruction::make(IROpcode::This);
            thisInst->result = createTemp(IRType::makePointer(IRType::makeClass(currentClassName)));
            thisInst->className = currentClassName;
            auto thisVal = emit(thisInst);
            inst->operands.push_back(thisVal);
            
            for (const auto& arg : call.args) {
                inst->operands.push_back(buildExpr(arg));
            }
            
            return emit(inst);
        }
        
        // Regular method call on an object
        IRValuePtr object = buildExpr(memberExpr->object);
        
        auto inst = IRInstruction::make(IROpcode::CallMethod);
        inst->methodName = memberExpr->member;
        inst->result = createTemp(IRType::makeAuto());
        inst->operands.push_back(object);
        
        for (const auto& arg : call.args) {
            inst->operands.push_back(buildExpr(arg));
        }
        
        return emit(inst);
    }
    
    // Simple function call
    if (auto* identExpr = std::get_if<IdentExpr>(&call.callee->data)) {
        auto inst = IRInstruction::make(IROpcode::Call);
        inst->methodName = identExpr->name;
        inst->result = createTemp(IRType::makeAuto());
        
        for (const auto& arg : call.args) {
            inst->operands.push_back(buildExpr(arg));
        }
        
        return emit(inst);
    }
    
    // Lambda or other callable
    IRValuePtr callee = buildExpr(call.callee);
    
    auto inst = IRInstruction::make(IROpcode::Call);
    inst->result = createTemp(IRType::makeAuto());
    inst->operands.push_back(callee);
    
    for (const auto& arg : call.args) {
        inst->operands.push_back(buildExpr(arg));
    }
    
    return emit(inst);
}

IRValuePtr IRBuilder::buildMember(const MemberExpr& member) {
    // Check for std module access
    if (auto* objIdent = std::get_if<IdentExpr>(&member.object->data)) {
        if (isStdModule(objIdent->name)) {
            // Return a reference to the static member
            auto result = IRValue::makeGlobal(
                objIdent->name + "::" + member.member,
                IRType::makeAuto()
            );
            return result;
        }
        
        if (isClassName(objIdent->name)) {
            // Static field access
            auto inst = IRInstruction::make(IROpcode::GetField);
            inst->className = objIdent->name;
            inst->stringData = member.member;
            inst->result = createTemp(IRType::makeAuto());
            // No object operand for static access
            return emit(inst);
        }
    }
    
    // Check for namespace path
    if (isNamespacePath(member.object)) {
        auto path = extractNamespacePath(member.object);
        path.push_back(member.member);
        
        std::string fullPath;
        for (const auto& p : path) {
            if (!fullPath.empty()) fullPath += "::";
            fullPath += p;
        }
        
        return IRValue::makeGlobal(fullPath, IRType::makeAuto());
    }
    
    // Check for this->field
    if (std::holds_alternative<ThisExpr>(member.object->data)) {
        auto thisInst = IRInstruction::make(IROpcode::This);
        thisInst->result = createTemp(IRType::makePointer(IRType::makeClass(currentClassName)));
        thisInst->className = currentClassName;
        auto thisVal = emit(thisInst);
        
        auto getFieldInst = IRInstruction::make(IROpcode::GetField);
        getFieldInst->className = currentClassName;
        getFieldInst->stringData = member.member;
        getFieldInst->result = createTemp(IRType::makeAuto());
        getFieldInst->operands.push_back(thisVal);
        return emit(getFieldInst);
    }
    
    // Regular field access
    IRValuePtr object = buildExpr(member.object);
    
    auto inst = IRInstruction::make(IROpcode::GetField);
    inst->stringData = member.member;
    inst->result = createTemp(IRType::makeAuto());
    inst->operands.push_back(object);
    
    // Try to get class name from object type
    if (object->type && object->type->kind == IRTypeKind::Class) {
        inst->className = object->type->name;
    }
    
    return emit(inst);
}

IRValuePtr IRBuilder::buildIndex(const IndexExpr& index) {
    IRValuePtr object = buildExpr(index.object);
    IRValuePtr indexVal = buildExpr(index.index);
    
    auto inst = IRInstruction::make(IROpcode::ArrayGet);
    
    // Determine element type
    IRTypePtr elemType = IRType::makeAuto();
    if (object->type) {
        if (object->type->kind == IRTypeKind::Array) {
            elemType = object->type->elementType;
        } else if (object->type->kind == IRTypeKind::Map) {
            elemType = object->type->valueType;
        }
    }
    
    inst->result = createTemp(elemType);
    inst->operands.push_back(object);
    inst->operands.push_back(indexVal);
    return emit(inst);
}

IRValuePtr IRBuilder::buildAssign(const AssignExpr& assign) {
    IRValuePtr value = buildExpr(assign.value);
    
    // Determine target type
    if (auto* identTarget = std::get_if<IdentExpr>(&assign.target->data)) {
        // Variable assignment
        IRValuePtr varPtr = lookupVariable(identTarget->name);
        if (varPtr) {
            auto storeInst = IRInstruction::make(IROpcode::Store);
            storeInst->operands.push_back(value);
            storeInst->operands.push_back(varPtr);
            emit(storeInst);
            return value;
        }
    } else if (auto* memberTarget = std::get_if<MemberExpr>(&assign.target->data)) {
        // Field assignment
        if (std::holds_alternative<ThisExpr>(memberTarget->object->data)) {
            // this->field = value
            auto thisInst = IRInstruction::make(IROpcode::This);
            thisInst->result = createTemp(IRType::makePointer(IRType::makeClass(currentClassName)));
            thisInst->className = currentClassName;
            auto thisVal = emit(thisInst);
            
            auto setFieldInst = IRInstruction::make(IROpcode::SetField);
            setFieldInst->className = currentClassName;
            setFieldInst->stringData = memberTarget->member;
            setFieldInst->operands.push_back(thisVal);
            setFieldInst->operands.push_back(value);
            emit(setFieldInst);
            return value;
        }
        
        IRValuePtr object = buildExpr(memberTarget->object);
        
        auto setFieldInst = IRInstruction::make(IROpcode::SetField);
        setFieldInst->stringData = memberTarget->member;
        setFieldInst->operands.push_back(object);
        setFieldInst->operands.push_back(value);
        
        if (object->type && object->type->kind == IRTypeKind::Class) {
            setFieldInst->className = object->type->name;
        }
        
        emit(setFieldInst);
        return value;
    } else if (auto* indexTarget = std::get_if<IndexExpr>(&assign.target->data)) {
        // Array/map element assignment
        IRValuePtr object = buildExpr(indexTarget->object);
        IRValuePtr indexVal = buildExpr(indexTarget->index);
        
        auto setInst = IRInstruction::make(IROpcode::ArraySet);
        setInst->operands.push_back(object);
        setInst->operands.push_back(indexVal);
        setInst->operands.push_back(value);
        emit(setInst);
        return value;
    }
    
    return value;
}

IRValuePtr IRBuilder::buildLambda(const LambdaExpr& lambda) {
    // Create a nested function for the lambda
    auto lambdaFunc = std::make_shared<IRFunction>();
    lambdaFunc->name = "__lambda_" + std::to_string(nextValueId++);
    lambdaFunc->returnType = lambda.returnType ? convertType(lambda.returnType) : IRType::makeAuto();
    
    // Save current context
    auto savedFunction = currentFunction;
    auto savedBlock = currentBlock;
    auto savedClassName = currentClassName;
    
    currentFunction = lambdaFunc;
    currentClassName.clear();
    pushScope();
    
    // Create entry block
    auto entryBlock = createBlock("lambda.entry");
    lambdaFunc->entryBlock = entryBlock;
    lambdaFunc->addBlock(entryBlock);
    setInsertPoint(entryBlock);
    
    // Build parameters
    for (size_t i = 0; i < lambda.params.size(); i++) {
        const auto& p = lambda.params[i];
        auto paramType = convertType(p.type);
        auto paramValue = IRValue::makeParameter(p.name, paramType, nextValueId++);
        
        IRParameter irParam;
        irParam.name = p.name;
        irParam.type = paramType;
        irParam.value = paramValue;
        lambdaFunc->parameters.push_back(irParam);
        
        declareVariable(p.name, paramValue);
    }
    
    // Build lambda body
    for (const auto& stmt : lambda.body) {
        buildStmt(stmt);
    }
    
    // Add implicit return if needed
    if (!currentBlock->isTerminated()) {
        if (lambdaFunc->returnType && lambdaFunc->returnType->kind == IRTypeKind::Void) {
            emitReturn();
        }
    }
    
    popScope();
    
    // Restore context
    currentFunction = savedFunction;
    currentBlock = savedBlock;
    currentClassName = savedClassName;
    setInsertPoint(savedBlock);
    
    // Collect captured variables (all variables from outer scope)
    std::vector<std::string> captures;
    for (const auto& scope : scopes) {
        for (const auto& [name, _] : scope.variables) {
            captures.push_back(name);
        }
    }
    
    // Emit lambda creation instruction
    auto inst = IRInstruction::make(IROpcode::Lambda);
    inst->lambdaFunc = lambdaFunc;
    inst->captureNames = captures;
    
    // Build function type
    std::vector<IRTypePtr> paramTypes;
    for (const auto& p : lambdaFunc->parameters) {
        paramTypes.push_back(p.type);
    }
    inst->type = IRType::makeFunction(lambdaFunc->returnType, paramTypes);
    inst->result = createTemp(inst->type);
    
    return emit(inst);
}

IRValuePtr IRBuilder::buildNew(const NewExpr& newExpr) {
    std::string className = newExpr.className;
    
    // Handle qualified names (Module.ClassName)
    size_t dotPos = className.find('.');
    std::string moduleName;
    std::string simpleClassName = className;
    if (dotPos != std::string::npos) {
        moduleName = className.substr(0, dotPos);
        simpleClassName = className.substr(dotPos + 1);
    }
    
    auto inst = IRInstruction::make(IROpcode::New);
    inst->className = className;
    inst->result = createTemp(IRType::makeClass(simpleClassName));
    
    // Build constructor arguments
    for (const auto& arg : newExpr.args) {
        inst->operands.push_back(buildExpr(arg));
    }
    
    return emit(inst);
}

IRValuePtr IRBuilder::buildSome(const SomeExpr& some) {
    IRValuePtr innerValue = buildExpr(some.value);
    
    auto inst = IRInstruction::make(IROpcode::Some);
    inst->result = createTemp(IRType::makeOptional(innerValue->type));
    inst->operands.push_back(innerValue);
    return emit(inst);
}

IRValuePtr IRBuilder::buildNone(const NoneExpr& none) {
    return createNone();
}

IRValuePtr IRBuilder::buildThis(const ThisExpr& thisExpr) {
    auto inst = IRInstruction::make(IROpcode::This);
    inst->result = createTemp(IRType::makePointer(IRType::makeClass(currentClassName)));
    inst->className = currentClassName;
    return emit(inst);
}

IRValuePtr IRBuilder::buildArray(const ArrayExpr& array) {
    // Determine element type from first element
    IRTypePtr elemType = IRType::makeAuto();
    std::vector<IRValuePtr> elemValues;
    
    for (const auto& elem : array.elements) {
        auto val = buildExpr(elem);
        elemValues.push_back(val);
        if (elemType->kind == IRTypeKind::Auto && val && val->type) {
            elemType = val->type;
        }
    }
    
    // Create array
    auto createInst = IRInstruction::make(IROpcode::ArrayCreate);
    createInst->result = createTemp(IRType::makeArray(elemType));
    createInst->type = elemType;
    createInst->operands = elemValues;
    
    return emit(createInst);
}

IRValuePtr IRBuilder::buildInterpolatedString(const std::string& str) {
    // Parse interpolated string and build concatenation
    std::vector<IRValuePtr> parts;
    std::string current;
    size_t i = 0;
    
    while (i < str.size()) {
        if (str[i] == '{') {
            // Emit current literal part
            if (!current.empty()) {
                parts.push_back(createConst(current));
                current.clear();
            }
            
            // Extract variable name
            i++;
            std::string varName;
            while (i < str.size() && str[i] != '}') {
                varName += str[i++];
            }
            i++; // Skip closing brace
            
            // Look up variable and convert to string
            IRValuePtr varValue = lookupVariable(varName);
            if (varValue) {
                // Load the value
                auto loadInst = IRInstruction::make(IROpcode::Load);
                IRTypePtr loadType = IRType::makeAuto();
                if (varValue->type && varValue->type->kind == IRTypeKind::Pointer) {
                    loadType = varValue->type->elementType;
                }
                loadInst->result = createTemp(loadType);
                loadInst->operands.push_back(varValue);
                auto loadedVal = emit(loadInst);
                
                // Convert to string
                auto toStringInst = IRInstruction::make(IROpcode::ToString);
                toStringInst->result = createTemp(IRType::makeString());
                toStringInst->operands.push_back(loadedVal);
                parts.push_back(emit(toStringInst));
            } else {
                // Assume it's a global reference
                auto globalRef = IRValue::makeGlobal(varName, IRType::makeAuto());
                auto toStringInst = IRInstruction::make(IROpcode::ToString);
                toStringInst->result = createTemp(IRType::makeString());
                toStringInst->operands.push_back(globalRef);
                parts.push_back(emit(toStringInst));
            }
        } else {
            current += str[i++];
        }
    }
    
    // Add final literal part
    if (!current.empty()) {
        parts.push_back(createConst(current));
    }
    
    // Concatenate all parts
    if (parts.empty()) {
        return createConst(std::string(""));
    }
    
    if (parts.size() == 1) {
        return parts[0];
    }
    
    // Build concatenation chain
    IRValuePtr result = parts[0];
    for (size_t j = 1; j < parts.size(); j++) {
        auto concatInst = IRInstruction::make(IROpcode::StringConcat);
        concatInst->result = createTemp(IRType::makeString());
        concatInst->operands.push_back(result);
        concatInst->operands.push_back(parts[j]);
        concatInst->isInterpolated = true;
        result = emit(concatInst);
    }
    
    return result;
}

} // namespace IR
