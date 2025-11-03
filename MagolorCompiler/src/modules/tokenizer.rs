use logos::Logos;

#[derive(Logos, Debug, PartialEq, Clone)]
pub enum Token {
    #[regex(r#""([^"]*)""#, |lex| lex.slice().trim_matches('"').to_string())]
    String(String),
    
    // Numeric literals
    #[regex(r"[0-9]+\.[0-9]+f64", |lex| lex.slice().trim_end_matches("f64").parse::<f64>().ok())]
    Float64(f64),
    #[regex(r"[0-9]+\.[0-9]+f32", |lex| lex.slice().trim_end_matches("f32").parse::<f32>().ok())]
    Float32(f32),
    #[regex(r"[0-9]+\.[0-9]+", |lex| lex.slice().parse::<f32>().ok())]
    DefaultFloat(f32),
    #[regex(r"[0-9]+i64", |lex| lex.slice().trim_end_matches("i64").parse::<i64>().ok())]
    Integer64(i64),
    #[regex(r"[0-9]+", |lex| lex.slice().parse::<i32>().ok())]
    Integer(i32),
    
    // Boolean literals
    #[regex(r"true|false", |lex| lex.slice().parse::<bool>().ok())]
    Bool(bool),
    
    // Keywords
    #[regex("fn|func|Fn|Func")]
    Func,
    #[regex("return|Return")]
    Return,
    #[token("let")]
    Let,
    #[token("use")]
    Use,
    #[regex("void|Void")]
    Void,
    #[token("if")]
    If,
    #[token("else")]
    Else,
    #[token("elif")]
    Elif,
    #[token("while")]
    While,
    #[token("for")]
    For,
    #[token("in")]
    In,
    #[token("break")]
    Break,
    #[token("continue")]
    Continue,
    #[token("class")]
    Class,
    #[token("struct")]
    Struct,
    #[token("new")]
    New,
    #[token("this")]
    This,
    #[token("public")]
    Public,
    #[token("private")]
    Private,
    #[token("static")]
    Static,
    #[token("namespace")]
    Namespace,
    #[token("enum")]
    Enum,
    #[token("interface")]
    Interface,
    #[token("implements")]
    Implements,
    #[token("extends")]
    Extends,
    #[token("null")]
    Null,
    #[token("try")]
    Try,
    #[token("catch")]
    Catch,
    #[token("finally")]
    Finally,
    #[token("throw")]
    Throw,
    #[token("async")]
    Async,
    #[token("await")]
    Await,
    #[token("const")]
    Const,
    #[token("readonly")]
    Readonly,
    #[token("override")]
    Override,
    #[token("virtual")]
    Virtual,
    #[token("abstract")]
    Abstract,
    #[token("sealed")]
    Sealed,
    #[token("var")]
    Var,
    #[token("switch")]
    Switch,
    #[token("case")]
    Case,
    #[token("default")]
    Default,
    
    // Operators
    #[token(">")]
    Greater,
    #[token("<")]
    Less,
    #[token("!=")]
    NotEq,
    #[token("<=")]
    LessEq,
    #[token(">=")]
    GreaterEq,
    #[token("==")]
    EqEq,
    #[token("&&")]
    And,
    #[token("||")]
    Or,
    #[token("!")]
    Not,
    #[token("+")]
    Plus,
    #[token("-")]
    Minus,
    #[token("*")]
    Multiply,
    #[token("/")]
    Divide,
    #[token("%")]
    Modulo,
    #[token("+=")]
    PlusEq,
    #[token("-=")]
    MinusEq,
    #[token("*=")]
    MultiplyEq,
    #[token("/=")]
    DivideEq,
    #[token("++")]
    Increment,
    #[token("--")]
    Decrement,
    #[token("&")]
    BitwiseAnd,
    #[token("|")]
    BitwiseOr,
    #[token("^")]
    BitwiseXor,
    #[token("<<")]
    LeftShift,
    #[token(">>")]
    RightShift,
    #[token("?.")]
    NullConditional,
    #[token("??")]
    NullCoalesce,
    #[token("=>")]
    Arrow,

    // Type keywords
    #[token("i32")]
    I32Type,
    #[token("i64")]
    I64Type,
    #[token("f32")]
    F32Type,
    #[token("f64")]
    F64Type,
    #[regex("String|string")]
    StringType,
    #[regex("bool|Bool")]
    BoolType,
    #[token("byte")]
    ByteType,
    #[token("short")]
    ShortType,
    #[token("long")]
    LongType,
    #[token("double")]
    DoubleType,
    #[token("decimal")]
    DecimalType,
    #[token("char")]
    CharType,
    #[token("object")]
    ObjectType,
    
    // Punctuation
    #[token("(")]
    LParen,
    #[token(")")]
    RParen,
    #[token("{")]
    LBrace,
    #[token("}")]
    RBrace,
    #[token("[")]
    LBracket,
    #[token("]")]
    RBracket,
    #[token(";")]
    Semicolon,
    #[token(":")]
    Colon,
    #[token("=")]
    Eq,
    #[token(",")]
    Comma,
    #[token(".")]
    Dot,
    #[token("?")]
    Question,
    
    // Identifiers (must come after keywords to avoid conflicts)
    #[regex("[a-zA-Z_][a-zA-Z0-9_]*", |lex| lex.slice().to_string())]
    Ident(String),
    
    // Skip whitespace and comments
    #[regex(r"[ \t\n\f]+", logos::skip)]
    #[regex(r"//[^\n]*", logos::skip)]
    #[regex(r"/\*([^*]|\*[^/])*\*/", logos::skip)]
    Error,
}

pub fn tokenizeFile(input: &str) -> Vec<Token> {
    Token::lexer(input)
        .filter_map(|tok| tok.ok())
        .collect()
}
