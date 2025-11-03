// src/modules/ast.rs
// Core AST type definitions

#[derive(Debug, Clone, PartialEq)]
pub enum Type {
    I32,
    I64,
    F32,
    F64,
    String,
    Bool,
    Void,
    Array(Box<Type>),
    Custom(String), // For user-defined types/classes
    Auto, // Type inference
}

#[derive(Debug, Clone)]
pub enum UnaryOp {
    Not,      // !
    Negate,   // -
    PreInc,   // ++x
    PreDec,   // --x
    PostInc,  // x++
    PostDec,  // x--
}

#[derive(Debug, Clone)]
pub enum BinaryOp {
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    And,      // &&
    Or,       // ||
    BitAnd,   // &
    BitOr,    // |
    BitXor,   // ^
    Shl,      // <<
    Shr,      // >>
}

#[derive(Debug, Clone)]
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
    Str(String),
    Int(i32),
    Int64(i64),
    Float32(f32),
    Float64(f64),
    Bool(bool),
    Null,
    VarRef(String),
    ArrayLiteral(Vec<ASTValue>),
    ArrayAccess {
        array: Box<ASTValue>,
        index: Box<ASTValue>,
    },
    MemberAccess {
        object: Box<ASTValue>,
        member: String,
    },
    FuncCall {
        name: String,
        args: Vec<ASTValue>,
    },
    MethodCall {
        object: Box<ASTValue>,
        method: String,
        args: Vec<ASTValue>,
    },
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
    Ternary {
        condition: Box<ASTValue>,
        then_val: Box<ASTValue>,
        else_val: Box<ASTValue>,
    },
    Cast {
        value: Box<ASTValue>,
        target_type: Type,
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
}

#[derive(Debug, Clone)]
pub struct MatchArm {
    pub pattern: Pattern,
    pub body: Vec<Statement>,
}

#[derive(Debug, Clone)]
pub enum Pattern {
    Literal(ASTValue),
    Wildcard,
    Identifier(String),
}

#[derive(Debug, Clone)]
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
}

#[derive(Debug, Clone)]
pub struct FunctionDef {
    pub name: String,
    pub params: Vec<Parameter>,
    pub return_type: Type,
    pub body: Vec<Statement>,
    pub access: AccessModifier,
    pub is_static: bool,
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
    pub fields: Vec<Field>,
    pub methods: Vec<FunctionDef>,
    pub constructor: Option<FunctionDef>,
    pub access: AccessModifier,
}

#[derive(Debug, Clone)]
pub struct StructDef {
    pub name: String,
    pub fields: Vec<Field>,
    pub access: AccessModifier,
}

#[derive(Debug, Clone)]
pub struct EnumDef {
    pub name: String,
    pub variants: Vec<String>,
    pub access: AccessModifier,
}

#[derive(Debug, Clone)]
pub enum TopLevel {
    Import(String),
    Function(FunctionDef),
    Class(ClassDef),
    Struct(StructDef),
    Enum(EnumDef),
    Namespace {
        name: String,
        items: Vec<TopLevel>,
    },
}

pub type Program = Vec<TopLevel>;
