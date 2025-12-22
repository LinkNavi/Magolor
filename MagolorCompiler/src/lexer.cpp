#include "lexer.hpp"
#include <cctype>

std::unordered_map<std::string, TokenType> Lexer::keywords = {
    {"fn", TokenType::FN}, {"let", TokenType::LET}, {"return", TokenType::RETURN},
    {"if", TokenType::IF}, {"else", TokenType::ELSE}, {"while", TokenType::WHILE},
    {"for", TokenType::FOR}, {"match", TokenType::MATCH}, {"class", TokenType::CLASS},
    {"new", TokenType::NEW}, {"this", TokenType::THIS}, {"true", TokenType::TRUE},
    {"false", TokenType::FALSE}, {"None", TokenType::NONE}, {"Some", TokenType::SOME},
    {"using", TokenType::USING}, {"pub", TokenType::PUB}, {"priv", TokenType::PRIV},
    {"static", TokenType::STATIC}, {"mut", TokenType::MUT}, {"cimport", TokenType::CIMPORT},
    {"int", TokenType::INT}, {"float", TokenType::FLOAT}, {"string", TokenType::STRING},
    {"bool", TokenType::BOOL}, {"void", TokenType::VOID}
};

Lexer::Lexer(const std::string& src, const std::string& filename, ErrorReporter& reporter)
    : src(src), filename(filename), reporter(reporter) {}

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

Token Lexer::makeToken(TokenType t, const std::string& v, int len) {
    return {t, v, line, col - (int)v.length(), len > 0 ? len : (int)v.length()};
}

void Lexer::error(const std::string& msg, int line, int col, int len) {
    reporter.error(msg, {filename, line, col, len});
}

Token Lexer::string() {
    int startLine = line, startCol = col;
    advance(); // opening quote
    std::string val;
    int startPos = pos;
    
    while (pos < src.size() && peek() != '"') {
        if (peek() == '\\') {
            advance();
            if (pos >= src.size()) {
                error("Unterminated string literal", startLine, startCol, pos - startPos + 1);
                return makeToken(TokenType::STRING_LIT, val);
            }
            char c = advance();
            switch (c) {
                case 'n': val += '\n'; break;
                case 't': val += '\t'; break;
                case '\\': val += '\\'; break;
                case '"': val += '"'; break;
                case 'r': val += '\r'; break;
                default: 
                    error("Unknown escape sequence: \\" + std::string(1, c), 
                          line, col - 1, 2);
                    val += c;
            }
        } else {
            val += advance();
        }
    }
    
    if (pos >= src.size()) {
        error("Unterminated string literal", startLine, startCol, pos - startPos);
        return makeToken(TokenType::STRING_LIT, val);
    }
    
    advance(); // closing quote
    return {TokenType::STRING_LIT, val, startLine, startCol, (int)(pos - startPos)};
}

Token Lexer::cppBlock() {
    int startLine = line, startCol = col;
    int startPos = pos;
    
    // Skip '@cpp'
    advance(); // @
    advance(); // c
    advance(); // p
    advance(); // p
    
    // Skip whitespace before {
    while (pos < src.size() && (peek() == ' ' || peek() == '\t' || peek() == '\n' || peek() == '\r')) {
        advance();
    }
    
    // Expect {
    if (pos >= src.size() || peek() != '{') {
        error("Expected '{' after @cpp", line, col, 1);
        return makeToken(TokenType::CPP_BLOCK, "");
    }
    advance(); // consume {
    
    int braceDepth = 1;
    std::string code;
    
    // Collect everything until matching }
    while (pos < src.size() && braceDepth > 0) {
        char c = peek();
        
        if (c == '"') {
            // Handle string literals in C++ code
            code += advance(); // opening quote
            while (pos < src.size() && peek() != '"') {
                if (peek() == '\\') {
                    code += advance(); // backslash
                    if (pos < src.size()) code += advance(); // escaped char
                } else {
                    code += advance();
                }
            }
            if (pos < src.size()) code += advance(); // closing quote
        } else if (c == '/' && peek(1) == '/') {
            // C++ line comment
            while (pos < src.size() && peek() != '\n') {
                code += advance();
            }
        } else if (c == '/' && peek(1) == '*') {
            // C++ block comment
            code += advance(); // /
            code += advance(); // *
            while (pos < src.size()) {
                if (peek() == '*' && peek(1) == '/') {
                    code += advance(); // *
                    code += advance(); // /
                    break;
                }
                code += advance();
            }
        } else if (c == '{') {
            braceDepth++;
            code += advance();
        } else if (c == '}') {
            braceDepth--;
            if (braceDepth == 0) {
                advance(); // consume final }
                break;
            }
            code += advance();
        } else {
            code += advance();
        }
    }
    
    if (braceDepth > 0) {
        error("Unterminated @cpp block", startLine, startCol, pos - startPos);
    }
    
    return {TokenType::CPP_BLOCK, code, startLine, startCol, (int)(pos - startPos)};
}

Token Lexer::number() {
    int startLine = line, startCol = col;
    std::string val;
    bool isFloat = false;
    int startPos = pos;
    
    while (pos < src.size() && (std::isdigit(peek()) || peek() == '.')) {
        if (peek() == '.') {
            if (peek(1) == '.' || !std::isdigit(peek(1))) break;
            if (isFloat) {
                error("Invalid number: multiple decimal points", 
                      startLine, startCol, pos - startPos + 1);
                break;
            }
            isFloat = true;
        }
        val += advance();
    }
    
    if (pos < src.size() && (std::isalpha(peek()) || peek() == '_')) {
        std::string suffix;
        while (pos < src.size() && (std::isalnum(peek()) || peek() == '_')) {
            suffix += advance();
        }
        error("Invalid numeric suffix: " + suffix, startLine, startCol, 
              pos - startPos);
    }
    
    return {isFloat ? TokenType::FLOAT_LIT : TokenType::INT_LIT, val, 
            startLine, startCol, (int)val.length()};
}

Token Lexer::identifier() {
    int startLine = line, startCol = col;
    std::string val;
    while (pos < src.size() && (std::isalnum(peek()) || peek() == '_'))
        val += advance();
    auto it = keywords.find(val);
    TokenType type = (it != keywords.end()) ? it->second : TokenType::IDENT;
    return {type, val, startLine, startCol, (int)val.length()};
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
        
        // Check for @cpp (MUST check all 4 characters)
        if (c == '@' && peek(1) == 'c' && peek(2) == 'p' && peek(3) == 'p' &&
            (pos + 4 >= src.size() || !std::isalnum(peek(4)))) {
            tokens.push_back(cppBlock());
            continue;
        }
        
        if (c == '"') { tokens.push_back(string()); continue; }
        if (std::isdigit(c)) { tokens.push_back(number()); continue; }
        if (std::isalpha(c) || c == '_') { tokens.push_back(identifier()); continue; }
        
        int startLine = line, startCol = col;
        switch (c) {
            case '+': advance(); tokens.push_back(makeToken(TokenType::PLUS, "+")); break;
            case '*': advance(); tokens.push_back(makeToken(TokenType::STAR, "*")); break;
            case '/': advance(); tokens.push_back(makeToken(TokenType::SLASH, "/")); break;
            case '%': advance(); tokens.push_back(makeToken(TokenType::PERCENT, "%")); break;
            case '(': advance(); tokens.push_back(makeToken(TokenType::LPAREN, "(")); break;
            case ')': advance(); tokens.push_back(makeToken(TokenType::RPAREN, ")")); break;
            case '{': advance(); tokens.push_back(makeToken(TokenType::LBRACE, "{")); break;
            case '}': advance(); tokens.push_back(makeToken(TokenType::RBRACE, "}")); break;
            case '[': advance(); tokens.push_back(makeToken(TokenType::LBRACKET, "[")); break;
            case ']': advance(); tokens.push_back(makeToken(TokenType::RBRACKET, "]")); break;
            case ',': advance(); tokens.push_back(makeToken(TokenType::COMMA, ",")); break;
            case ';': advance(); tokens.push_back(makeToken(TokenType::SEMICOLON, ";")); break;
            case '$': advance(); tokens.push_back(makeToken(TokenType::DOLLAR, "$")); break;
            case '@': advance(); tokens.push_back(makeToken(TokenType::AT, "@")); break;
            case '.': advance(); tokens.push_back(makeToken(TokenType::DOT, ".")); break;
            case ':':
                advance();
                if (peek() == ':') { 
                    advance(); 
                    tokens.push_back(makeToken(TokenType::DOUBLE_COLON, "::", 2)); 
                } else {
                    tokens.push_back(makeToken(TokenType::COLON, ":"));
                }
                break;
            case '-':
                advance();
                if (peek() == '>') { 
                    advance(); 
                    tokens.push_back(makeToken(TokenType::ARROW, "->", 2)); 
                } else {
                    tokens.push_back(makeToken(TokenType::MINUS, "-"));
                }
                break;
            case '=':
                advance();
                if (peek() == '=') { 
                    advance(); 
                    tokens.push_back(makeToken(TokenType::EQ, "==", 2)); 
                } else if (peek() == '>') { 
                    advance(); 
                    tokens.push_back(makeToken(TokenType::FAT_ARROW, "=>", 2)); 
                } else {
                    tokens.push_back(makeToken(TokenType::ASSIGN, "="));
                }
                break;
            case '!':
                advance();
                if (peek() == '=') { 
                    advance(); 
                    tokens.push_back(makeToken(TokenType::NE, "!=", 2)); 
                } else {
                    tokens.push_back(makeToken(TokenType::NOT, "!"));
                }
                break;
            case '<':
                advance();
                if (peek() == '=') { 
                    advance(); 
                    tokens.push_back(makeToken(TokenType::LE, "<=", 2)); 
                } else {
                    tokens.push_back(makeToken(TokenType::LT, "<"));
                }
                break;
            case '>':
                advance();
                if (peek() == '=') { 
                    advance(); 
                    tokens.push_back(makeToken(TokenType::GE, ">=", 2)); 
                } else {
                    tokens.push_back(makeToken(TokenType::GT, ">"));
                }
                break;
            case '&':
                advance();
                if (peek() == '&') { 
                    advance(); 
                    tokens.push_back(makeToken(TokenType::AND, "&&", 2)); 
                } else {
                    error("Unexpected character '&'", startLine, startCol, 1);
                    reporter.addNote("Did you mean '&&'?", {filename, startLine, startCol, 1});
                }
                break;
            case '|':
                advance();
                if (peek() == '|') { 
                    advance(); 
                    tokens.push_back(makeToken(TokenType::OR, "||", 2)); 
                } else {
                    error("Unexpected character '|'", startLine, startCol, 1);
                    reporter.addNote("Did you mean '||'?", {filename, startLine, startCol, 1});
                }
                break;
            default:
                error("Unknown character: '" + std::string(1, c) + "'", 
                      startLine, startCol, 1);
                advance();
        }
    }
    
    tokens.push_back({TokenType::EOF_TOK, "", line, col, 0});
    return tokens;
}
