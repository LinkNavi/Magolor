// src/modules/tokenizer.rs - Enhanced with position tracking
use logos::{Logos, Lexer};

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Span {
    pub start: usize,
    pub end: usize,
    pub line: usize,
    pub column: usize,
}

impl Span {
    pub fn new(start: usize, end: usize, line: usize, column: usize) -> Self {
        Self { start, end, line, column }
    }
}

#[derive(Debug, Clone, PartialEq)]
pub struct SpannedToken {
    pub token: Token,
    pub span: Span,
}

#[derive(Logos, Debug, Clone, PartialEq)]
pub enum Token {
    // String literals with escape sequences
    #[regex(r#""([^"\\]|\\[nrt"\\])*""#, parse_string)]
    String(String),
    
    // Numeric literals with better parsing
    #[regex(r"0x[0-9a-fA-F]+", parse_hex)]
    #[regex(r"0b[01]+", parse_binary)]
    #[regex(r"[0-9]+\.[0-9]+([eE][+-]?[0-9]+)?f64", parse_f64)]
    #[regex(r"[0-9]+\.[0-9]+([eE][+-]?[0-9]+)?f32", parse_f32)]
    #[regex(r"[0-9]+\.[0-9]+([eE][+-]?[0-9]+)?", parse_default_float)]
    #[regex(r"[0-9]+i64", parse_i64)]
    #[regex(r"[0-9]+u64", parse_u64)]
    #[regex(r"[0-9]+u32", parse_u32)]
    #[regex(r"[0-9]+", parse_i32)]
    Integer(i32),
    Integer64(i64),
    UInteger(u32),
    UInteger64(u64),
    Float32(f32),
    Float64(f64),
    DefaultFloat(f32),
    
    // Character literals
    #[regex(r"'([^'\\]|\\[nrt'\\])'", parse_char)]
    Char(char),
    
    // Boolean literals
    #[token("true")]
    True,
    #[token("false")]
    False,
    
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
    #[token("trait")]
    Trait,
    #[token("impl")]
    Impl,
    #[token("namespace")]
    Namespace,
    #[token("use")]
    Use,
    #[token("as")]
    As,
    #[token("where")]
    Where,
    
    // Keywords - Memory & Safety
    #[token("ref")]
    Ref,
    #[token("deref")]
    Deref,
    #[token("move")]
    Move,
    #[token("copy")]
    Copy,
    #[token("unsafe")]
    Unsafe,
    
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
    #[token("self")]
    SelfKeyword,
    #[token("super")]
    Super,
    #[token("base")]
    Base,
    #[token("null")]
    Null,
    #[token("void")]
    Void,
    #[token("sizeof")]
    SizeOf,
    #[token("typeof")]
    TypeOf,
    
    // Type keywords
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
    #[token("char")]
    CharType,
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
    #[token("**")]
    Power,
    
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
    #[token("<=>")]
    Spaceship,
    
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
    #[token("~")]
    BitwiseNot,
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
    
    // Operators - Special
    #[token("?.")]
    SafeNav,
    #[token("??")]
    NullCoalesce,
    #[token("..")]
    Range,
    #[token("...")]
    RangeInclusive,
    
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
    #[token("@")]
    At,
    #[token("#")]
    Hash,
    #[token("$")]
    Dollar,
    
    // Identifiers (must come after keywords)
    #[regex("[a-zA-Z_][a-zA-Z0-9_]*", |lex| lex.slice().to_string())]
    Ident(String),
    
    // Comments
    #[regex(r"//[^\n]*", logos::skip)]
    #[regex(r"/\*([^*]|\*[^/])*\*/", logos::skip)]
    Comment,
    
    // Skip whitespace
    #[regex(r"[ \t\r\f]+", logos::skip)]
    #[token("\n")]
    Newline,
    

}

// Helper parsing functions
fn parse_string(lex: &mut Lexer<Token>) -> Option<String> {
    let s = lex.slice();
    let content = &s[1..s.len()-1]; // Remove quotes
    Some(content.replace("\\n", "\n")
        .replace("\\r", "\r")
        .replace("\\t", "\t")
        .replace("\\\"", "\"")
        .replace("\\\\", "\\"))
}

fn parse_char(lex: &mut Lexer<Token>) -> Option<char> {
    let s = lex.slice();
    let content = &s[1..s.len()-1]; // Remove quotes
    if content.starts_with('\\') {
        match &content[1..] {
            "n" => Some('\n'),
            "r" => Some('\r'),
            "t" => Some('\t'),
            "'" => Some('\''),
            "\\" => Some('\\'),
            _ => None,
        }
    } else {
        content.chars().next()
    }
}

fn parse_hex(lex: &mut Lexer<Token>) -> Option<i32> {
    i32::from_str_radix(&lex.slice()[2..], 16).ok()
}

fn parse_binary(lex: &mut Lexer<Token>) -> Option<i32> {
    i32::from_str_radix(&lex.slice()[2..], 2).ok()
}

fn parse_i32(lex: &mut Lexer<Token>) -> Option<i32> {
    lex.slice().parse().ok()
}

fn parse_i64(lex: &mut Lexer<Token>) -> Option<i64> {
    lex.slice().trim_end_matches("i64").parse().ok()
}

fn parse_u32(lex: &mut Lexer<Token>) -> Option<u32> {
    lex.slice().trim_end_matches("u32").parse().ok()
}

fn parse_u64(lex: &mut Lexer<Token>) -> Option<u64> {
    lex.slice().trim_end_matches("u64").parse().ok()
}

fn parse_f32(lex: &mut Lexer<Token>) -> Option<f32> {
    lex.slice().trim_end_matches("f32").parse().ok()
}

fn parse_f64(lex: &mut Lexer<Token>) -> Option<f64> {
    lex.slice().trim_end_matches("f64").parse().ok()
}

fn parse_default_float(lex: &mut Lexer<Token>) -> Option<f32> {
    lex.slice().parse().ok()
}

pub fn tokenize(input: &str) -> Vec<SpannedToken> {
    let mut tokens = Vec::new();
    let mut lexer = Token::lexer(input);
    let mut line = 1;
    let mut line_start = 0;
    
    while let Some(result) = lexer.next() {
        let span_range = lexer.span();
        let token = result.unwrap();
        
        // Count newlines before this token
        let slice = &input[line_start..span_range.start];
        line += slice.matches('\n').count();
        if let Some(last_newline) = slice.rfind('\n') {
            line_start = line_start + last_newline + 1;
        }
        
        let column = span_range.start - line_start + 1;
        
        // Handle newlines
        if token == Token::Newline {
            line += 1;
            line_start = span_range.end;
            continue; // Skip newlines in token stream
        }
        
        tokens.push(SpannedToken {
            token,
            span: Span::new(span_range.start, span_range.end, line, column),
        });
    }
    
    tokens
}

pub fn format_error(input: &str, span: Span, message: &str) -> String {
    let line_start = input[..span.start]
        .rfind('\n')
        .map(|i| i + 1)
        .unwrap_or(0);
    
    let line_end = input[span.start..]
        .find('\n')
        .map(|i| span.start + i)
        .unwrap_or(input.len());
    
    let line_text = &input[line_start..line_end];
    let col_offset = span.start - line_start;
    let error_len = (span.end - span.start).max(1);
    
    format!(
        "Error at line {}, column {}:\n{}\n{}\n{}{}",
        span.line,
        span.column,
        message,
        line_text,
        " ".repeat(col_offset),
        "^".repeat(error_len)
    )
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_position_tracking() {
        let input = "let x = 42;\nlet y = 10;";
        let tokens = tokenize(input);
        assert_eq!(tokens[0].span.line, 1);
        assert_eq!(tokens[5].span.line, 2); // "let" on second line
    }

    #[test]
    fn test_hex_binary() {
        let input = "0xFF 0b1010";
        let tokens = tokenize(input);
        assert_eq!(tokens[0].token, Token::Integer(255));
        assert_eq!(tokens[1].token, Token::Integer(10));
    }

    #[test]
    fn test_escape_sequences() {
        let input = r#""Hello\nWorld""#;
        let tokens = tokenize(input);
        if let Token::String(s) = &tokens[0].token {
            assert_eq!(s, "Hello\nWorld");
        }
    }
}
