#pragma once
#include <string>
#include <vector>
#include <unordered_map>

enum class TokenType {
    // Keywords
    FN, LET, RETURN, IF, ELSE, WHILE, FOR, MATCH, CLASS, NEW, THIS,
    TRUE, FALSE, NONE, SOME, USING, PUB, PRIV, STATIC, MUT,
    // Types
    INT, FLOAT, STRING, BOOL, VOID,
    // Literals
    INT_LIT, FLOAT_LIT, STRING_LIT, IDENT,
    // Operators
    PLUS, MINUS, STAR, SLASH, PERCENT, EQ, NE, LT, GT, LE, GE,
    AND, OR, NOT, ASSIGN, ARROW, FAT_ARROW, DOT, DOUBLE_COLON,
    // Delimiters
    LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET,
    COMMA, COLON, SEMICOLON, DOLLAR,
    // Special
    EOF_TOK, NEWLINE
};

struct Token {
    TokenType type;
    std::string value;
    int line, col;
};

class Lexer {
public:
    explicit Lexer(const std::string& src);
    std::vector<Token> tokenize();
private:
    std::string src;
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
    Token makeToken(TokenType t, const std::string& v = "");
};
