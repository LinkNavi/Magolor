// codegen_llvm_exprs.cpp - LLVM IR Code Generator for Magolor
// Part 3: Expression Generation

#include "codegen_llvm.hpp"
#include <sstream>

// =============================================================================
// Expression Generation
// =============================================================================

llvm::Value* LLVMCodeGen::genExpr(const ExprPtr& expr) {
    return std::visit([this, &expr](auto&& e) -> llvm::Value* {
        using T = std::decay_t<decltype(e)>;
        if constexpr (std::is_same_v<T, IntLitExpr>) {
            return genIntLit(e);
        } else if constexpr (std::is_same_v<T, FloatLitExpr>) {
            return genFloatLit(e);
        } else if constexpr (std::is_same_v<T, StringLitExpr>) {
            if (e.interpolated) {
                return genInterpolatedString(e);
            }
            return genStringLit(e);
        } else if constexpr (std::is_same_v<T, BoolLitExpr>) {
            return genBoolLit(e);
        } else if constexpr (std::is_same_v<T, IdentExpr>) {
            return genIdent(e);
        } else if constexpr (std::is_same_v<T, BinaryExpr>) {
            return genBinary(e);
        } else if constexpr (std::is_same_v<T, UnaryExpr>) {
            return genUnary(e);
        } else if constexpr (std::is_same_v<T, CallExpr>) {
            return genCall(e);
        } else if constexpr (std::is_same_v<T, MemberExpr>) {
            return genMember(e);
        } else if constexpr (std::is_same_v<T, IndexExpr>) {
            return genIndex(e);
        } else if constexpr (std::is_same_v<T, AssignExpr>) {
            return genAssign(e);
        } else if constexpr (std::is_same_v<T, LambdaExpr>) {
            return genLambda(e);
        } else if constexpr (std::is_same_v<T, NewExpr>) {
            return genNew(e);
        } else if constexpr (std::is_same_v<T, SomeExpr>) {
            return genSome(e);
        } else if constexpr (std::is_same_v<T, NoneExpr>) {
            return genNone(e);
        } else if constexpr (std::is_same_v<T, ThisExpr>) {
            return genThis(e);
        } else if constexpr (std::is_same_v<T, ArrayExpr>) {
            return genArray(e);
        } else {
            emitError("Unknown expression type");
            return llvm::Constant::getNullValue(llvm::Type::getInt64Ty(*context));
        }
    }, expr->data);
}

llvm::Value* LLVMCodeGen::genIntLit(const IntLitExpr& e) {
    return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), e.value);
}

llvm::Value* LLVMCodeGen::genFloatLit(const FloatLitExpr& e) {
    return llvm::ConstantFP::get(llvm::Type::getDoubleTy(*context), e.value);
}

llvm::Value* LLVMCodeGen::genStringLit(const StringLitExpr& e) {
    return createString(e.value);
}

llvm::Value* LLVMCodeGen::genBoolLit(const BoolLitExpr& e) {
    return llvm::ConstantInt::get(llvm::Type::getInt1Ty(*context), e.value ? 1 : 0);
}

llvm::Value* LLVMCodeGen::genIdent(const IdentExpr& e) {
    // Check local variables
    if (auto* var = lookupVar(e.name)) {
        return builder->CreateLoad(var->type, var->alloca, e.name);
    }
    
    // Check for global functions
    if (auto* fn = module->getFunction(e.name)) {
        return fn;
    }
    
    // Check for class static members
    auto* gv = module->getGlobalVariable(e.name);
    if (gv) {
        return builder->CreateLoad(gv->getValueType(), gv);
    }
    
    // Check if it's a stdlib module reference (Std)
    if (e.name == "Std") {
        // Return a null placeholder - actual resolution happens in genCall/genMember
        return llvm::Constant::getNullValue(llvm::PointerType::get(*context, 0));
    }
    
    emitError("Unknown identifier: " + e.name);
    return llvm::Constant::getNullValue(llvm::Type::getInt64Ty(*context));
}

llvm::Value* LLVMCodeGen::genBinary(const BinaryExpr& e) {
    // Handle short-circuit evaluation for && and ||
    if (e.op == "&&") {
        auto* lhs = genExpr(e.left);
        if (!lhs->getType()->isIntegerTy(1)) {
            lhs = builder->CreateICmpNE(lhs, 
                llvm::Constant::getNullValue(lhs->getType()));
        }
        
        auto* rhsBB = llvm::BasicBlock::Create(*context, "and.rhs", currentFunction);
        auto* mergeBB = llvm::BasicBlock::Create(*context, "and.merge", currentFunction);
        
        auto* entryBB = builder->GetInsertBlock();
        builder->CreateCondBr(lhs, rhsBB, mergeBB);
        
        builder->SetInsertPoint(rhsBB);
        auto* rhs = genExpr(e.right);
        if (!rhs->getType()->isIntegerTy(1)) {
            rhs = builder->CreateICmpNE(rhs, 
                llvm::Constant::getNullValue(rhs->getType()));
        }
        auto* rhsEndBB = builder->GetInsertBlock();
        builder->CreateBr(mergeBB);
        
        builder->SetInsertPoint(mergeBB);
        auto* phi = builder->CreatePHI(llvm::Type::getInt1Ty(*context), 2);
        phi->addIncoming(llvm::ConstantInt::getFalse(*context), entryBB);
        phi->addIncoming(rhs, rhsEndBB);
        return phi;
    }
    
    if (e.op == "||") {
        auto* lhs = genExpr(e.left);
        if (!lhs->getType()->isIntegerTy(1)) {
            lhs = builder->CreateICmpNE(lhs, 
                llvm::Constant::getNullValue(lhs->getType()));
        }
        
        auto* rhsBB = llvm::BasicBlock::Create(*context, "or.rhs", currentFunction);
        auto* mergeBB = llvm::BasicBlock::Create(*context, "or.merge", currentFunction);
        
        auto* entryBB = builder->GetInsertBlock();
        builder->CreateCondBr(lhs, mergeBB, rhsBB);
        
        builder->SetInsertPoint(rhsBB);
        auto* rhs = genExpr(e.right);
        if (!rhs->getType()->isIntegerTy(1)) {
            rhs = builder->CreateICmpNE(rhs, 
                llvm::Constant::getNullValue(rhs->getType()));
        }
        auto* rhsEndBB = builder->GetInsertBlock();
        builder->CreateBr(mergeBB);
        
        builder->SetInsertPoint(mergeBB);
        auto* phi = builder->CreatePHI(llvm::Type::getInt1Ty(*context), 2);
        phi->addIncoming(llvm::ConstantInt::getTrue(*context), entryBB);
        phi->addIncoming(rhs, rhsEndBB);
        return phi;
    }
    
    auto* left = genExpr(e.left);
    auto* right = genExpr(e.right);
    
    // Determine operation type based on operands
    auto* leftType = left->getType();
    auto* rightType = right->getType();
    
    // String concatenation
    if (leftType->isPointerTy() && rightType->isPointerTy()) {
        if (e.op == "+") {
            return builder->CreateCall(runtime.stringConcat, {left, right});
        }
        return genStringBinary(e.op, left, right);
    }
    
    // Float operations (promote int if needed)
    if (leftType->isDoubleTy() || rightType->isDoubleTy()) {
        if (leftType->isIntegerTy()) {
            left = builder->CreateSIToFP(left, llvm::Type::getDoubleTy(*context));
        }
        if (rightType->isIntegerTy()) {
            right = builder->CreateSIToFP(right, llvm::Type::getDoubleTy(*context));
        }
        return genFloatBinary(e.op, left, right);
    }
    
    // Integer operations
    if (leftType->isIntegerTy() && rightType->isIntegerTy()) {
        // Ensure same width
        if (leftType->getIntegerBitWidth() != rightType->getIntegerBitWidth()) {
            if (leftType->getIntegerBitWidth() < rightType->getIntegerBitWidth()) {
                left = builder->CreateSExt(left, rightType);
            } else {
                right = builder->CreateSExt(right, leftType);
            }
        }
        return genIntBinary(e.op, left, right);
    }
    
    // Pointer comparisons
    if (leftType->isPointerTy() || rightType->isPointerTy()) {
        return genPointerBinary(e.op, left, right);
    }
    
    emitError("Unsupported binary operation types");
    return llvm::Constant::getNullValue(llvm::Type::getInt64Ty(*context));
}

llvm::Value* LLVMCodeGen::genIntBinary(const std::string& op, 
                                       llvm::Value* left, llvm::Value* right) {
    if (op == "+") return builder->CreateAdd(left, right, "addtmp");
    if (op == "-") return builder->CreateSub(left, right, "subtmp");
    if (op == "*") return builder->CreateMul(left, right, "multmp");
    if (op == "/") return builder->CreateSDiv(left, right, "divtmp");
    if (op == "%") return builder->CreateSRem(left, right, "modtmp");
    
    if (op == "==") return builder->CreateICmpEQ(left, right, "eqtmp");
    if (op == "!=") return builder->CreateICmpNE(left, right, "netmp");
    if (op == "<") return builder->CreateICmpSLT(left, right, "lttmp");
    if (op == "<=") return builder->CreateICmpSLE(left, right, "letmp");
    if (op == ">") return builder->CreateICmpSGT(left, right, "gttmp");
    if (op == ">=") return builder->CreateICmpSGE(left, right, "getmp");
    
    if (op == "&") return builder->CreateAnd(left, right, "andtmp");
    if (op == "|") return builder->CreateOr(left, right, "ortmp");
    if (op == "^") return builder->CreateXor(left, right, "xortmp");
    if (op == "<<") return builder->CreateShl(left, right, "shltmp");
    if (op == ">>") return builder->CreateAShr(left, right, "shrtmp");
    
    emitError("Unknown integer binary operator: " + op);
    return llvm::Constant::getNullValue(left->getType());
}

llvm::Value* LLVMCodeGen::genFloatBinary(const std::string& op, 
                                         llvm::Value* left, llvm::Value* right) {
    if (op == "+") return builder->CreateFAdd(left, right, "faddtmp");
    if (op == "-") return builder->CreateFSub(left, right, "fsubtmp");
    if (op == "*") return builder->CreateFMul(left, right, "fmultmp");
    if (op == "/") return builder->CreateFDiv(left, right, "fdivtmp");
    
    if (op == "==") return builder->CreateFCmpOEQ(left, right, "feqtmp");
    if (op == "!=") return builder->CreateFCmpONE(left, right, "fnetmp");
    if (op == "<") return builder->CreateFCmpOLT(left, right, "flttmp");
    if (op == "<=") return builder->CreateFCmpOLE(left, right, "fletmp");
    if (op == ">") return builder->CreateFCmpOGT(left, right, "fgttmp");
    if (op == ">=") return builder->CreateFCmpOGE(left, right, "fgetmp");
    
    emitError("Unknown float binary operator: " + op);
    return llvm::Constant::getNullValue(left->getType());
}

llvm::Value* LLVMCodeGen::genBoolBinary(const std::string& op, 
                                        llvm::Value* left, llvm::Value* right) {
    if (op == "==") return builder->CreateICmpEQ(left, right, "beqtmp");
    if (op == "!=") return builder->CreateICmpNE(left, right, "bnetmp");
    if (op == "&&") return builder->CreateAnd(left, right, "bandtmp");
    if (op == "||") return builder->CreateOr(left, right, "bortmp");
    
    emitError("Unknown bool binary operator: " + op);
    return llvm::Constant::getNullValue(left->getType());
}

llvm::Value* LLVMCodeGen::genStringBinary(const std::string& op, 
                                          llvm::Value* left, llvm::Value* right) {
    if (op == "+") {
        return builder->CreateCall(runtime.stringConcat, {left, right});
    }
    
    auto* strcmp = module->getFunction("strcmp");
    auto* cmpResult = builder->CreateCall(strcmp, {left, right});
    auto* zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0);
    
    if (op == "==") return builder->CreateICmpEQ(cmpResult, zero, "streq");
    if (op == "!=") return builder->CreateICmpNE(cmpResult, zero, "strne");
    if (op == "<") return builder->CreateICmpSLT(cmpResult, zero, "strlt");
    if (op == "<=") return builder->CreateICmpSLE(cmpResult, zero, "strle");
    if (op == ">") return builder->CreateICmpSGT(cmpResult, zero, "strgt");
    if (op == ">=") return builder->CreateICmpSGE(cmpResult, zero, "strge");
    
    emitError("Unknown string binary operator: " + op);
    return llvm::Constant::getNullValue(llvm::Type::getInt1Ty(*context));
}

llvm::Value* LLVMCodeGen::genPointerBinary(const std::string& op, 
                                           llvm::Value* left, llvm::Value* right) {
    if (op == "==") return builder->CreateICmpEQ(left, right, "ptreq");
    if (op == "!=") return builder->CreateICmpNE(left, right, "ptrne");
    
    emitError("Unknown pointer binary operator: " + op);
    return llvm::Constant::getNullValue(llvm::Type::getInt1Ty(*context));
}

llvm::Value* LLVMCodeGen::genUnary(const UnaryExpr& e) {
    auto* operand = genExpr(e.operand);
    
    if (e.op == "-") {
        if (operand->getType()->isDoubleTy()) {
            return builder->CreateFNeg(operand, "fnegtmp");
        }
        return builder->CreateNeg(operand, "negtmp");
    }
    
    if (e.op == "!") {
        if (!operand->getType()->isIntegerTy(1)) {
            operand = builder->CreateICmpNE(operand, 
                llvm::Constant::getNullValue(operand->getType()));
        }
        return builder->CreateNot(operand, "nottmp");
    }
    
    if (e.op == "~") {
        return builder->CreateNot(operand, "bnottmp");
    }
    
    emitError("Unknown unary operator: " + e.op);
    return operand;
}

llvm::Value* LLVMCodeGen::genCall(const CallExpr& e) {
    // Get the callee
    std::string funcName;
    llvm::Value* thisPtr = nullptr;
    
    // Check if it's a member call
    if (auto* memberExpr = std::get_if<MemberExpr>(&e.callee->data)) {
        // Build the full path for namespace calls (e.g., Std.IO.println, IO.print)
        std::vector<std::string> path;
        ExprPtr current = e.callee;
        
        while (auto* member = std::get_if<MemberExpr>(&current->data)) {
            path.push_back(member->member);
            current = member->object;
        }
        
        if (auto* ident = std::get_if<IdentExpr>(&current->data)) {
            path.push_back(ident->name);
        }
        
        std::reverse(path.begin(), path.end());
        
        // Debug output
        std::string fullPath;
        for (size_t i = 0; i < path.size(); i++) {
            if (i > 0) fullPath += ".";
            fullPath += path[i];
        }
        
        static const std::unordered_set<std::string> stdModules = {
            "IO", "Math", "String", "Array", "Map", "File", "Time", 
            "Random", "System", "Network", "Crypto", "Parse", "Option"
        };
        
        std::string moduleName;
        std::string methodName;
        
        // Handle Std.Module.function pattern (e.g., Std.IO.print)
        if (path.size() >= 3 && path[0] == "Std" && stdModules.count(path[1]) > 0) {
            moduleName = path[1];
            methodName = path[2];
        }
        // Handle Module.function pattern (e.g., IO.print)
        else if (path.size() >= 2 && stdModules.count(path[0]) > 0) {
            moduleName = path[0];
            methodName = path[1];
        }
        // Handle Std.function pattern (e.g., Std.print) - default to IO
        else if (path.size() == 2 && path[0] == "Std") {
            moduleName = "IO";
            methodName = path[1];
        }
        
        // If we identified a stdlib call, handle it
        if (!moduleName.empty() && !methodName.empty()) {
            // Generate arguments
            std::vector<llvm::Value*> args;
            for (const auto& arg : e.args) {
                args.push_back(genExpr(arg));
            }
            
            return callStdLibFunction(moduleName, methodName, args);
        }
        
        // Check for method call on an object
        auto objExpr = memberExpr->object;
        thisPtr = genExpr(objExpr);
        funcName = memberExpr->member;
        
        // If it's a class method call
        if (objExpr->type && objExpr->type->kind == Type::CLASS) {
            funcName = getMangledName(objExpr->type->className, memberExpr->member);
        }
    } else if (auto* identExpr = std::get_if<IdentExpr>(&e.callee->data)) {
        funcName = identExpr->name;
    } else {
        // Function pointer or lambda call
        auto* callee = genExpr(e.callee);
        
        std::vector<llvm::Value*> args;
        for (const auto& arg : e.args) {
            args.push_back(genExpr(arg));
        }
        
        auto* fnPtrType = llvm::dyn_cast<llvm::PointerType>(callee->getType());
        if (fnPtrType) {
            auto* fnType = llvm::FunctionType::get(
                llvm::Type::getInt64Ty(*context), 
                std::vector<llvm::Type*>(args.size(), llvm::Type::getInt64Ty(*context)), 
                false
            );
            return builder->CreateCall(fnType, callee, args);
        }
        
        emitError("Cannot call non-function value");
        return llvm::Constant::getNullValue(llvm::Type::getInt64Ty(*context));
    }
    
    // Look up the function
    auto* fn = module->getFunction(funcName);
    if (!fn) {
        // Try with mg_ prefix for runtime functions
        fn = module->getFunction("mg_" + funcName);
    }
    
    if (!fn) {
        emitError("Unknown function: " + funcName);
        return llvm::Constant::getNullValue(llvm::Type::getInt64Ty(*context));
    }
    
    // Generate arguments
    std::vector<llvm::Value*> args;
    
    // Add 'this' pointer for method calls
    if (thisPtr) {
        args.push_back(thisPtr);
    }
    
    for (const auto& arg : e.args) {
        args.push_back(genExpr(arg));
    }
    
    // Cast arguments to match function parameter types
    auto* fnType = fn->getFunctionType();
    for (size_t i = 0; i < args.size() && i < fnType->getNumParams(); i++) {
        auto* paramType = fnType->getParamType(i);
        if (args[i]->getType() != paramType) {
            args[i] = castValue(args[i], paramType);
        }
    }
    
    return builder->CreateCall(fn, args);
}

llvm::Value* LLVMCodeGen::genMember(const MemberExpr& e) {
    // Check for namespace access (e.g., Math.PI, Std.IO)
    if (auto* ident = std::get_if<IdentExpr>(&e.object->data)) {
        static const std::unordered_set<std::string> stdModules = {
            "IO", "Math", "String", "Array", "Map", "File", "Time", 
            "Random", "System", "Network", "Crypto", "Parse", "Option"
        };
        
        // Handle Std.Module pattern - return a placeholder for the module
        if (ident->name == "Std" && stdModules.count(e.member) > 0) {
            // This is Std.IO, Std.Math, etc - return placeholder
            // The actual function call will be resolved in genCall
            return llvm::Constant::getNullValue(llvm::PointerType::get(*context, 0));
        }
        
        if (stdModules.count(ident->name) > 0) {
            // Standard library constant or function reference
            std::string globalName = ident->name + "_" + e.member;
            
            // Check for known constants
            if (ident->name == "Math") {
                if (e.member == "PI") {
                    return llvm::ConstantFP::get(llvm::Type::getDoubleTy(*context), 
                                                 3.14159265358979323846);
                }
                if (e.member == "E") {
                    return llvm::ConstantFP::get(llvm::Type::getDoubleTy(*context), 
                                                 2.71828182845904523536);
                }
            }
            
            // Return function reference
            auto* fn = module->getFunction(globalName);
            if (fn) return fn;
            
            // Try with mg_ prefix
            fn = module->getFunction("mg_" + globalName);
            if (fn) return fn;
            
            // Return placeholder - will be resolved in genCall
            return llvm::Constant::getNullValue(llvm::PointerType::get(*context, 0));
        }
        
        // Check for class static member
        auto* gv = module->getGlobalVariable(ident->name + "." + e.member);
        if (gv) {
            return builder->CreateLoad(gv->getValueType(), gv);
        }
    }
    
    // Object field access
    auto* obj = genExpr(e.object);
    
    // Get class layout
    std::string className;
    if (e.object->type && e.object->type->kind == Type::CLASS) {
        className = e.object->type->className;
    }
    
    if (!className.empty()) {
        auto it = classLayouts.find(className);
        if (it != classLayouts.end()) {
            auto& layout = it->second;
            auto fieldIt = layout.fieldIndices.find(e.member);
            if (fieldIt != layout.fieldIndices.end()) {
                auto* fieldPtr = builder->CreateStructGEP(layout.structType, obj, 
                                                          fieldIt->second);
                auto* fieldType = layout.structType->getElementType(fieldIt->second);
                return builder->CreateLoad(fieldType, fieldPtr);
            }
        }
    }
    
    // Try as struct field with opaque pointer
    emitError("Unknown member: " + e.member);
    return llvm::Constant::getNullValue(llvm::Type::getInt64Ty(*context));
}

llvm::Value* LLVMCodeGen::genIndex(const IndexExpr& e) {
    auto* obj = genExpr(e.object);
    auto* idx = genExpr(e.index);
    
    // Ensure index is i64
    if (!idx->getType()->isIntegerTy(64)) {
        idx = builder->CreateSExt(idx, llvm::Type::getInt64Ty(*context));
    }
    
    // For strings, return character at index
    if (obj->getType()->isPointerTy()) {
        auto* elemPtr = builder->CreateGEP(llvm::Type::getInt8Ty(*context), obj, idx);
        return builder->CreateLoad(llvm::Type::getInt8Ty(*context), elemPtr);
    }
    
    emitError("Cannot index non-array type");
    return llvm::Constant::getNullValue(llvm::Type::getInt64Ty(*context));
}

llvm::Value* LLVMCodeGen::genAssign(const AssignExpr& e) {
    auto* value = genExpr(e.value);
    
    // Handle identifier assignment
    if (auto* ident = std::get_if<IdentExpr>(&e.target->data)) {
        if (auto* var = lookupVar(ident->name)) {
            if (value->getType() != var->type) {
                value = castValue(value, var->type);
            }
            builder->CreateStore(value, var->alloca);
            return value;
        }
    }
    
    // Handle member assignment
    if (auto* member = std::get_if<MemberExpr>(&e.target->data)) {
        auto* obj = genExpr(member->object);
        
        std::string className;
        if (member->object->type && member->object->type->kind == Type::CLASS) {
            className = member->object->type->className;
        }
        
        if (!className.empty()) {
            auto it = classLayouts.find(className);
            if (it != classLayouts.end()) {
                auto& layout = it->second;
                auto fieldIt = layout.fieldIndices.find(member->member);
                if (fieldIt != layout.fieldIndices.end()) {
                    auto* fieldPtr = builder->CreateStructGEP(layout.structType, obj, 
                                                              fieldIt->second);
                    builder->CreateStore(value, fieldPtr);
                    return value;
                }
            }
        }
    }
    
    // Handle index assignment
    if (auto* index = std::get_if<IndexExpr>(&e.target->data)) {
        auto* obj = genExpr(index->object);
        auto* idx = genExpr(index->index);
        
        if (!idx->getType()->isIntegerTy(64)) {
            idx = builder->CreateSExt(idx, llvm::Type::getInt64Ty(*context));
        }
        
        auto* elemPtr = builder->CreateGEP(llvm::Type::getInt8Ty(*context), obj, idx);
        auto* castPtr = builder->CreateBitCast(elemPtr, 
            llvm::PointerType::get(value->getType(), 0));
        builder->CreateStore(value, castPtr);
        return value;
    }
    
    emitError("Invalid assignment target");
    return value;
}

llvm::Value* LLVMCodeGen::genLambda(const LambdaExpr& e) {
    // Create a unique name for the lambda function
    std::string lambdaName = "__lambda_" + std::to_string(tempCounter++);
    
    // Build parameter types
    std::vector<llvm::Type*> paramTypes;
    for (const auto& param : e.params) {
        paramTypes.push_back(toLLVMType(param.type));
    }
    
    auto* retType = e.returnType ? toLLVMType(e.returnType) : 
                                   llvm::Type::getVoidTy(*context);
    auto* fnType = llvm::FunctionType::get(retType, paramTypes, false);
    
    auto* fn = llvm::Function::Create(fnType, llvm::Function::InternalLinkage, 
                                      lambdaName, module.get());
    
    // Save current state
    auto* savedFn = currentFunction;
    auto* savedBlock = builder->GetInsertBlock();
    auto savedThis = currentThis;
    
    // Create lambda body
    auto* entry = llvm::BasicBlock::Create(*context, "entry", fn);
    builder->SetInsertPoint(entry);
    currentFunction = fn;
    
    pushScope();
    
    // Declare parameters
    size_t idx = 0;
    for (const auto& param : e.params) {
        auto* arg = fn->getArg(idx);
        arg->setName(param.name);
        auto* alloca = createEntryBlockAlloca(fn, param.name, arg->getType());
        builder->CreateStore(arg, alloca);
        declareVar(param.name, alloca, arg->getType(), param.type, false);
        idx++;
    }
    
    // Generate body
    for (const auto& stmt : e.body) {
        genStmt(stmt);
    }
    
    // Add implicit return if needed
    if (!builder->GetInsertBlock()->getTerminator()) {
        if (retType->isVoidTy()) {
            builder->CreateRetVoid();
        } else {
            builder->CreateRet(getDefaultValue(retType));
        }
    }
    
    popScope();
    
    // Restore state
    currentFunction = savedFn;
    currentThis = savedThis;
    builder->SetInsertPoint(savedBlock);
    
    return fn;
}

llvm::Value* LLVMCodeGen::genNew(const NewExpr& e) {
    std::string className = e.className;
    
    // Handle qualified names (e.g., Module.ClassName)
    size_t dotPos = className.find('.');
    if (dotPos != std::string::npos) {
        // For now, just use the class name part
        className = className.substr(dotPos + 1);
    }
    
    // Generate arguments
    std::vector<llvm::Value*> args;
    for (const auto& arg : e.args) {
        args.push_back(genExpr(arg));
    }
    
    return createClassInstance(className, args);
}

llvm::Value* LLVMCodeGen::genSome(const SomeExpr& e) {
    auto* value = genExpr(e.value);
    
    // Create Option struct { i1 has_value, T value }
    auto* boolTy = llvm::Type::getInt1Ty(*context);
    auto* valType = value->getType();
    
    auto* optionTy = llvm::StructType::get(*context, {boolTy, valType});
    
    // Allocate and initialize
    auto* alloca = createEntryBlockAlloca(currentFunction, "__some", optionTy);
    
    auto* hasValPtr = builder->CreateStructGEP(optionTy, alloca, 0);
    builder->CreateStore(llvm::ConstantInt::getTrue(*context), hasValPtr);
    
    auto* valPtr = builder->CreateStructGEP(optionTy, alloca, 1);
    builder->CreateStore(value, valPtr);
    
    return builder->CreateLoad(optionTy, alloca);
}

llvm::Value* LLVMCodeGen::genNone(const NoneExpr& e) {
    // Create Option with has_value = false
    auto* boolTy = llvm::Type::getInt1Ty(*context);
    auto* i64Ty = llvm::Type::getInt64Ty(*context);
    
    auto* optionTy = llvm::StructType::get(*context, {boolTy, i64Ty});
    
    auto* alloca = createEntryBlockAlloca(currentFunction, "__none", optionTy);
    
    auto* hasValPtr = builder->CreateStructGEP(optionTy, alloca, 0);
    builder->CreateStore(llvm::ConstantInt::getFalse(*context), hasValPtr);
    
    auto* valPtr = builder->CreateStructGEP(optionTy, alloca, 1);
    builder->CreateStore(llvm::ConstantInt::get(i64Ty, 0), valPtr);
    
    return builder->CreateLoad(optionTy, alloca);
}

llvm::Value* LLVMCodeGen::genThis(const ThisExpr& e) {
    if (!currentThis) {
        emitError("'this' used outside of method");
        return llvm::Constant::getNullValue(
            llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0));
    }
    return currentThis;
}

llvm::Value* LLVMCodeGen::genArray(const ArrayExpr& e) {
    auto* i64Ty = llvm::Type::getInt64Ty(*context);
    auto* i8PtrTy = llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0);
    
    // Determine element type
    llvm::Type* elemType = i64Ty;  // Default
    if (!e.elements.empty() && e.elements[0]->type) {
        elemType = toLLVMType(e.elements[0]->type);
    }
    
    size_t elemSize = module->getDataLayout().getTypeAllocSize(elemType);
    size_t numElements = e.elements.size();
    
    // Allocate array structure: { data*, length, capacity }
    auto* arrayStructTy = llvm::StructType::get(*context, {i8PtrTy, i64Ty, i64Ty});
    auto* arrayAlloca = createEntryBlockAlloca(currentFunction, "__array", arrayStructTy);
    
    // Allocate data buffer
    auto* dataSize = llvm::ConstantInt::get(i64Ty, elemSize * numElements);
    auto* data = builder->CreateCall(runtime.mallocFunc, {dataSize});
    
    // Store data pointer
    auto* dataPtr = builder->CreateStructGEP(arrayStructTy, arrayAlloca, 0);
    builder->CreateStore(data, dataPtr);
    
    // Store length
    auto* lenPtr = builder->CreateStructGEP(arrayStructTy, arrayAlloca, 1);
    builder->CreateStore(llvm::ConstantInt::get(i64Ty, numElements), lenPtr);
    
    // Store capacity
    auto* capPtr = builder->CreateStructGEP(arrayStructTy, arrayAlloca, 2);
    builder->CreateStore(llvm::ConstantInt::get(i64Ty, numElements), capPtr);
    
    // Initialize elements
    for (size_t i = 0; i < e.elements.size(); i++) {
        auto* elem = genExpr(e.elements[i]);
        auto* offset = llvm::ConstantInt::get(i64Ty, i * elemSize);
        auto* elemPtr = builder->CreateGEP(llvm::Type::getInt8Ty(*context), data, offset);
        auto* castPtr = builder->CreateBitCast(elemPtr, llvm::PointerType::get(elemType, 0));
        
        if (elem->getType() != elemType) {
            elem = castValue(elem, elemType);
        }
        builder->CreateStore(elem, castPtr);
    }
    
    return arrayAlloca;
}

llvm::Value* LLVMCodeGen::genInterpolatedString(const StringLitExpr& e) {
    // Parse interpolated string and build concatenated result
    const std::string& s = e.value;
    std::vector<llvm::Value*> parts;
    std::string current;
    size_t i = 0;
    
    while (i < s.size()) {
        if (s[i] == '{') {
            // Emit current literal part
            if (!current.empty()) {
                parts.push_back(createString(current));
                current.clear();
            }
            
            // Extract variable name
            i++;
            std::string varName;
            while (i < s.size() && s[i] != '}') {
                varName += s[i++];
            }
            i++;  // Skip '}'
            
            // Get variable value and convert to string
            if (auto* var = lookupVar(varName)) {
                auto* val = builder->CreateLoad(var->type, var->alloca);
                
                // Convert the value to string based on its type
                llvm::Value* strVal = nullptr;
                if (var->type->isIntegerTy(64)) {
                    strVal = builder->CreateCall(runtime.intToString, {val});
                } else if (var->type->isDoubleTy()) {
                    strVal = builder->CreateCall(runtime.floatToString, {val});
                } else if (var->type->isIntegerTy(1)) {
                    strVal = builder->CreateCall(runtime.boolToString, {val});
                } else if (var->type->isPointerTy()) {
                    // Already a string
                    strVal = val;
                } else {
                    strVal = createString("<unknown type>");
                }
                
                parts.push_back(strVal);
            } else {
                parts.push_back(createString("<undefined>"));
            }
        } else {
            current += s[i++];
        }
    }
    
    if (!current.empty()) {
        parts.push_back(createString(current));
    }
    
    // Concatenate all parts
    if (parts.empty()) {
        return createString("");
    }
    
    llvm::Value* result = parts[0];
    for (size_t j = 1; j < parts.size(); j++) {
        result = builder->CreateCall(runtime.stringConcat, {result, parts[j]});
    }
    
    return result;
}
