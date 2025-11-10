// src/modules/ast.rs - Enhanced with better type system

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum Type {
    // Primitive types
    I8,
    I16,
    I32,
    I64,
    U8,
    U16,
    U32,
    U64,
    F32,
    F64,
    Char,
    Bool,
    String,
    Void,
    
    // Pointer types
    Ptr(Box<Type>),
    Ref(Box<Type>, Mutability),
    
    // Compound types
    Array(Box<Type>, Option<usize>), // Type and optional size
    Slice(Box<Type>),
    Tuple(Vec<Type>),
    
    // User-defined types
    Custom(String),
    Generic(String, Vec<Type>), // Generic type with parameters
    
    // Function types
    Function {
        params: Vec<Type>,
        return_type: Box<Type>,
    },
    
    // Special
    Auto, // Type inference
    Never, // ! type for functions that never return
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum Mutability {
    Mutable,
    Immutable,
}

impl Type {
    pub fn is_numeric(&self) -> bool {
        matches!(
            self,
            Type::I8 | Type::I16 | Type::I32 | Type::I64 |
            Type::U8 | Type::U16 | Type::U32 | Type::U64 |
            Type::F32 | Type::F64
        )
    }
    
    pub fn is_integer(&self) -> bool {
        matches!(
            self,
            Type::I8 | Type::I16 | Type::I32 | Type::I64 |
            Type::U8 | Type::U16 | Type::U32 | Type::U64
        )
    }
    
    pub fn is_float(&self) -> bool {
        matches!(self, Type::F32 | Type::F64)
    }
    
    pub fn is_signed(&self) -> bool {
        matches!(
            self,
            Type::I8 | Type::I16 | Type::I32 | Type::I64
        )
    }
}

#[derive(Debug, Clone)]
pub enum UnaryOp {
    Not,      // !
    Negate,   // -
    BitwiseNot, // ~
    PreInc,   // ++x
    PreDec,   // --x
    PostInc,  // x++
    PostDec,  // x--
    Deref,    // *x
    AddrOf,   // &x
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub enum BinaryOp {
    // Arithmetic
    Add, Sub, Mul, Div, Mod, Power,
    
    // Logical
    And, Or,
    
    // Bitwise
    BitAnd, BitOr, BitXor, Shl, Shr,
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub enum ComparisonOp {
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
}

#[derive(Debug, Clone)]
pub enum ASTValue {
    // Literals
    Str(String),
    Char(char),
    Int(i64),
    UInt(u64),
    Float32(f32),
    Float64(f64),
    Bool(bool),
    Null,
    
    // Identifiers
    VarRef(String),
    
    // Collections
    ArrayLiteral(Vec<ASTValue>),
    TupleLiteral(Vec<ASTValue>),
    
    // Access operations
    ArrayAccess {
        array: Box<ASTValue>,
        index: Box<ASTValue>,
    },
    TupleAccess {
        tuple: Box<ASTValue>,
        index: usize,
    },
    MemberAccess {
        object: Box<ASTValue>,
        member: String,
    },
    
    // Function calls
    FuncCall {
        name: String,
        args: Vec<ASTValue>,
        generic_args: Vec<Type>,
    },
    MethodCall {
        object: Box<ASTValue>,
        method: String,
        args: Vec<ASTValue>,
        generic_args: Vec<Type>,
    },
    
    // Operators
    Binary {
        op: BinaryOp,
        left: Box<ASTValue>,
        right: Box<ASTValue>,
    },
    Unary {
        op: UnaryOp,
        operand: Box<ASTValue>,
    },
    Comparison {
        op: ComparisonOp,
        left: Box<ASTValue>,
        right: Box<ASTValue>,
    },
    
    // Control flow expressions
    Ternary {
        condition: Box<ASTValue>,
        then_val: Box<ASTValue>,
        else_val: Box<ASTValue>,
    },
    
    // Type operations
    Cast {
        value: Box<ASTValue>,
        target_type: Type,
    },
    SizeOf(Type),
    
    // Closures/Lambdas
    Lambda {
        params: Vec<Parameter>,
        return_type: Type,
        body: Vec<Statement>,
    },
    
    // Ranges
    Range {
        start: Box<ASTValue>,
        end: Box<ASTValue>,
        inclusive: bool,
    },
    
    // Null coalescing
    NullCoalesce {
        value: Box<ASTValue>,
        default: Box<ASTValue>,
    },
    
    // Safe navigation
    SafeNav {
        object: Box<ASTValue>,
        member: String,
    },
}

#[derive(Debug, Clone)]
pub enum Statement {
    VarDecl {
        name: String,
        var_type: Type,
        value: Option<ASTValue>,
        is_mutable: bool,
    },
    
    Assignment {
        target: ASTValue,
        value: ASTValue,
    },
    
    CompoundAssignment {
        target: ASTValue,
        op: BinaryOp,
        value: ASTValue,
    },
    
    Expression(ASTValue),
    
    Return(Option<ASTValue>),
    Break,
    Continue,
    
    // Control flow
    If {
        condition: ASTValue,
        then_body: Vec<Statement>,
        elif_branches: Vec<(ASTValue, Vec<Statement>)>,
        else_body: Option<Vec<Statement>>,
    },
    
    While {
        condition: ASTValue,
        body: Vec<Statement>,
    },
    
    For {
        init: Option<Box<Statement>>,
        condition: Option<ASTValue>,
        increment: Option<Box<Statement>>,
        body: Vec<Statement>,
    },
    
    ForEach {
        item: String,
        item_type: Type,
        collection: ASTValue,
        body: Vec<Statement>,
    },
    
    Match {
        value: ASTValue,
        arms: Vec<MatchArm>,
    },
    
    Block(Vec<Statement>),
    
    // Memory management
    Defer(Box<Statement>), // Execute statement at end of scope
}

#[derive(Debug, Clone)]
pub struct MatchArm {
    pub pattern: Pattern,
    pub guard: Option<ASTValue>, // Optional when clause
    pub body: Vec<Statement>,
}

#[derive(Debug, Clone)]
pub enum Pattern {
    Literal(ASTValue),
    Identifier(String),
    Tuple(Vec<Pattern>),
    Array(Vec<Pattern>),
    Wildcard,
    Range {
        start: Box<ASTValue>,
        end: Box<ASTValue>,
        inclusive: bool,
    },
}

#[derive(Debug, Clone, PartialEq)]
pub enum AccessModifier {
    Public,
    Private,
    Protected,
}

#[derive(Debug, Clone)]
pub struct Parameter {
    pub name: String,
    pub param_type: Type,
    pub default_value: Option<ASTValue>,
    pub is_ref: bool,
}

#[derive(Debug, Clone)]
pub struct GenericParam {
    pub name: String,
    pub constraints: Vec<String>, // Trait bounds
}

#[derive(Debug, Clone)]
pub struct FunctionDef {
    pub name: String,
    pub generic_params: Vec<GenericParam>,
    pub params: Vec<Parameter>,
    pub return_type: Type,
    pub body: Vec<Statement>,
    pub access: AccessModifier,
    pub is_static: bool,
    pub is_unsafe: bool,
    pub is_inline: bool,
}

#[derive(Debug, Clone)]
pub struct Field {
    pub name: String,
    pub field_type: Type,
    pub access: AccessModifier,
    pub is_static: bool,
    pub default_value: Option<ASTValue>,
}

#[derive(Debug, Clone)]
pub struct ClassDef {
    pub name: String,
    pub generic_params: Vec<GenericParam>,
    pub fields: Vec<Field>,
    pub methods: Vec<FunctionDef>,
    pub constructor: Option<FunctionDef>,
    pub destructor: Option<FunctionDef>,
    pub access: AccessModifier,
}

#[derive(Debug, Clone)]
pub struct StructDef {
    pub name: String,
    pub generic_params: Vec<GenericParam>,
    pub fields: Vec<Field>,
    pub access: AccessModifier,
    pub is_packed: bool,
}

#[derive(Debug, Clone)]
pub struct EnumVariant {
    pub name: String,
    pub data: Option<Vec<Type>>, // Tuple-like data
}

#[derive(Debug, Clone)]
pub struct EnumDef {
    pub name: String,
    pub generic_params: Vec<GenericParam>,
    pub variants: Vec<EnumVariant>,
    pub access: AccessModifier,
}

#[derive(Debug, Clone)]
pub struct TraitDef {
    pub name: String,
    pub generic_params: Vec<GenericParam>,
    pub methods: Vec<FunctionDef>, // Method signatures
    pub access: AccessModifier,
}

#[derive(Debug, Clone)]
pub struct ImplBlock {
    pub trait_name: Option<String>, // None for inherent impl
    pub type_name: String,
    pub generic_params: Vec<GenericParam>,
    pub methods: Vec<FunctionDef>,
}

#[derive(Debug, Clone)]
pub struct TypeAlias {
    pub name: String,
    pub generic_params: Vec<GenericParam>,
    pub target_type: Type,
    pub access: AccessModifier,
}

#[derive(Debug, Clone)]
pub enum TopLevel {
    Import(Vec<String>), // Import path
    Function(FunctionDef),
    Class(ClassDef),
    Struct(StructDef),
    Enum(EnumDef),
    Trait(TraitDef),
    Impl(ImplBlock),
    TypeAlias(TypeAlias),
    Namespace {
        name: String,
        items: Vec<TopLevel>,
    },
    // Global constant
    Constant {
        name: String,
        const_type: Type,
        value: ASTValue,
        access: AccessModifier,
    },
}

pub type Program = Vec<TopLevel>;

// Helper for building types
impl Type {
    pub fn ptr(inner: Type) -> Self {
        Type::Ptr(Box::new(inner))
    }
    
    pub fn ref_type(inner: Type, mutable: bool) -> Self {
        Type::Ref(
            Box::new(inner),
            if mutable { Mutability::Mutable } else { Mutability::Immutable }
        )
    }
    
    pub fn array(inner: Type, size: Option<usize>) -> Self {
        Type::Array(Box::new(inner), size)
    }
}
