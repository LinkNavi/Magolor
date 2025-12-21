// src/modules/ast.rs - Comprehensive AST with full C#/Rust features

use std::collections::HashMap;

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum Type {
    // Primitive types
    I32,
    I64,
    F32,
    F64,
    String,
    Bool,
    Void,
    
    // Arrays (C# style: int[])
    Array(Box<Type>),
    
    // User-defined types
    Custom(String),
    
    // Type inference (var in C#)
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
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
}

#[derive(Debug, Clone)]
pub enum UnaryOp {
    Not,
    Negate,
    PreInc,   // ++x
    PreDec,   // --x
    PostInc,  // x++
    PostDec,  // x--
}

#[derive(Debug, Clone)]
pub enum ASTValue {
    // Literals
    Int(i64),
    Float(f64),
    Bool(bool),
    Str(String),
    Null,
    
    // Variables
    VarRef(String),
    
    // Collections
    ArrayLiteral(Vec<ASTValue>),
    
    // Access
    ArrayAccess {
        array: Box<ASTValue>,
        index: Box<ASTValue>,
    },
    MemberAccess {
        object: Box<ASTValue>,
        member: String,
    },
    
    // Function calls
    FuncCall {
        name: String,
        args: Vec<ASTValue>,
    },
    MethodCall {
        object: Box<ASTValue>,
        method: String,
        args: Vec<ASTValue>,
    },
    
    // Object creation (C# style: new MyClass())
    NewObject {
        class_name: String,
        args: Vec<ASTValue>,
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
    
    // Ternary (C# style: condition ? then : else)
    Ternary {
        condition: Box<ASTValue>,
        then_val: Box<ASTValue>,
        else_val: Box<ASTValue>,
    },
    
    // This keyword (C# style)
    This,
}

#[derive(Debug, Clone)]
pub enum Statement {
    // Variable declaration (C# style: int x = 5; or var x = 5;)
    VarDecl {
        name: String,
        var_type: Type,
        value: Option<ASTValue>,
        is_mutable: bool,  // true by default in C#, explicit in Rust
    },
    
    // Assignment
    Assignment {
        target: ASTValue,
        value: ASTValue,
    },
    
    // Compound assignment (C# style: x += 5)
    CompoundAssignment {
        target: ASTValue,
        op: BinaryOp,
        value: ASTValue,
    },
    
    // Expression statement
    Expression(ASTValue),
    
    // Return
    Return(Option<ASTValue>),
    
    // Control flow
    Break,
    Continue,
    
    // If statement (C# style: if/elif/else)
    If {
        condition: ASTValue,
        then_body: Vec<Statement>,
        elif_branches: Vec<(ASTValue, Vec<Statement>)>,
        else_body: Option<Vec<Statement>>,
    },
    
    // While loop
    While {
        condition: ASTValue,
        body: Vec<Statement>,
    },
    
    // For loop (C-style)
    For {
        init: Option<Box<Statement>>,
        condition: Option<ASTValue>,
        increment: Option<Box<Statement>>,
        body: Vec<Statement>,
    },
    
    // Foreach loop (C# style)
    ForEach {
        item: String,
        item_type: Type,
        collection: ASTValue,
        body: Vec<Statement>,
    },
    
    // Block
    Block(Vec<Statement>),
}

#[derive(Debug, Clone, PartialEq)]
pub enum AccessModifier {
    Public,
    Private,
    Protected,
}

impl Default for AccessModifier {
    fn default() -> Self {
        AccessModifier::Public
    }
}

#[derive(Debug, Clone)]
pub struct Parameter {
    pub name: String,
    pub param_type: Type,
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
pub struct Constructor {
    pub params: Vec<Parameter>,
    pub body: Vec<Statement>,
    pub access: AccessModifier,
}

#[derive(Debug, Clone)]
pub struct ClassDef {
    pub name: String,
    pub fields: Vec<Field>,
    pub methods: Vec<FunctionDef>,
    pub constructors: Vec<Constructor>,
    pub access: AccessModifier,
}

#[derive(Debug, Clone)]
pub struct StructDef {
    pub name: String,
    pub fields: Vec<Field>,
    pub access: AccessModifier,
}

#[derive(Debug, Clone)]
pub struct ConstantDef {
    pub name: String,
    pub const_type: Type,
    pub value: ASTValue,
    pub access: AccessModifier,
}

#[derive(Debug, Clone)]
pub struct NamespaceDef {
    pub name: String,
    pub items: Vec<TopLevel>,
}

#[derive(Debug, Clone)]
pub enum TopLevel {
    Function(FunctionDef),
    Class(ClassDef),
    Struct(StructDef),
    Namespace(NamespaceDef),
    Constant(ConstantDef),
}

pub type Program = Vec<TopLevel>;

// Helper methods
impl Type {
    pub fn is_numeric(&self) -> bool {
        matches!(self, Type::I32 | Type::I64 | Type::F32 | Type::F64)
    }
    
    pub fn is_integer(&self) -> bool {
        matches!(self, Type::I32 | Type::I64)
    }
    
    pub fn is_void(&self) -> bool {
        matches!(self, Type::Void)
    }
}

impl FunctionDef {
    pub fn new(name: String, params: Vec<Parameter>, return_type: Type, body: Vec<Statement>) -> Self {
        Self {
            name,
            params,
            return_type,
            body,
            access: AccessModifier::Public,
            is_static: false,
        }
    }
}

impl ClassDef {
    pub fn new(name: String) -> Self {
        Self {
            name,
            fields: Vec::new(),
            methods: Vec::new(),
            constructors: Vec::new(),
            access: AccessModifier::Public,
        }
    }
    
    pub fn add_field(&mut self, field: Field) {
        self.fields.push(field);
    }
    
    pub fn add_method(&mut self, method: FunctionDef) {
        self.methods.push(method);
    }
    
    pub fn add_constructor(&mut self, constructor: Constructor) {
        self.constructors.push(constructor);
    }
}

impl StructDef {
    pub fn new(name: String) -> Self {
        Self {
            name,
            fields: Vec::new(),
            access: AccessModifier::Public,
        }
    }
    
    pub fn add_field(&mut self, field: Field) {
        self.fields.push(field);
    }
}
