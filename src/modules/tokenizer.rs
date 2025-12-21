use logos::Logos;

#[derive(Logos, Debug, Clone, PartialEq)]
pub enum Token {
    #[regex(r#""([^"\\]|\\[nrt"\\])*""#, |lex| {
        let s = lex.slice();
        let content = &s[1..s.len()-1];
        content.replace("\\n", "\n")
            .replace("\\r", "\r")
            .replace("\\t", "\t")
            .replace("\\\"", "\"")
            .replace("\\\\", "\\")
    })]
    String(String),
    
    #[regex(r"[0-9]+\.[0-9]+", |lex| lex.slice().parse().ok())]
    Float(f64),
    
    #[regex(r"[0-9]+", |lex| lex.slice().parse().ok())]
    Integer(i64),
    
    #[token("true")]
    True,
    #[token("false")]
    False,
    #[token("null")]
    Null,
    
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
    #[token("foreach")]
    ForEach,
    #[token("in")]
    In,
    #[token("break")]
    Break,
    #[token("continue")]
    Continue,
    #[token("return")]
    Return,
    #[token("match")]
    Match,
    
    #[token("fn")]
    Func,
    #[token("let")]
    Let,
    #[token("const")]
    Const,
    #[token("static")]
    Static,
    #[token("class")]
    Class,
    #[token("struct")]
    Struct,
    #[token("enum")]
    Enum,
    #[token("impl")]
    Impl,
    #[token("trait")]
    Trait,
    #[token("namespace")]
    Namespace,
    #[token("use")]
    Use,
    
    #[token("public")]
    Public,
    #[token("private")]
    Private,
    #[token("protected")]
    Protected,
    
    #[token("this")]
    This,
    #[token("self")]
    SelfKw,
    #[token("void")]
    Void,
    #[token("new")]
    New,
    #[token("mut")]
    Mut,
    #[token("ref")]
    Ref,
    
    #[token("i8")]
    I8Type,
    #[token("i16")]
    I16Type,
    #[token("i32")]
    I32Type,
    #[token("i64")]
    I64Type,
    #[token("u8")]
    U8Type,
    #[token("u16")]
    U16Type,
    #[token("u32")]
    U32Type,
    #[token("u64")]
    U64Type,
    #[token("f32")]
    F32Type,
    #[token("f64")]
    F64Type,
    #[token("string")]
    StringType,
    #[token("bool")]
    BoolType,
    #[token("char")]
    CharType,
    #[token("var")]
    VarType,
    
    #[token("+")]
    Plus,
    #[token("-")]
    Minus,
    #[token("*")]
    Star,
    #[token("/")]
    Slash,
    #[token("%")]
    Percent,
    #[token("++")]
    PlusPlus,
    #[token("--")]
    MinusMinus,
    
    #[token("==")]
    EqEq,
    #[token("!=")]
    NotEq,
    #[token("<")]
    Less,
    #[token("<=")]
    LessEq,
    #[token(">")]
    Greater,
    #[token(">=")]
    GreaterEq,
    
    #[token("&&")]
    AndAnd,
    #[token("||")]
    OrOr,
    #[token("!")]
    Not,
    
    #[token("&")]
    And,
    #[token("|")]
    Or,
    #[token("^")]
    Xor,
    #[token("~")]
    BitwiseNot,
    #[token("<<")]
    Shl,
    #[token(">>")]
    Shr,
    
    #[token("=")]
    Eq,
    #[token("+=")]
    PlusEq,
    #[token("-=")]
    MinusEq,
    #[token("*=")]
    StarEq,
    #[token("/=")]
    SlashEq,
    #[token("%=")]
    PercentEq,
    #[token("&=")]
    AndEq,
    #[token("|=")]
    OrEq,
    #[token("^=")]
    XorEq,
    #[token("<<=")]
    ShlEq,
    #[token(">>=")]
    ShrEq,
    
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
    #[token("::")]
    ColonColon,
    #[token(",")]
    Comma,
    #[token(".")]
    Dot,
    #[token("?")]
    Question,
    #[token("=>")]
    FatArrow,
    #[token("->")]
    Arrow,
    
    #[regex("[a-zA-Z_][a-zA-Z0-9_]*", |lex| lex.slice().to_string())]
    Ident(String),
    
    #[regex(r"//[^\n]*", logos::skip)]
    #[regex(r"/\*([^*]|\*[^/])*\*/", logos::skip)]
    Comment,
    
    #[regex(r"[ \t\r\n]+", logos::skip)]
    Whitespace,
}

pub fn tokenize(input: &str) -> Vec<Token> {
    Token::lexer(input).filter_map(|r| r.ok()).collect()
}
