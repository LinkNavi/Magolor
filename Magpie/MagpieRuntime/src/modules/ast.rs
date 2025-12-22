// src/modules/ast.rs
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum Type {
    I8, I16, I32, I64,
    U8, U16, U32, U64,
    F32, F64,
    String, Bool, Char, Void,
    Array(Box<Type>),
    Object,  // New: JS-like object type
    Custom(String),
    Inferred,
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub enum BinaryOp {
    Add, Sub, Mul, Div, Mod,
    And, Or,
    BitAnd, BitOr, BitXor, Shl, Shr,
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub enum ComparisonOp {
    Equal, NotEqual, Less, LessEqual, Greater, GreaterEqual,
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub enum UnaryOp {
    Not, Negate, BitNot,
    PreInc, PreDec, PostInc, PostDec,
}

#[derive(Debug, Clone)]
pub enum Expr {
    Int(i64),
    Float(f64),
    Bool(bool),
    Str(String),
    Null,
    Var(String),
    Array(Vec<Expr>),
    Object(Vec<(String, Expr)>),  // New: Object literal {key: value, ...}
    Index { array: Box<Expr>, index: Box<Expr> },
    Field { object: Box<Expr>, field: String },
    Call { name: String, args: Vec<Expr> },
    Method { object: Box<Expr>, method: String, args: Vec<Expr> },
    New { class: String, args: Vec<Expr> },
    Binary { op: BinaryOp, left: Box<Expr>, right: Box<Expr> },
    Unary { op: UnaryOp, operand: Box<Expr> },
    Compare { op: ComparisonOp, left: Box<Expr>, right: Box<Expr> },
    Ternary { cond: Box<Expr>, then_val: Box<Expr>, else_val: Box<Expr> },
    This,
}

#[derive(Debug, Clone)]
pub enum Stmt {
    Let { name: String, ty: Type, value: Option<Expr>, mutable: bool },
    Assign { target: Expr, value: Expr },
    CompoundAssign { target: Expr, op: BinaryOp, value: Expr },
    Expr(Expr),
    Return(Option<Expr>),
    Break,
    Continue,
    If { cond: Expr, then_body: Vec<Stmt>, elifs: Vec<(Expr, Vec<Stmt>)>, else_body: Option<Vec<Stmt>> },
    While { cond: Expr, body: Vec<Stmt> },
    For { init: Option<Box<Stmt>>, cond: Option<Expr>, inc: Option<Box<Stmt>>, body: Vec<Stmt> },
    ForEach { item: String, ty: Type, collection: Expr, body: Vec<Stmt> },
    Block(Vec<Stmt>),
}

#[derive(Debug, Clone, PartialEq, Default)]
pub enum Access { #[default] Public, Private, Protected }

#[derive(Debug, Clone)]
pub struct Param { pub name: String, pub ty: Type }

#[derive(Debug, Clone)]
pub struct FnDef {
    pub name: String,
    pub params: Vec<Param>,
    pub ret: Type,
    pub body: Vec<Stmt>,
    pub access: Access,
    pub is_static: bool,
}

#[derive(Debug, Clone)]
pub struct Field {
    pub name: String,
    pub ty: Type,
    pub access: Access,
    pub is_static: bool,
    pub default: Option<Expr>,
}

#[derive(Debug, Clone)]
pub struct ClassDef {
    pub name: String,
    pub fields: Vec<Field>,
    pub methods: Vec<FnDef>,
    pub access: Access,
}

#[derive(Debug, Clone)]
pub struct StructDef {
    pub name: String,
    pub fields: Vec<Field>,
    pub access: Access,
}

#[derive(Debug, Clone)]
pub struct EnumVariant {
    pub name: String,
    pub value: Option<i64>,
}

#[derive(Debug, Clone)]
pub struct EnumDef {
    pub name: String,
    pub variants: Vec<EnumVariant>,
    pub access: Access,
}

#[derive(Debug, Clone)]
pub enum TopLevel {
    Function(FnDef),
    Class(ClassDef),
    Struct(StructDef),
    Enum(EnumDef),
    Const { name: String, ty: Type, value: Expr, access: Access },
}

pub type Program = Vec<TopLevel>;
