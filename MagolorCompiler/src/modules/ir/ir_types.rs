// src/modules/ir/ir_types.rs
// Core IR type definitions

use std::collections::HashMap;

#[derive(Debug, Clone, PartialEq)]
pub enum IRType {
    I8,
    I16,
    I32,
    I64,
    F32,
    F64,
    Bool,
    Ptr(Box<IRType>),
    Array(Box<IRType>, Option<usize>),
    Struct(Vec<IRType>),
    Function(Vec<IRType>, Box<IRType>),
    Void,
}

impl IRType {
    pub fn size_bytes(&self) -> usize {
        match self {
            IRType::I8 | IRType::Bool => 1,
            IRType::I16 => 2,
            IRType::I32 | IRType::F32 => 4,
            IRType::I64 | IRType::F64 | IRType::Ptr(_) => 8,
            IRType::Array(ty, Some(len)) => ty.size_bytes() * len,
            IRType::Struct(fields) => fields.iter().map(|f| f.size_bytes()).sum(),
            _ => 8, // Default pointer size
        }
    }

    pub fn alignment(&self) -> usize {
        match self {
            IRType::I8 | IRType::Bool => 1,
            IRType::I16 => 2,
            IRType::I32 | IRType::F32 => 4,
            IRType::I64 | IRType::F64 | IRType::Ptr(_) => 8,
            IRType::Array(ty, _) => ty.alignment(),
            IRType::Struct(fields) => fields.iter().map(|f| f.alignment()).max().unwrap_or(1),
            _ => 8,
        }
    }
}

#[derive(Debug, Clone, PartialEq)]
pub enum IRValue {
    Register(usize),
    Constant(IRConstant),
    Global(String),
    Local(usize),
    Argument(usize),
    Undef,
}

#[derive(Debug, Clone, PartialEq)]
pub enum IRConstant {
    I8(i8),
    I16(i16),
    I32(i32),
    I64(i64),
    F32(f32),
    F64(f64),
    Bool(bool),
    String(String),
    Null,
}

#[derive(Debug, Clone)]
pub enum IRInstruction {
    // Arithmetic
    Add { dst: usize, lhs: IRValue, rhs: IRValue, ty: IRType },
    Sub { dst: usize, lhs: IRValue, rhs: IRValue, ty: IRType },
    Mul { dst: usize, lhs: IRValue, rhs: IRValue, ty: IRType },
    Div { dst: usize, lhs: IRValue, rhs: IRValue, ty: IRType },
    Mod { dst: usize, lhs: IRValue, rhs: IRValue, ty: IRType },
    
    // Bitwise
    And { dst: usize, lhs: IRValue, rhs: IRValue },
    Or { dst: usize, lhs: IRValue, rhs: IRValue },
    Xor { dst: usize, lhs: IRValue, rhs: IRValue },
    Shl { dst: usize, lhs: IRValue, rhs: IRValue },
    Shr { dst: usize, lhs: IRValue, rhs: IRValue, signed: bool },
    Not { dst: usize, src: IRValue },
    
    // Comparison
    Cmp { dst: usize, op: CmpOp, lhs: IRValue, rhs: IRValue, ty: IRType },
    
    // Memory
    Load { dst: usize, addr: IRValue, ty: IRType },
    Store { addr: IRValue, value: IRValue, ty: IRType },
    Alloca { dst: usize, ty: IRType, count: Option<usize> },
    
    // Control Flow
    Branch { target: usize },
    CondBranch { cond: IRValue, true_target: usize, false_target: usize },
    Return { value: Option<IRValue> },
    
    // Function Calls
    Call { dst: Option<usize>, func: String, args: Vec<IRValue>, ty: IRType },
    IndirectCall { dst: Option<usize>, func: IRValue, args: Vec<IRValue>, ty: IRType },
    
    // Conversions
    Cast { dst: usize, src: IRValue, from_ty: IRType, to_ty: IRType },
    Bitcast { dst: usize, src: IRValue, to_ty: IRType },
    
    // Array/Pointer
    GetElementPtr { dst: usize, base: IRValue, indices: Vec<IRValue>, ty: IRType },
    ExtractValue { dst: usize, aggregate: IRValue, index: usize },
    InsertValue { dst: usize, aggregate: IRValue, value: IRValue, index: usize },
    
    // Special
    Phi { dst: usize, incoming: Vec<(IRValue, usize)>, ty: IRType },
    Select { dst: usize, cond: IRValue, true_val: IRValue, false_val: IRValue },
    Move { dst: usize, src: IRValue },
    
    // Intrinsics
    Intrinsic { dst: Option<usize>, name: String, args: Vec<IRValue> },
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum CmpOp {
    Eq,
    Ne,
    Lt,
    Le,
    Gt,
    Ge,
    // Float-specific comparisons
    FEq,
    FNe,
    FLt,
    FLe,
    FGt,
    FGe,
}

#[derive(Debug, Clone)]
pub struct IRBasicBlock {
    pub id: usize,
    pub instructions: Vec<IRInstruction>,
    pub predecessors: Vec<usize>,
    pub successors: Vec<usize>,
    pub dominators: Vec<usize>,
    pub dom_frontier: Vec<usize>,
}

impl IRBasicBlock {
    pub fn new(id: usize) -> Self {
        Self {
            id,
            instructions: Vec::new(),
            predecessors: Vec::new(),
            successors: Vec::new(),
            dominators: Vec::new(),
            dom_frontier: Vec::new(),
        }
    }
}

#[derive(Debug, Clone)]
pub struct IRFunction {
    pub name: String,
    pub params: Vec<(String, IRType)>,
    pub return_type: IRType,
    pub blocks: Vec<IRBasicBlock>,
    pub local_count: usize,
    pub register_count: usize,
    pub is_inline: bool,
    pub is_pure: bool, // For optimization
    pub attributes: FunctionAttributes,
}

#[derive(Debug, Clone, Default)]
pub struct FunctionAttributes {
    pub inline: InlineHint,
    pub no_inline: bool,
    pub pure: bool,
    pub const_fn: bool,
    pub hot: bool,
    pub cold: bool,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum InlineHint {
    None,
    Always,
    Hint,
}

impl Default for InlineHint {
    fn default() -> Self {
        InlineHint::None
    }
}

#[derive(Debug, Clone)]
pub struct IRGlobal {
    pub name: String,
    pub ty: IRType,
    pub init: Option<IRConstant>,
    pub is_const: bool,
    pub alignment: usize,
}

#[derive(Debug)]
pub struct IRProgram {
    pub functions: HashMap<String, IRFunction>,
    pub globals: HashMap<String, IRGlobal>,
    pub string_literals: HashMap<String, usize>,
    pub type_definitions: HashMap<String, IRType>,
}

impl IRProgram {
    pub fn new() -> Self {
        Self {
            functions: HashMap::new(),
            globals: HashMap::new(),
            string_literals: HashMap::new(),
            type_definitions: HashMap::new(),
        }
    }
}
