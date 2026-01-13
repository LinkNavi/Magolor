#include "ir_builder.hpp"
#include <variant>
#include <algorithm>
#include <sstream>

namespace IR {

// ============================================================================
// Constructor
// ============================================================================

IRBuilder::IRBuilder() {
    module = std::make_shared<IRModule>();
}

// ============================================================================
// Type conversion from AST types to IR types
// ============================================================================

IRTypePtr IRBuilder::convertType(const TypePtr& astType) {
    if (!astType) return IRType::makeAuto();
    
    switch (astType->kind) {
        case Type::INT:
            return IRType::makeInt64();
        case Type::FLOAT:
            return IRType::makeFloat64();
        case Type::STRING:
            return IRType::makeString();
        case Type::BOOL:
            return IRType::makeBool();
        case Type::VOID:
            return IRType::makeVoid();
        case Type::CLASS:
            return IRType::makeClass(astType->className);
        case Type::OPTION:
            return IRType::makeOptional(convertType(astType->innerType));
        case Type::ARRAY:
            return IRType::makeArray(convertType(astType->innerType));
        case Type::GENERIC: {
            if (astType->className == "Array" && astType->genericArgs.size() == 1) {
                return IRType::makeArray(convertType(astType->genericArgs[0]));
            }
            if (astType->className == "Map" && astType->genericArgs.size() == 2) {
                return IRType::makeMap(
                    convertType(astType->genericArgs[0]),
                    convertType(astType->genericArgs[1])
                );
            }
            if (astType->className == "Set" && astType->genericArgs.size() == 1) {
                return IRType::makeSet(convertType(astType->genericArgs[0]));
            }
            if (astType->className == "Option" && astType->genericArgs.size() == 1) {
                return IRType::makeOptional(convertType(astType->genericArgs[0]));
            }
            std::vector<IRTypePtr> args;
            for (const auto& arg : astType->genericArgs) {
                args.push_back(convertType(arg));
            }
            return IRType::makeGeneric(astType->className, args);
        }
        case Type::FUNCTION: {
            std::vector<IRTypePtr> params;
            for (const auto& p : astType->paramTypes) {
                params.push_back(convertType(p));
            }
            return IRType::makeFunction(convertType(astType->returnType), params);
        }
    }
    return IRType::makeAuto();
}

// ============================================================================
// Value creation helpers
// ============================================================================

IRValuePtr IRBuilder::createTemp(IRTypePtr type) {
    return IRValue::makeTemp(type, nextValueId++);
}

IRValuePtr IRBuilder::createConst(int64_t val) {
    return IRValue::makeConstInt(val);
}

IRValuePtr IRBuilder::createConst(double val) {
    return IRValue::makeConstFloat(val);
}

IRValuePtr IRBuilder::createConst(bool val) {
    return IRValue::makeConstBool(val);
}

IRValuePtr IRBuilder::createConst(const std::string& val) {
    return IRValue::makeConstString(val);
}

IRValuePtr IRBuilder::createNone(IRTypePtr optType) {
    return IRValue::makeConstNone(optType);
}

// ============================================================================
// Instruction emission
// ============================================================================

IRValuePtr IRBuilder::emit(IRInstructionPtr inst) {
    if (currentBlock) {
        currentBlock->addInstruction(inst);
    }
    return inst->result;
}

void IRBuilder::emitBranch(IRBasicBlockPtr target) {
    if (!currentBlock || currentBlock->isTerminated()) return;
    
    auto inst = IRInstruction::make(IROpcode::Branch);
    inst->targetBlock = target;
    emit(inst);
    linkBlocks(currentBlock, target);
}

void IRBuilder::emitCondBranch(IRValuePtr cond, IRBasicBlockPtr trueBlock, IRBasicBlockPtr falseBlock) {
    if (!currentBlock || currentBlock->isTerminated()) return;
    
    auto inst = IRInstruction::make(IROpcode::CondBranch);
    inst->operands.push_back(cond);
    inst->trueBlock = trueBlock;
    inst->falseBlock = falseBlock;
    emit(inst);
    linkBlocks(currentBlock, trueBlock);
    linkBlocks(currentBlock, falseBlock);
}

void IRBuilder::emitReturn(IRValuePtr value) {
    if (!currentBlock || currentBlock->isTerminated()) return;
    
    auto inst = IRInstruction::make(IROpcode::Return);
    if (value) {
        inst->operands.push_back(value);
    }
    emit(inst);
}

// ============================================================================
// Block management
// ============================================================================

IRBasicBlockPtr IRBuilder::createBlock(const std::string& name) {
    auto block = std::make_shared<IRBasicBlock>();
    block->name = name;
    block->parent = currentFunction;
    return block;
}

void IRBuilder::setInsertPoint(IRBasicBlockPtr block) {
    currentBlock = block;
    if (currentFunction && block) {
        // Add block to function if not already present
        auto it = std::find(currentFunction->blocks.begin(), 
                           currentFunction->blocks.end(), block);
        if (it == currentFunction->blocks.end()) {
            currentFunction->addBlock(block);
        }
    }
}

void IRBuilder::linkBlocks(IRBasicBlockPtr from, IRBasicBlockPtr to) {
    if (!from || !to) return;
    
    // Add to successors if not already present
    auto succIt = std::find(from->successors.begin(), from->successors.end(), to);
    if (succIt == from->successors.end()) {
        from->successors.push_back(to);
    }
    
    // Add to predecessors if not already present
    auto predIt = std::find(to->predecessors.begin(), to->predecessors.end(), from);
    if (predIt == to->predecessors.end()) {
        to->predecessors.push_back(from);
    }
}

// ============================================================================
// Scope management
// ============================================================================

void IRBuilder::pushScope() {
    scopes.push_back(Scope{});
}

void IRBuilder::popScope() {
    if (!scopes.empty()) {
        scopes.pop_back();
    }
}

void IRBuilder::declareVariable(const std::string& name, IRValuePtr value) {
    if (!scopes.empty()) {
        scopes.back().variables[name] = value;
    }
    if (currentFunction) {
        currentFunction->locals[name] = value;
    }
}

IRValuePtr IRBuilder::lookupVariable(const std::string& name) {
    // Search from innermost to outermost scope
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto varIt = it->variables.find(name);
        if (varIt != it->variables.end()) {
            return varIt->second;
        }
    }
    return nullptr;
}

// ============================================================================
// Helper methods
// ============================================================================

bool IRBuilder::isClassName(const std::string& name) const {
    return knownClassNames.count(name) > 0;
}

bool IRBuilder::isStdModule(const std::string& name) const {
    static const std::unordered_set<std::string> stdModules = {
        "Crypto", "File", "String", "Array", "Map",
        "Math", "IO", "Parse", "Option", "Time",
        "Random", "System", "Network"
    };
    return stdModules.count(name) > 0;
}

bool IRBuilder::isNamespacePath(const ExprPtr& expr) const {
    ExprPtr root = expr;
    while (auto* nested = std::get_if<MemberExpr>(&root->data)) {
        root = nested->object;
    }
    
    if (auto* rootIdent = std::get_if<IdentExpr>(&root->data)) {
        if (rootIdent->name == "Std" || rootIdent->name == "std") {
            return true;
        }
        if (importedNamespaces.count(rootIdent->name) > 0) {
            return true;
        }
    }
    return false;
}

std::vector<std::string> IRBuilder::extractNamespacePath(const ExprPtr& expr) {
    std::vector<std::string> path;
    ExprPtr current = expr;
    
    while (auto* member = std::get_if<MemberExpr>(&current->data)) {
        path.push_back(member->member);
        current = member->object;
    }
    
    if (auto* ident = std::get_if<IdentExpr>(&current->data)) {
        if (ident->name != "Std" && ident->name != "std") {
            path.push_back(ident->name);
        }
    }
    
    std::reverse(path.begin(), path.end());
    return path;
}

IROpcode IRBuilder::binaryOpToOpcode(const std::string& op, IRTypePtr leftType) {
    if (op == "+") return IROpcode::Add;
    if (op == "-") return IROpcode::Sub;
    if (op == "*") return IROpcode::Mul;
    if (op == "/") return IROpcode::Div;
    if (op == "%") return IROpcode::Mod;
    if (op == "==") return IROpcode::Eq;
    if (op == "!=") return IROpcode::Ne;
    if (op == "<") return IROpcode::Lt;
    if (op == "<=") return IROpcode::Le;
    if (op == ">") return IROpcode::Gt;
    if (op == ">=") return IROpcode::Ge;
    if (op == "&&") return IROpcode::And;
    if (op == "||") return IROpcode::Or;
    return IROpcode::Nop;
}

// ============================================================================
// Main build entry point
// ============================================================================

IRModulePtr IRBuilder::build(const Program& prog) {
    module = std::make_shared<IRModule>();
    module->name = prog.moduleName.empty() ? "main" : prog.moduleName;
    
    // Collect class names first
    for (const auto& cls : prog.classes) {
        knownClassNames.insert(cls.name);
    }
    
    // Build metadata
    buildUsings(prog.usings);
    buildCImports(prog.cimports);
    buildCppHeaders(prog.cppHeaders);
    buildLinkDecls(prog.linkDecls);
    buildIncludeDecls(prog.includeDecls);
    
    // Build classes
    for (const auto& cls : prog.classes) {
        auto irClass = buildClass(cls);
        module->classes.push_back(irClass);
    }
    
    // Build top-level functions
    for (const auto& fn : prog.functions) {
        auto irFunc = buildFunction(fn);
        module->functions.push_back(irFunc);
    }
    
    return module;
}

// ============================================================================
// Top-level builders
// ============================================================================

void IRBuilder::buildUsings(const std::vector<UsingDecl>& usings) {
    for (const auto& u : usings) {
        std::string modulePath;
        for (size_t i = 0; i < u.path.size(); i++) {
            if (i > 0) modulePath += ".";
            modulePath += u.path[i];
        }
        module->usings.push_back(modulePath);
    }
}

void IRBuilder::buildCImports(const std::vector<CImportDecl>& cimports) {
    for (const auto& imp : cimports) {
        IRCImport irImp;
        irImp.header = imp.header;
        irImp.isSystemHeader = imp.isSystemHeader;
        irImp.asNamespace = imp.asNamespace;
        irImp.symbols = imp.symbols;
        module->cimports.push_back(irImp);
        
        if (!imp.asNamespace.empty()) {
            importedNamespaces.insert(imp.asNamespace);
        }
    }
}

void IRBuilder::buildCppHeaders(const std::vector<CppHeaderDecl>& headers) {
    for (const auto& h : headers) {
        module->cppHeaders.push_back(h.code);
    }
}

void IRBuilder::buildLinkDecls(const std::vector<LinkDecl>& links) {
    for (const auto& l : links) {
        for (const auto& flag : l.flags) {
            module->linkFlags.push_back(flag);
        }
    }
}

void IRBuilder::buildIncludeDecls(const std::vector<IncludeDecl>& includes) {
    for (const auto& inc : includes) {
        for (const auto& h : inc.headers) {
            module->includeHeaders.push_back(h);
        }
    }
}

IRClassPtr IRBuilder::buildClass(const ClassDecl& cls) {
    auto irClass = std::make_shared<IRClass>();
    irClass->name = cls.name;
    irClass->isPublic = cls.isPublic;
    irClass->parentClass = cls.parent;
    
    // Build fields
    for (const auto& f : cls.fields) {
        IRField irField;
        irField.name = f.name;
        irField.type = convertType(f.type);
        irField.isPublic = f.isPublic;
        irField.isStatic = f.isStatic;
        
        // Handle static field initializers
        if (f.initValue && f.isStatic) {
            irField.initValue = buildExpr(f.initValue);
        }
        
        irClass->fields.push_back(irField);
    }
    
    // Build methods
    currentClassName = cls.name;
    for (const auto& m : cls.methods) {
        auto irMethod = buildFunction(m, cls.name);
        irClass->methods.push_back(irMethod);
    }
    currentClassName.clear();
    
    return irClass;
}

IRFunctionPtr IRBuilder::buildFunction(const FnDecl& fn, const std::string& className) {
    auto irFunc = std::make_shared<IRFunction>();
    irFunc->name = fn.name;
    irFunc->className = className;
    irFunc->returnType = convertType(fn.returnType);
    irFunc->isPublic = fn.isPublic;
    irFunc->isStatic = fn.isStatic;
    irFunc->isConstructor = (fn.name == "create" && !className.empty());
    irFunc->isMain = (fn.name == "main" && className.empty());
    
    currentFunction = irFunc;
    pushScope();
    
    // Create entry block
    auto entryBlock = createBlock("entry");
    irFunc->entryBlock = entryBlock;
    irFunc->addBlock(entryBlock);
    setInsertPoint(entryBlock);
    
    // Build parameters
    for (size_t i = 0; i < fn.params.size(); i++) {
        const auto& p = fn.params[i];
        auto paramType = convertType(p.type);
        auto paramValue = IRValue::makeParameter(p.name, paramType, nextValueId++);
        
        IRParameter irParam;
        irParam.name = p.name;
        irParam.type = paramType;
        irParam.value = paramValue;
        irFunc->parameters.push_back(irParam);
        
        declareVariable(p.name, paramValue);
    }
    
    // Build function body
    for (const auto& stmt : fn.body) {
        buildStmt(stmt);
    }
    
    // Add implicit return for main or void functions
    if (!currentBlock->isTerminated()) {
        if (irFunc->isMain) {
            // Return 0 for main
            emitReturn(createConst(static_cast<int64_t>(0)));
        } else if (irFunc->returnType && irFunc->returnType->kind == IRTypeKind::Void) {
            emitReturn();
        }
    }
    
    popScope();
    currentFunction = nullptr;
    currentBlock = nullptr;
    
    return irFunc;
}

} // namespace IR
