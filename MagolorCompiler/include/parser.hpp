#pragma once
#include "lexer.hpp"
#include "ast.hpp"
#include "error.hpp"
#include <vector>

class Parser {
public:
    Parser(std::vector<Token> tokens, const std::string& filename, ErrorReporter& reporter);
    Program parse();
    
private:
    std::vector<Token> tokens;
    std::string filename;
    ErrorReporter& reporter;
    size_t pos = 0;
    bool skipGenericArgs(); 
    Token peek(int offset = 0);
    Token advance();
    bool check(TokenType t);
    bool match(TokenType t);
    Token expect(TokenType t, const std::string& msg);
    
    void error(const std::string& msg);
    void error(const std::string& msg, const Token& tok);
    void errorWithHint(const std::string& msg, const Token& tok, const std::string& hint);
    void synchronize();
    
    SourceLoc tokenToLoc(const Token& tok);  // NEW: Convert token to source location
    
    Field parseField();
    
    // Declarations
    UsingDecl parseUsing();
    CImportDecl parseCImport();
    FnDecl parseFunction();
    ClassDecl parseClass();
    
    // Types
    TypePtr parseType();
    TypePtr parseFunctionType();
    
    // Statements
    StmtPtr parseStmt();
    StmtPtr parseLet();
    StmtPtr parseReturn();
    StmtPtr parseIf();
    StmtPtr parseWhile();
    StmtPtr parseFor();
    StmtPtr parseMatch();
    std::vector<StmtPtr> parseBlock();
    
    // Expressions (precedence climbing)
    ExprPtr parseExpr();
    ExprPtr parseOr();
    ExprPtr parseAnd();
    ExprPtr parseEquality();
    ExprPtr parseComparison();
    ExprPtr parseTerm();
    ExprPtr parseFactor();
    ExprPtr parseUnary();
    ExprPtr parseCall();
    ExprPtr parsePrimary();
    ExprPtr parseLambda();
    ExprPtr parseInterpolatedString(const std::string& str);
};
