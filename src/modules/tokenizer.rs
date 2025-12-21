// src/modules/tokenizer.rs - Clean tokenizer for C#/Rust hybrid language
use logos::Logos;

#[derive(Logos, Debug, Clone, PartialEq)]
pub enum Token {
    // === Literals ===
    
    // Strings (C# style)
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
    
    // Numbers (simple parsing)
    #[regex(r"[0-9]+", |lex| lex.slice().parse().ok())]
    Integer(i64),
    
    #[regex(r"[0-9]+\.[0-9]+", |lex| lex.slice().parse().ok())]
    Float(f64),
    
    // Booleans (C# style: true/false)
    #[token("true")]
    True,
    #[token("false")]
    False,
    
    #[token("null")]  // C# style
    Null,
    
    // === Keywords - Control Flow (C# style) ===
    #[token("if")]
    If,
    #[token("else")]
    Else,
    #[token("elif")]    // Python-style shorthand
    Elif,
    #[token("while")]
    While,
    #[token("for")]
    For,
    #[token("foreach")]  // C# style
    ForEach,
    #[token("in")]
    In,
    #[token("break")]
    Break,
    #[token("continue")]
    Continue,
    #[token("return")]
    Return,
    
    // === Keywords - Declarations (C# style) ===
    #[token("fn")]       // Rust style for functions
    Func,
    #[token("let")]      // Rust style for variables
    Let,
    #[token("const")]    // C# style for constants
    Const,
    #[token("static")]   // C# style
    Static,
    #[token("class")]    // C# style
    Class,
    #[token("struct")]   // C# style
    Struct,
    #[token("namespace")] // C# style
    Namespace,
    
    // === Access Modifiers (C# style) ===
    #[token("public")]
    Public,
    #[token("private")]
    Private,
    #[token("protected")]
    Protected,
    
    // === Special Keywords ===
    #[token("this")]     // C# style
    This,
    #[token("void")]     // C# style
    Void,
    #[token("new")]      // C# style for object creation
    New,
    #[token("mut")]      // Rust style for explicit mutability
    Mut,
    
    // === Type Keywords (C# + Rust hybrid) ===
    #[token("i32")]
    I32Type,
    #[token("i64")]
    I64Type,
    #[token("f32")]
    F32Type,
    #[token("f64")]
    F64Type,
    #[token("string")]   // C# style (lowercase)
    StringType,
    #[token("bool")]
    BoolType,
    #[token("var")]      // C# style type inference
    VarType,
    
    // === Operators - Arithmetic ===
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
    #[token("++")]       // C# style
    PlusPlus,
    #[token("--")]       // C# style
    MinusMinus,
    
    // === Operators - Comparison ===
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
    
    // === Operators - Logical ===
    #[token("&&")]       // C# style
    AndAnd,
    #[token("||")]       // C# style
    OrOr,
    #[token("!")]
    Not,
    
    // === Operators - Bitwise ===
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
    
    // === Operators - Assignment (C# style) ===
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
    
    // === Punctuation ===
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
    #[token(",")]
    Comma,
    #[token(".")]
    Dot,
    #[token("?")]        // For ternary: condition ? then : else
    Question,
    
    // === Identifiers (must come after keywords) ===
    #[regex("[a-zA-Z_][a-zA-Z0-9_]*", |lex| lex.slice().to_string())]
    Ident(String),
    
    // === Comments (C#/C++ style) ===
    #[regex(r"//[^\n]*", logos::skip)]
    #[regex(r"/\*([^*]|\*[^/])*\*/", logos::skip)]
    Comment,
    
    // === Whitespace ===
    #[regex(r"[ \t\r\n]+", logos::skip)]
    Whitespace,
}

// Simple tokenize function
pub fn tokenize(input: &str) -> Vec<Token> {
    let mut tokens = Vec::new();
    let mut lexer = Token::lexer(input);
    
    while let Some(result) = lexer.next() {
        if let Ok(token) = result {
            tokens.push(token);
        }
    }
    
    tokens
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_basic_tokens() {
        let input = "let x = 42;";
        let tokens = tokenize(input);
        assert_eq!(tokens.len(), 5);
    }

    #[test]
    fn test_function_declaration() {
        let input = "public i32 fn add(i32 a, i32 b) { return a + b; }";
        let tokens = tokenize(input);
        assert!(tokens.len() > 10);
    }
}
