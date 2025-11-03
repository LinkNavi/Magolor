// src/modules/tokenizer.rs
use logos::Logos;

#[derive(Logos, Debug, Clone, PartialEq)]
pub enum Token {
    // String literals
    #[regex(r#""([^"\\]|\\.)*""#, |lex| lex.slice()[1..lex.slice().len()-1].to_string())]
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
    
    // Keywords - Control Flow
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
    #[token("match")]
    Match,
    #[token("case")]
    Case,
    #[token("break")]
    Break,
    #[token("continue")]
    Continue,
    #[token("return")]
    Return,
    
    // Keywords - Declarations
    #[token("fn")]
    Func,
    #[token("let")]
    Let,
    #[token("mut")]
    Mut,
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
    #[token("interface")]
    Interface,
    #[token("namespace")]
    Namespace,
    #[token("use")]
    Use,
    #[token("as")]
    As,
    
    // Keywords - Access Modifiers
    #[token("public")]
    Public,
    #[token("private")]
    Private,
    #[token("protected")]
    Protected,
    
    // Keywords - Special
    #[token("new")]
    New,
    #[token("this")]
    This,
    #[token("base")]
    Base,
    #[token("null")]
    Null,
    #[token("void")]
    Void,
    
    // Type keywords
    #[token("i32")]
    I32Type,
    #[token("i64")]
    I64Type,
    #[token("f32")]
    F32Type,
    #[token("f64")]
    F64Type,
    #[token("string")]
    StringType,
    #[token("bool")]
    BoolType,
    #[token("var")]
    VarType,
    
    // Operators - Arithmetic
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
    
    // Operators - Comparison
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
    
    // Operators - Logical
    #[token("&&")]
    AndAnd,
    #[token("||")]
    OrOr,
    #[token("!")]
    Not,
    
    // Operators - Bitwise
    #[token("&")]
    And,
    #[token("|")]
    Or,
    #[token("^")]
    Xor,
    #[token("<<")]
    Shl,
    #[token(">>")]
    Shr,
    
    // Operators - Assignment
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
    #[token("::")]
    ColonColon,
    #[token(",")]
    Comma,
    #[token(".")]
    Dot,
    #[token("?")]
    Question,
    #[token("->")]
    Arrow,
    #[token("=>")]
    FatArrow,
    
    // Identifiers (must come after keywords)
    #[regex("[a-zA-Z_][a-zA-Z0-9_]*", |lex| lex.slice().to_string())]
    Ident(String),
    
    // Comments
    #[regex(r"//[^\n]*", logos::skip)]
    #[regex(r"/\*([^*]|\*[^/])*\*/", logos::skip)]
    Comment,
    
    // Skip whitespace
    #[regex(r"[ \t\n\r\f]+", logos::skip)]
    Whitespace,
    
    // Error token
    #[error]
    Error,
}

pub fn tokenize(input: &str) -> Vec<Token> {
    Token::lexer(input)
        .filter_map(|result| match result {
            Ok(token) => Some(token),
            Err(_) => None,
        })
        .collect()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_basic_tokens() {
        let input = "let x = 42;";
        let tokens = tokenize(input);
        assert_eq!(tokens[0], Token::Let);
        assert_eq!(tokens[1], Token::Ident("x".to_string()));
        assert_eq!(tokens[2], Token::Eq);
        assert_eq!(tokens[3], Token::Integer(42));
        assert_eq!(tokens[4], Token::Semicolon);
    }

    #[test]
    fn test_operators() {
        let input = "+ - * / % == != < > <= >= && || !";
        let tokens = tokenize(input);
        assert_eq!(tokens[0], Token::Plus);
        assert_eq!(tokens[1], Token::Minus);
        assert_eq!(tokens[2], Token::Star);
    }
}