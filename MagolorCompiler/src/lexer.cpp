#include "lexer.hpp"
#include <cctype>
#include <stdexcept>

std::unordered_map<std::string, TokenType> Lexer::keywords = {
    {"fn", TokenType::FN}, {"let", TokenType::LET}, {"return", TokenType::RETURN},
    {"if", TokenType::IF}, {"else", TokenType::ELSE}, {"while", TokenType::WHILE},
    {"for", TokenType::FOR}, {"match", TokenType::MATCH}, {"class", TokenType::CLASS},
    {"new", TokenType::NEW}, {"this", TokenType::THIS}, {"true", TokenType::TRUE},
    {"false", TokenType::FALSE}, {"None", TokenType::NONE}, {"Some", TokenType::SOME},
    {"using", TokenType::USING}, {"pub", TokenType::PUB}, {"priv", TokenType::PRIV},
    {"static", TokenType::STATIC}, {"mut", TokenType::MUT},
    {"int", TokenType::INT}, {"float", TokenType::FLOAT}, {"string", TokenType::STRING},
    {"bool", TokenType::BOOL}, {"void", TokenType::VOID}
};

Lexer::Lexer(const std::string& src) : src(src) {}

char Lexer::peek(int offset) {
    size_t idx = pos + offset;
    return idx < src.size() ? src[idx] : '\0';
}

char Lexer::advance() {
    char c = src[pos++];
    if (c == '\n') { line++; col = 1; }
    else col++;
    return c;
}

void Lexer::skipWhitespace() {
    while (pos < src.size() && (peek() == ' ' || peek() == '\t' || peek() == '\r' || peek() == '\n'))
        advance();
}

void Lexer::skipComment() {
    if (peek() == '/' && peek(1) == '/') {
        while (pos < src.size() && peek() != '\n') advance();
    }
}

Token Lexer::makeToken(TokenType t, const std::string& v) {
    return {t, v, line, col};
}

Token Lexer::string() {
    int startLine = line, startCol = col;
    advance(); // opening quote
    std::string val;
    while (pos < src.size() && peek() != '"') {
        if (peek() == '\\') {
            advance();
            char c = advance();
            switch (c) {
                case 'n': val += '\n'; break;
                case 't': val += '\t'; break;
                case '\\': val += '\\'; break;
                case '"': val += '"'; break;
                default: val += c;
            }
        } else {
            val += advance();
        }
    }
    if (pos >= src.size()) throw std::runtime_error("Unterminated string");
    advance(); // closing quote
    return {TokenType::STRING_LIT, val, startLine, startCol};
}

Token Lexer::number() {
    int startLine = line, startCol = col;
    std::string val;
    bool isFloat = false;
    while (pos < src.size() && (std::isdigit(peek()) || peek() == '.')) {
        if (peek() == '.') {
            if (isFloat) break;
            isFloat = true;
        }
        val += advance();
    }
    return {isFloat ? TokenType::FLOAT_LIT : TokenType::INT_LIT, val, startLine, startCol};
}

Token Lexer::identifier() {
    int startLine = line, startCol = col;
    std::string val;
    while (pos < src.size() && (std::isalnum(peek()) || peek() == '_'))
        val += advance();
    auto it = keywords.find(val);
    TokenType type = (it != keywords.end()) ? it->second : TokenType::IDENT;
    return {type, val, startLine, startCol};
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (pos < src.size()) {
        skipWhitespace();
        if (pos >= src.size()) break;
        
        if (peek() == '/' && peek(1) == '/') {
            skipComment();
            continue;
        }
        
        char c = peek();
        if (c == '"') { tokens.push_back(string()); continue; }
        if (std::isdigit(c)) { tokens.push_back(number()); continue; }
        if (std::isalpha(c) || c == '_') { tokens.push_back(identifier()); continue; }
        
        int startLine = line, startCol = col;
        switch (c) {
            case '+': advance(); tokens.push_back({TokenType::PLUS, "+", startLine, startCol}); break;
            case '*': advance(); tokens.push_back({TokenType::STAR, "*", startLine, startCol}); break;
            case '/': advance(); tokens.push_back({TokenType::SLASH, "/", startLine, startCol}); break;
            case '%': advance(); tokens.push_back({TokenType::PERCENT, "%", startLine, startCol}); break;
            case '(': advance(); tokens.push_back({TokenType::LPAREN, "(", startLine, startCol}); break;
            case ')': advance(); tokens.push_back({TokenType::RPAREN, ")", startLine, startCol}); break;
            case '{': advance(); tokens.push_back({TokenType::LBRACE, "{", startLine, startCol}); break;
            case '}': advance(); tokens.push_back({TokenType::RBRACE, "}", startLine, startCol}); break;
            case '[': advance(); tokens.push_back({TokenType::LBRACKET, "[", startLine, startCol}); break;
            case ']': advance(); tokens.push_back({TokenType::RBRACKET, "]", startLine, startCol}); break;
            case ',': advance(); tokens.push_back({TokenType::COMMA, ",", startLine, startCol}); break;
            case ';': advance(); tokens.push_back({TokenType::SEMICOLON, ";", startLine, startCol}); break;
            case '$': advance(); tokens.push_back({TokenType::DOLLAR, "$", startLine, startCol}); break;
            case '.': advance(); tokens.push_back({TokenType::DOT, ".", startLine, startCol}); break;
            case ':':
                advance();
                if (peek() == ':') { advance(); tokens.push_back({TokenType::DOUBLE_COLON, "::", startLine, startCol}); }
                else tokens.push_back({TokenType::COLON, ":", startLine, startCol});
                break;
            case '-':
                advance();
                if (peek() == '>') { advance(); tokens.push_back({TokenType::ARROW, "->", startLine, startCol}); }
                else tokens.push_back({TokenType::MINUS, "-", startLine, startCol});
                break;
            case '=':
                advance();
                if (peek() == '=') { advance(); tokens.push_back({TokenType::EQ, "==", startLine, startCol}); }
                else if (peek() == '>') { advance(); tokens.push_back({TokenType::FAT_ARROW, "=>", startLine, startCol}); }
                else tokens.push_back({TokenType::ASSIGN, "=", startLine, startCol});
                break;
            case '!':
                advance();
                if (peek() == '=') { advance(); tokens.push_back({TokenType::NE, "!=", startLine, startCol}); }
                else tokens.push_back({TokenType::NOT, "!", startLine, startCol});
                break;
            case '<':
                advance();
                if (peek() == '=') { advance(); tokens.push_back({TokenType::LE, "<=", startLine, startCol}); }
                else tokens.push_back({TokenType::LT, "<", startLine, startCol});
                break;
            case '>':
                advance();
                if (peek() == '=') { advance(); tokens.push_back({TokenType::GE, ">=", startLine, startCol}); }
                else tokens.push_back({TokenType::GT, ">", startLine, startCol});
                break;
            case '&':
                advance();
                if (peek() == '&') { advance(); tokens.push_back({TokenType::AND, "&&", startLine, startCol}); }
                break;
            case '|':
                advance();
                if (peek() == '|') { advance(); tokens.push_back({TokenType::OR, "||", startLine, startCol}); }
                break;
            default:
                throw std::runtime_error("Unknown character: " + std::string(1, c) + " at line " + std::to_string(line));
        }
    }
    tokens.push_back({TokenType::EOF_TOK, "", line, col});
    return tokens;
}
