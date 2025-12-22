#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "error.hpp"

enum class TokenType {
    // Keywords
    FN, LET, RETURN, IF, ELSE, WHILE, FOR, MATCH, CLASS, NEW, THIS,
    TRUE, FALSE, NONE, SOME, USING, PUB, PRIV, STATIC, MUT, CIMPORT,
    // Types
    INT, FLOAT, STRING, BOOL, VOID,
    // Literals
    INT_LIT, FLOAT_LIT, STRING_LIT, IDENT,
    // Operators
    PLUS, MINUS, STAR, SLASH, PERCENT, EQ, NE, LT, GT, LE, GE,
    AND, OR, NOT, ASSIGN, ARROW, FAT_ARROW, DOT, DOUBLE_COLON,
    // Delimiters
    LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET,
    COMMA, COLON, SEMICOLON, DOLLAR, AT,
    // Special
    CPP_BLOCK,  // @cpp { ... }
    EOF_TOK, NEWLINE
};

struct Token {
    TokenType type;
    std::string value;
    int line, col;
    int length;
    
    SourceLocation loc(const std::string& file) const {
        return {file, line, col, length};
    }
};

class Lexer {
public:
    Lexer(const std::string& src, const std::string& filename, ErrorReporter& reporter);
    std::vector<Token> tokenize();
    
private:
    std::string src;
    std::string filename;
    ErrorReporter& reporter;
    size_t pos = 0;
    int line = 1, col = 1;
    static std::unordered_map<std::string, TokenType> keywords;
    
    char peek(int offset = 0);
    char advance();
    void skipWhitespace();
    void skipComment();
    Token string();
    Token number();
    Token identifier();
    Token cppBlock();  // Parse @cpp { ... }
    Token makeToken(TokenType t, const std::string& v = "", int len = 1);
    
    void error(const std::string& msg, int line, int col, int len = 1);
};
