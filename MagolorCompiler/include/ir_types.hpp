#pragma once

#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <unordered_map>
#include <optional>

namespace IR {

// Forward declarations
struct IRType;
struct IRValue;
struct IRInstruction;
struct IRBasicBlock;
struct IRFunction;
struct IRClass;
struct IRModule;

using IRTypePtr = std::shared_ptr<IRType>;
using IRValuePtr = std::shared_ptr<IRValue>;
using IRInstructionPtr = std::shared_ptr<IRInstruction>;
using IRBasicBlockPtr = std::shared_ptr<IRBasicBlock>;
using IRFunctionPtr = std::shared_ptr<IRFunction>;
using IRClassPtr = std::shared_ptr<IRClass>;
using IRModulePtr = std::shared_ptr<IRModule>;

// ============================================================================
// IR Type System
// ============================================================================

enum class IRTypeKind {
    Void,
    Int64,
    Float64,
    Bool,
    String,
    Pointer,
    Array,
    Map,
    Set,
    Optional,
    Function,
    Class,
    Generic,
    Auto
};

struct IRType {
    IRTypeKind kind;
    std::string name;                      // For class types
    IRTypePtr elementType;                 // For Array, Optional, Pointer
    IRTypePtr keyType;                     // For Map
    IRTypePtr valueType;                   // For Map
    IRTypePtr returnType;                  // For Function
    std::vector<IRTypePtr> paramTypes;     // For Function
    std::vector<IRTypePtr> genericArgs;    // For Generic types
    
    static IRTypePtr makeVoid();
    static IRTypePtr makeInt64();
    static IRTypePtr makeFloat64();
    static IRTypePtr makeBool();
    static IRTypePtr makeString();
    static IRTypePtr makePointer(IRTypePtr pointee);
    static IRTypePtr makeArray(IRTypePtr element);
    static IRTypePtr makeMap(IRTypePtr key, IRTypePtr value);
    static IRTypePtr makeSet(IRTypePtr element);
    static IRTypePtr makeOptional(IRTypePtr inner);
    static IRTypePtr makeFunction(IRTypePtr ret, std::vector<IRTypePtr> params);
    static IRTypePtr makeClass(const std::string& name);
    static IRTypePtr makeGeneric(const std::string& name, std::vector<IRTypePtr> args);
    static IRTypePtr makeAuto();
    
    bool equals(const IRTypePtr& other) const;
    std::string toString() const;
};

// ============================================================================
// IR Values (SSA-style)
// ============================================================================

enum class IRValueKind {
    Constant,
    Parameter,
    Instruction,
    Global,
    Temporary
};

struct IRConstant {
    std::variant<int64_t, double, bool, std::string> value;
    bool isNone = false;  // For Option::None
};

struct IRValue {
    IRValueKind kind;
    IRTypePtr type;
    std::string name;
    uint64_t id;
    std::optional<IRConstant> constant;
    
    static IRValuePtr makeConstInt(int64_t val);
    static IRValuePtr makeConstFloat(double val);
    static IRValuePtr makeConstBool(bool val);
    static IRValuePtr makeConstString(const std::string& val);
    static IRValuePtr makeConstNone(IRTypePtr optType);
    static IRValuePtr makeParameter(const std::string& name, IRTypePtr type, uint64_t id);
    static IRValuePtr makeTemp(IRTypePtr type, uint64_t id);
    static IRValuePtr makeGlobal(const std::string& name, IRTypePtr type);
};

// ============================================================================
// IR Instructions
// ============================================================================

enum class IROpcode {
    // Arithmetic
    Add, Sub, Mul, Div, Mod, Neg,
    
    // Comparison
    Eq, Ne, Lt, Le, Gt, Ge,
    
    // Logical
    And, Or, Not,
    
    // Memory
    Alloca, Load, Store, GetElementPtr,
    
    // Control flow
    Branch, CondBranch, Return, Unreachable,
    
    // Function calls
    Call, CallMethod, CallStatic,
    
    // Object operations
    New, GetField, SetField, This,
    
    // Array operations
    ArrayCreate, ArrayGet, ArraySet, ArrayLength, ArrayPush, ArrayPop,
    
    // Map operations
    MapCreate, MapGet, MapSet, MapKeys, MapValues,
    
    // Optional operations
    Some, None, IsSome, IsNone, Unwrap, UnwrapOr,
    
    // String operations
    StringConcat, StringInterpolate, ToString,
    
    // Lambda
    Lambda, Capture,
    
    // Type conversion
    Cast, BitCast,
    
    // Phi node for SSA
    Phi,
    
    // Special
    CppInline, Nop
};

struct IRInstruction {
    IROpcode opcode;
    IRValuePtr result;
    std::vector<IRValuePtr> operands;
    IRTypePtr type;
    
    // Additional metadata for specific instructions
    std::string stringData;                // For CppInline, field names, etc.
    std::string className;                 // For New, GetField, SetField
    std::string methodName;                // For CallMethod, CallStatic
    IRBasicBlockPtr trueBlock;             // For CondBranch
    IRBasicBlockPtr falseBlock;            // For CondBranch
    IRBasicBlockPtr targetBlock;           // For Branch
    std::vector<std::pair<IRValuePtr, IRBasicBlockPtr>> phiIncoming; // For Phi
    std::vector<std::string> captureNames; // For Lambda captures
    IRFunctionPtr lambdaFunc;              // For Lambda
    bool isInterpolated = false;           // For StringConcat
    
    static IRInstructionPtr make(IROpcode op, IRValuePtr result = nullptr);
};

// ============================================================================
// Basic Block
// ============================================================================

struct IRBasicBlock {
    std::string name;
    std::vector<IRInstructionPtr> instructions;
    IRFunctionPtr parent;
    
    // Predecessor/successor tracking for CFG
    std::vector<IRBasicBlockPtr> predecessors;
    std::vector<IRBasicBlockPtr> successors;
    
    bool isTerminated() const;
    void addInstruction(IRInstructionPtr inst);
    IRInstructionPtr getTerminator() const;
};

// ============================================================================
// Function
// ============================================================================

struct IRParameter {
    std::string name;
    IRTypePtr type;
    IRValuePtr value;
};

struct IRFunction {
    std::string name;
    std::string className;                 // Empty for free functions
    IRTypePtr returnType;
    std::vector<IRParameter> parameters;
    std::vector<IRBasicBlockPtr> blocks;
    IRBasicBlockPtr entryBlock;
    bool isPublic = false;
    bool isStatic = false;
    bool isConstructor = false;
    bool isMain = false;
    
    // Local variable tracking
    std::unordered_map<std::string, IRValuePtr> locals;
    
    IRBasicBlockPtr createBlock(const std::string& name);
    void addBlock(IRBasicBlockPtr block);
};

// ============================================================================
// Class/Struct
// ============================================================================

struct IRField {
    std::string name;
    IRTypePtr type;
    bool isPublic = false;
    bool isStatic = false;
    IRValuePtr initValue;  // For static const fields
};

struct IRClass {
    std::string name;
    std::vector<IRField> fields;
    std::vector<IRFunctionPtr> methods;
    std::string parentClass;
    bool isPublic = false;
};

// ============================================================================
// Module (Top-level IR container)
// ============================================================================

struct IRCImport {
    std::string header;
    bool isSystemHeader;
    std::string asNamespace;
    std::vector<std::string> symbols;
};

struct IRModule {
    std::string name;
    
    // Declarations
    std::vector<IRClassPtr> classes;
    std::vector<IRFunctionPtr> functions;
    std::vector<IRCImport> cimports;
    
    // Metadata
    std::vector<std::string> cppHeaders;
    std::vector<std::string> linkFlags;
    std::vector<std::string> includeHeaders;
    std::vector<std::string> usings;
    
    // Stdlib module implementations (preserved from original)
    std::unordered_map<std::string, std::string> stdModuleImpls;
    
    IRFunctionPtr findFunction(const std::string& name) const;
    IRClassPtr findClass(const std::string& name) const;
};

} // namespace IR
