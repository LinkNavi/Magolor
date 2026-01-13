#include "ir_types.hpp"
#include <sstream>

namespace IR {

// ============================================================================
// IRType Implementation
// ============================================================================

IRTypePtr IRType::makeVoid() {
    auto t = std::make_shared<IRType>();
    t->kind = IRTypeKind::Void;
    return t;
}

IRTypePtr IRType::makeInt64() {
    auto t = std::make_shared<IRType>();
    t->kind = IRTypeKind::Int64;
    return t;
}

IRTypePtr IRType::makeFloat64() {
    auto t = std::make_shared<IRType>();
    t->kind = IRTypeKind::Float64;
    return t;
}

IRTypePtr IRType::makeBool() {
    auto t = std::make_shared<IRType>();
    t->kind = IRTypeKind::Bool;
    return t;
}

IRTypePtr IRType::makeString() {
    auto t = std::make_shared<IRType>();
    t->kind = IRTypeKind::String;
    return t;
}

IRTypePtr IRType::makePointer(IRTypePtr pointee) {
    auto t = std::make_shared<IRType>();
    t->kind = IRTypeKind::Pointer;
    t->elementType = pointee;
    return t;
}

IRTypePtr IRType::makeArray(IRTypePtr element) {
    auto t = std::make_shared<IRType>();
    t->kind = IRTypeKind::Array;
    t->elementType = element;
    return t;
}

IRTypePtr IRType::makeMap(IRTypePtr key, IRTypePtr value) {
    auto t = std::make_shared<IRType>();
    t->kind = IRTypeKind::Map;
    t->keyType = key;
    t->valueType = value;
    return t;
}

IRTypePtr IRType::makeSet(IRTypePtr element) {
    auto t = std::make_shared<IRType>();
    t->kind = IRTypeKind::Set;
    t->elementType = element;
    return t;
}

IRTypePtr IRType::makeOptional(IRTypePtr inner) {
    auto t = std::make_shared<IRType>();
    t->kind = IRTypeKind::Optional;
    t->elementType = inner;
    return t;
}

IRTypePtr IRType::makeFunction(IRTypePtr ret, std::vector<IRTypePtr> params) {
    auto t = std::make_shared<IRType>();
    t->kind = IRTypeKind::Function;
    t->returnType = ret;
    t->paramTypes = std::move(params);
    return t;
}

IRTypePtr IRType::makeClass(const std::string& name) {
    auto t = std::make_shared<IRType>();
    t->kind = IRTypeKind::Class;
    t->name = name;
    return t;
}

IRTypePtr IRType::makeGeneric(const std::string& name, std::vector<IRTypePtr> args) {
    auto t = std::make_shared<IRType>();
    t->kind = IRTypeKind::Generic;
    t->name = name;
    t->genericArgs = std::move(args);
    return t;
}

IRTypePtr IRType::makeAuto() {
    auto t = std::make_shared<IRType>();
    t->kind = IRTypeKind::Auto;
    return t;
}

bool IRType::equals(const IRTypePtr& other) const {
    if (!other) return false;
    if (kind != other->kind) return false;
    
    switch (kind) {
        case IRTypeKind::Void:
        case IRTypeKind::Int64:
        case IRTypeKind::Float64:
        case IRTypeKind::Bool:
        case IRTypeKind::String:
        case IRTypeKind::Auto:
            return true;
            
        case IRTypeKind::Pointer:
        case IRTypeKind::Array:
        case IRTypeKind::Set:
        case IRTypeKind::Optional:
            return elementType && elementType->equals(other->elementType);
            
        case IRTypeKind::Map:
            return keyType && keyType->equals(other->keyType) &&
                   valueType && valueType->equals(other->valueType);
                   
        case IRTypeKind::Function:
            if (!returnType->equals(other->returnType)) return false;
            if (paramTypes.size() != other->paramTypes.size()) return false;
            for (size_t i = 0; i < paramTypes.size(); i++) {
                if (!paramTypes[i]->equals(other->paramTypes[i])) return false;
            }
            return true;
            
        case IRTypeKind::Class:
            return name == other->name;
            
        case IRTypeKind::Generic:
            if (name != other->name) return false;
            if (genericArgs.size() != other->genericArgs.size()) return false;
            for (size_t i = 0; i < genericArgs.size(); i++) {
                if (!genericArgs[i]->equals(other->genericArgs[i])) return false;
            }
            return true;
    }
    return false;
}

std::string IRType::toString() const {
    switch (kind) {
        case IRTypeKind::Void: return "void";
        case IRTypeKind::Int64: return "int64";
        case IRTypeKind::Float64: return "float64";
        case IRTypeKind::Bool: return "bool";
        case IRTypeKind::String: return "string";
        case IRTypeKind::Auto: return "auto";
        
        case IRTypeKind::Pointer:
            return "ptr<" + (elementType ? elementType->toString() : "?") + ">";
            
        case IRTypeKind::Array:
            return "array<" + (elementType ? elementType->toString() : "?") + ">";
            
        case IRTypeKind::Set:
            return "set<" + (elementType ? elementType->toString() : "?") + ">";
            
        case IRTypeKind::Optional:
            return "optional<" + (elementType ? elementType->toString() : "?") + ">";
            
        case IRTypeKind::Map:
            return "map<" + (keyType ? keyType->toString() : "?") + ", " +
                   (valueType ? valueType->toString() : "?") + ">";
                   
        case IRTypeKind::Function: {
            std::string s = "fn(";
            for (size_t i = 0; i < paramTypes.size(); i++) {
                if (i > 0) s += ", ";
                s += paramTypes[i] ? paramTypes[i]->toString() : "?";
            }
            s += ") -> " + (returnType ? returnType->toString() : "?");
            return s;
        }
        
        case IRTypeKind::Class:
            return "class " + name;
            
        case IRTypeKind::Generic: {
            std::string s = name + "<";
            for (size_t i = 0; i < genericArgs.size(); i++) {
                if (i > 0) s += ", ";
                s += genericArgs[i] ? genericArgs[i]->toString() : "?";
            }
            s += ">";
            return s;
        }
    }
    return "unknown";
}

// ============================================================================
// IRValue Implementation
// ============================================================================

static uint64_t globalValueId = 0;

IRValuePtr IRValue::makeConstInt(int64_t val) {
    auto v = std::make_shared<IRValue>();
    v->kind = IRValueKind::Constant;
    v->type = IRType::makeInt64();
    v->id = globalValueId++;
    v->constant = IRConstant{val, false};
    return v;
}

IRValuePtr IRValue::makeConstFloat(double val) {
    auto v = std::make_shared<IRValue>();
    v->kind = IRValueKind::Constant;
    v->type = IRType::makeFloat64();
    v->id = globalValueId++;
    v->constant = IRConstant{val, false};
    return v;
}

IRValuePtr IRValue::makeConstBool(bool val) {
    auto v = std::make_shared<IRValue>();
    v->kind = IRValueKind::Constant;
    v->type = IRType::makeBool();
    v->id = globalValueId++;
    v->constant = IRConstant{val, false};
    return v;
}

IRValuePtr IRValue::makeConstString(const std::string& val) {
    auto v = std::make_shared<IRValue>();
    v->kind = IRValueKind::Constant;
    v->type = IRType::makeString();
    v->id = globalValueId++;
    v->constant = IRConstant{val, false};
    return v;
}

IRValuePtr IRValue::makeConstNone(IRTypePtr optType) {
    auto v = std::make_shared<IRValue>();
    v->kind = IRValueKind::Constant;
    v->type = optType ? optType : IRType::makeOptional(IRType::makeAuto());
    v->id = globalValueId++;
    IRConstant c;
    c.isNone = true;
    v->constant = c;
    return v;
}

IRValuePtr IRValue::makeParameter(const std::string& name, IRTypePtr type, uint64_t id) {
    auto v = std::make_shared<IRValue>();
    v->kind = IRValueKind::Parameter;
    v->type = type;
    v->name = name;
    v->id = id;
    return v;
}

IRValuePtr IRValue::makeTemp(IRTypePtr type, uint64_t id) {
    auto v = std::make_shared<IRValue>();
    v->kind = IRValueKind::Temporary;
    v->type = type;
    v->name = "t" + std::to_string(id);
    v->id = id;
    return v;
}

IRValuePtr IRValue::makeGlobal(const std::string& name, IRTypePtr type) {
    auto v = std::make_shared<IRValue>();
    v->kind = IRValueKind::Global;
    v->type = type;
    v->name = name;
    v->id = globalValueId++;
    return v;
}

// ============================================================================
// IRInstruction Implementation
// ============================================================================

IRInstructionPtr IRInstruction::make(IROpcode op, IRValuePtr result) {
    auto inst = std::make_shared<IRInstruction>();
    inst->opcode = op;
    inst->result = result;
    return inst;
}

// ============================================================================
// IRBasicBlock Implementation
// ============================================================================

bool IRBasicBlock::isTerminated() const {
    if (instructions.empty()) return false;
    auto& last = instructions.back();
    return last->opcode == IROpcode::Branch ||
           last->opcode == IROpcode::CondBranch ||
           last->opcode == IROpcode::Return ||
           last->opcode == IROpcode::Unreachable;
}

void IRBasicBlock::addInstruction(IRInstructionPtr inst) {
    instructions.push_back(inst);
}

IRInstructionPtr IRBasicBlock::getTerminator() const {
    if (instructions.empty()) return nullptr;
    auto& last = instructions.back();
    if (last->opcode == IROpcode::Branch ||
        last->opcode == IROpcode::CondBranch ||
        last->opcode == IROpcode::Return ||
        last->opcode == IROpcode::Unreachable) {
        return last;
    }
    return nullptr;
}

// ============================================================================
// IRFunction Implementation
// ============================================================================

IRBasicBlockPtr IRFunction::createBlock(const std::string& blockName) {
    auto block = std::make_shared<IRBasicBlock>();
    block->name = blockName;
    return block;
}

void IRFunction::addBlock(IRBasicBlockPtr block) {
    blocks.push_back(block);
    if (!entryBlock) {
        entryBlock = block;
    }
}

// ============================================================================
// IRModule Implementation
// ============================================================================

IRFunctionPtr IRModule::findFunction(const std::string& funcName) const {
    for (const auto& fn : functions) {
        if (fn->name == funcName) return fn;
    }
    for (const auto& cls : classes) {
        for (const auto& method : cls->methods) {
            if (method->name == funcName && method->className == cls->name) {
                return method;
            }
        }
    }
    return nullptr;
}

IRClassPtr IRModule::findClass(const std::string& className) const {
    for (const auto& cls : classes) {
        if (cls->name == className) return cls;
    }
    return nullptr;
}

} // namespace IR
