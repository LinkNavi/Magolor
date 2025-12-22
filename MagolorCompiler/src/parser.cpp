#include "parser.hpp"
#include <sstream>

Parser::Parser(std::vector<Token> tokens, const std::string& filename, ErrorReporter& reporter)
    : tokens(std::move(tokens)), filename(filename), reporter(reporter) {}

Token Parser::peek(int offset) {
    size_t idx = pos + offset;
    return idx < tokens.size() ? tokens[idx] : tokens.back();
}

Token Parser::advance() { return tokens[pos++]; }
bool Parser::check(TokenType t) { return peek().type == t; }
bool Parser::match(TokenType t) {
    if (check(t)) { advance(); return true; }
    return false;
}

Token Parser::expect(TokenType t, const std::string& msg) {
    if (!check(t)) {
        error(msg, peek());
        return peek();
    }
    return advance();
}

void Parser::error(const std::string& msg) {
    Token tok = peek();
    reporter.error(msg, tok.loc(filename));
}

void Parser::error(const std::string& msg, const Token& tok) {
    reporter.error(msg, tok.loc(filename));
}

void Parser::errorWithHint(const std::string& msg, const Token& tok, const std::string& hint) {
    reporter.error(msg, tok.loc(filename), hint);
}

Program Parser::parse() {
    Program prog;
    while (!check(TokenType::EOF_TOK)) {
        if (check(TokenType::USING)) {
            prog.usings.push_back(parseUsing());
        } else if (check(TokenType::CIMPORT)) {
            prog.cimports.push_back(parseCImport());
        } else if (check(TokenType::CLASS)) {
            prog.classes.push_back(parseClass());
        } else if (check(TokenType::FN)) {
            prog.functions.push_back(parseFunction());
        } else {
            error("Unexpected token: " + peek().value, peek());
            advance(); // skip the bad token
        }
    }
    return prog;
}

UsingDecl Parser::parseUsing() {
    expect(TokenType::USING, "Expected 'using'");
    UsingDecl decl;
    Token ident = expect(TokenType::IDENT, "Expected module name");
    decl.path.push_back(ident.value);
    while (match(TokenType::DOT)) {
        ident = expect(TokenType::IDENT, "Expected module name");
        decl.path.push_back(ident.value);
    }
    expect(TokenType::SEMICOLON, "Expected ';' after using declaration");
    return decl;
}

CImportDecl Parser::parseCImport() {
    expect(TokenType::CIMPORT, "Expected 'cimport'");
    CImportDecl decl;
    
    // Check for < or " to determine system vs local header
    if (match(TokenType::LT)) {
        // System header: cimport <stdio.h>
        decl.isSystemHeader = true;
        Token headerName = expect(TokenType::IDENT, "Expected header name");
        decl.header = headerName.value;
        
        // Handle extension (e.g., .h)
        if (match(TokenType::DOT)) {
            decl.header += ".";
            Token ext = expect(TokenType::IDENT, "Expected header extension");
            decl.header += ext.value;
        }
        
        expect(TokenType::GT, "Expected '>' after system header");
    } else {
        // String literal: cimport "myheader.h"
        Token headerToken = expect(TokenType::STRING_LIT, "Expected header name in quotes or <>");
        decl.header = headerToken.value;
        decl.isSystemHeader = false;
    }
    
    // Optional: as namespace
    if (check(TokenType::IDENT) && peek().value == "as") {
        advance(); // consume 'as'
        Token ns = expect(TokenType::IDENT, "Expected namespace name after 'as'");
        decl.asNamespace = ns.value;
    }
    
    // Optional: only import specific symbols
    if (check(TokenType::LPAREN)) {
        advance();
        if (!check(TokenType::RPAREN)) {
            Token sym = expect(TokenType::IDENT, "Expected symbol name");
            decl.symbols.push_back(sym.value);
            
            while (match(TokenType::COMMA)) {
                Token nextSym = expect(TokenType::IDENT, "Expected symbol name");
                decl.symbols.push_back(nextSym.value);
            }
        }
        expect(TokenType::RPAREN, "Expected ')' after symbol list");
    }
    
    expect(TokenType::SEMICOLON, "Expected ';' after cimport");
    return decl;
}

TypePtr Parser::parseType() {
    auto type = std::make_shared<Type>();
    if (check(TokenType::FN)) return parseFunctionType();
    
    Token t = advance();
    switch (t.type) {
        case TokenType::INT: type->kind = Type::INT; break;
        case TokenType::FLOAT: type->kind = Type::FLOAT; break;
        case TokenType::STRING: type->kind = Type::STRING; break;
        case TokenType::BOOL: type->kind = Type::BOOL; break;
        case TokenType::VOID: type->kind = Type::VOID; break;
        case TokenType::IDENT:
            type->kind = Type::CLASS;
            type->className = t.value;
            break;
        default:
            error("Expected type", t);
            type->kind = Type::VOID;
    }
    return type;
}

TypePtr Parser::parseFunctionType() {
    expect(TokenType::FN, "Expected 'fn'");
    expect(TokenType::LPAREN, "Expected '('");
    
    auto type = std::make_shared<Type>();
    type->kind = Type::FUNCTION;
    
    if (!check(TokenType::RPAREN)) {
        type->paramTypes.push_back(parseType());
        while (match(TokenType::COMMA)) type->paramTypes.push_back(parseType());
    }
    expect(TokenType::RPAREN, "Expected ')'");
    expect(TokenType::ARROW, "Expected '->' in function type");
    type->returnType = parseType();
    return type;
}

FnDecl Parser::parseFunction() {
    expect(TokenType::FN, "Expected 'fn'");
    FnDecl fn;
    Token nameToken = expect(TokenType::IDENT, "Expected function name");
    fn.name = nameToken.value;
    expect(TokenType::LPAREN, "Expected '(' after function name");
    
    if (!check(TokenType::RPAREN)) {
        Param p;
        Token paramName = expect(TokenType::IDENT, "Expected parameter name");
        p.name = paramName.value;
        expect(TokenType::COLON, "Expected ':' after parameter name");
        p.type = parseType();
        fn.params.push_back(p);
        
        while (match(TokenType::COMMA)) {
            Param p2;
            Token paramName2 = expect(TokenType::IDENT, "Expected parameter name");
            p2.name = paramName2.value;
            expect(TokenType::COLON, "Expected ':' after parameter name");
            p2.type = parseType();
            fn.params.push_back(p2);
        }
    }
    expect(TokenType::RPAREN, "Expected ')' after parameters");
    
    if (match(TokenType::ARROW)) {
        fn.returnType = parseType();
    } else {
        fn.returnType = std::make_shared<Type>();
        fn.returnType->kind = Type::VOID;
    }
    
    fn.body = parseBlock();
    return fn;
}

ClassDecl Parser::parseClass() {
    expect(TokenType::CLASS, "Expected 'class'");
    ClassDecl cls;
    Token nameToken = expect(TokenType::IDENT, "Expected class name");
    cls.name = nameToken.value;
    expect(TokenType::LBRACE, "Expected '{' after class name");
    
    while (!check(TokenType::RBRACE) && !check(TokenType::EOF_TOK)) {
        bool isPublic = match(TokenType::PUB);
        if (check(TokenType::FN)) {
            FnDecl m = parseFunction();
            m.isPublic = isPublic;
            cls.methods.push_back(m);
        } else {
            Field f;
            f.isPublic = isPublic;
            Token fieldName = expect(TokenType::IDENT, "Expected field name");
            f.name = fieldName.value;
            expect(TokenType::COLON, "Expected ':' after field name");
            f.type = parseType();
            expect(TokenType::SEMICOLON, "Expected ';' after field declaration");
            cls.fields.push_back(f);
        }
    }
    expect(TokenType::RBRACE, "Expected '}' at end of class");
    return cls;
}

std::vector<StmtPtr> Parser::parseBlock() {
    expect(TokenType::LBRACE, "Expected '{'");
    std::vector<StmtPtr> stmts;
    while (!check(TokenType::RBRACE) && !check(TokenType::EOF_TOK)) {
        stmts.push_back(parseStmt());
    }
    expect(TokenType::RBRACE, "Expected '}'");
    return stmts;
}

StmtPtr Parser::parseStmt() {
    if (check(TokenType::LET)) return parseLet();
    if (check(TokenType::RETURN)) return parseReturn();
    if (check(TokenType::IF)) return parseIf();
    if (check(TokenType::WHILE)) return parseWhile();
    if (check(TokenType::FOR)) return parseFor();
    if (check(TokenType::MATCH)) return parseMatch();
       if (check(TokenType::CPP_BLOCK)) {
        auto stmt = std::make_shared<Stmt>();
        CppStmt cpp;
        cpp.code = advance().value;  // Get the C++ code
        stmt->data = cpp;
        return stmt;
    }
    auto stmt = std::make_shared<Stmt>();
    ExprStmt es;
    es.expr = parseExpr();
    stmt->data = es;
    match(TokenType::SEMICOLON);
    return stmt;
}

StmtPtr Parser::parseLet() {
    expect(TokenType::LET, "Expected 'let'");
    auto stmt = std::make_shared<Stmt>();
    LetStmt ls;
    ls.isMut = match(TokenType::MUT);
    Token varName = expect(TokenType::IDENT, "Expected variable name");
    ls.name = varName.value;
    if (match(TokenType::COLON)) ls.type = parseType();
    expect(TokenType::ASSIGN, "Expected '=' in let statement");
    ls.init = parseExpr();
    expect(TokenType::SEMICOLON, "Expected ';' after let statement");
    stmt->data = ls;
    return stmt;
}

StmtPtr Parser::parseReturn() {
    expect(TokenType::RETURN, "Expected 'return'");
    auto stmt = std::make_shared<Stmt>();
    ReturnStmt rs;
    if (!check(TokenType::SEMICOLON)) rs.value = parseExpr();
    expect(TokenType::SEMICOLON, "Expected ';' after return statement");
    stmt->data = rs;
    return stmt;
}

StmtPtr Parser::parseIf() {
    expect(TokenType::IF, "Expected 'if'");
    auto stmt = std::make_shared<Stmt>();
    IfStmt is;
    expect(TokenType::LPAREN, "Expected '(' after 'if'");
    is.cond = parseExpr();
    expect(TokenType::RPAREN, "Expected ')' after if condition");
    is.thenBody = parseBlock();
    if (match(TokenType::ELSE)) {
        if (check(TokenType::IF)) {
            is.elseBody.push_back(parseIf());
        } else {
            is.elseBody = parseBlock();
        }
    }
    stmt->data = is;
    return stmt;
}

StmtPtr Parser::parseWhile() {
    expect(TokenType::WHILE, "Expected 'while'");
    auto stmt = std::make_shared<Stmt>();
    WhileStmt ws;
    expect(TokenType::LPAREN, "Expected '(' after 'while'");
    ws.cond = parseExpr();
    expect(TokenType::RPAREN, "Expected ')' after while condition");
    ws.body = parseBlock();
    stmt->data = ws;
    return stmt;
}

StmtPtr Parser::parseFor() {
    expect(TokenType::FOR, "Expected 'for'");
    auto stmt = std::make_shared<Stmt>();
    ForStmt fs;
    expect(TokenType::LPAREN, "Expected '(' after 'for'");
    Token varName = expect(TokenType::IDENT, "Expected variable name");
    fs.var = varName.value;
    Token inToken = expect(TokenType::IDENT, "Expected 'in'");
    if (inToken.value != "in") {
        errorWithHint("Expected 'in' keyword", inToken, "use 'for (x in array)' syntax");
    }
    fs.iterable = parseExpr();
    expect(TokenType::RPAREN, "Expected ')' after for header");
    fs.body = parseBlock();
    stmt->data = fs;
    return stmt;
}

StmtPtr Parser::parseMatch() {
    expect(TokenType::MATCH, "Expected 'match'");
    auto stmt = std::make_shared<Stmt>();
    MatchStmt ms;
    ms.expr = parseExpr();
    expect(TokenType::LBRACE, "Expected '{' after match expression");
    
    while (!check(TokenType::RBRACE) && !check(TokenType::EOF_TOK)) {
        MatchArm arm;
        if (check(TokenType::SOME)) { 
            advance(); 
            arm.pattern = "Some"; 
        } else if (check(TokenType::NONE)) { 
            advance(); 
            arm.pattern = "None"; 
        } else {
            Token pattern = expect(TokenType::IDENT, "Expected pattern");
            arm.pattern = pattern.value;
        }
        
        if (match(TokenType::LPAREN)) {
            Token bindVar = expect(TokenType::IDENT, "Expected binding variable");
            arm.bindVar = bindVar.value;
            expect(TokenType::RPAREN, "Expected ')' after binding");
        }
        expect(TokenType::FAT_ARROW, "Expected '=>' in match arm");
        
        if (check(TokenType::LBRACE)) {
            arm.body = parseBlock();
        } else if (check(TokenType::RETURN)) {
            advance();
            auto retStmt = std::make_shared<Stmt>();
            ReturnStmt rs;
            if (!check(TokenType::COMMA) && !check(TokenType::RBRACE)) {
                rs.value = parseExpr();
            }
            retStmt->data = rs;
            arm.body.push_back(retStmt);
        } else {
            arm.body.push_back(parseStmt());
        }
        match(TokenType::COMMA);
        ms.arms.push_back(arm);
    }
    expect(TokenType::RBRACE, "Expected '}' at end of match");
    stmt->data = ms;
    return stmt;
}

ExprPtr Parser::parseExpr() { return parseOr(); }

ExprPtr Parser::parseOr() {
    auto left = parseAnd();
    while (match(TokenType::OR)) {
        auto expr = std::make_shared<Expr>();
        BinaryExpr be; 
        be.op = "||"; 
        be.left = left; 
        be.right = parseAnd();
        expr->data = be;
        left = expr;
    }
    return left;
}

ExprPtr Parser::parseAnd() {
    auto left = parseEquality();
    while (match(TokenType::AND)) {
        auto expr = std::make_shared<Expr>();
        BinaryExpr be; 
        be.op = "&&"; 
        be.left = left; 
        be.right = parseEquality();
        expr->data = be;
        left = expr;
    }
    return left;
}

ExprPtr Parser::parseEquality() {
    auto left = parseComparison();
    while (check(TokenType::EQ) || check(TokenType::NE)) {
        std::string op = advance().value;
        auto expr = std::make_shared<Expr>();
        BinaryExpr be; 
        be.op = op; 
        be.left = left; 
        be.right = parseComparison();
        expr->data = be;
        left = expr;
    }
    return left;
}

ExprPtr Parser::parseComparison() {
    auto left = parseTerm();
    while (check(TokenType::LT) || check(TokenType::GT) || 
           check(TokenType::LE) || check(TokenType::GE)) {
        std::string op = advance().value;
        auto expr = std::make_shared<Expr>();
        BinaryExpr be; 
        be.op = op; 
        be.left = left; 
        be.right = parseTerm();
        expr->data = be;
        left = expr;
    }
    return left;
}

ExprPtr Parser::parseTerm() {
    auto left = parseFactor();
    while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        std::string op = advance().value;
        auto expr = std::make_shared<Expr>();
        BinaryExpr be; 
        be.op = op; 
        be.left = left; 
        be.right = parseFactor();
        expr->data = be;
        left = expr;
    }
    return left;
}

ExprPtr Parser::parseFactor() {
    auto left = parseUnary();
    while (check(TokenType::STAR) || check(TokenType::SLASH) || check(TokenType::PERCENT)) {
        std::string op = advance().value;
        auto expr = std::make_shared<Expr>();
        BinaryExpr be; 
        be.op = op; 
        be.left = left; 
        be.right = parseUnary();
        expr->data = be;
        left = expr;
    }
    return left;
}

ExprPtr Parser::parseUnary() {
    if (check(TokenType::NOT) || check(TokenType::MINUS)) {
        std::string op = advance().value;
        auto expr = std::make_shared<Expr>();
        UnaryExpr ue; 
        ue.op = op; 
        ue.operand = parseUnary();
        expr->data = ue;
        return expr;
    }
    return parseCall();
}

ExprPtr Parser::parseCall() {
    auto expr = parsePrimary();
    
    while (true) {
        if (match(TokenType::LPAREN)) {
            auto callExpr = std::make_shared<Expr>();
            CallExpr ce; 
            ce.callee = expr;
            if (!check(TokenType::RPAREN)) {
                ce.args.push_back(parseExpr());
                while (match(TokenType::COMMA)) {
                    ce.args.push_back(parseExpr());
                }
            }
            expect(TokenType::RPAREN, "Expected ')' after arguments");
            callExpr->data = ce;
            expr = callExpr;
        } else if (match(TokenType::DOT)) {
            auto memberExpr = std::make_shared<Expr>();
            MemberExpr me; 
            me.object = expr;
            Token member = expect(TokenType::IDENT, "Expected member name");
            me.member = member.value;
            memberExpr->data = me;
            expr = memberExpr;
        } else if (match(TokenType::LBRACKET)) {
            auto indexExpr = std::make_shared<Expr>();
            IndexExpr ie; 
            ie.object = expr; 
            ie.index = parseExpr();
            expect(TokenType::RBRACKET, "Expected ']' after index");
            indexExpr->data = ie;
            expr = indexExpr;
        } else if (match(TokenType::DOUBLE_COLON)) {
            // Handle :: for namespaced access (e.g., Math::sqrt)
            auto memberExpr = std::make_shared<Expr>();
            MemberExpr me;
            me.object = expr;
            Token member = expect(TokenType::IDENT, "Expected function/member name after '::'");
            me.member = member.value;
            memberExpr->data = me;
            expr = memberExpr;
        } else {
            break;
        }
    }
    return expr;
}

ExprPtr Parser::parseLambda() {
    expect(TokenType::FN, "Expected 'fn'");
    expect(TokenType::LPAREN, "Expected '('");
    
    auto expr = std::make_shared<Expr>();
    LambdaExpr le;
    
    if (!check(TokenType::RPAREN)) {
        Param p;
        Token paramName = expect(TokenType::IDENT, "Expected parameter name");
        p.name = paramName.value;
        expect(TokenType::COLON, "Expected ':' after parameter");
        p.type = parseType();
        le.params.push_back(p);
        
        while (match(TokenType::COMMA)) {
            Param p2;
            Token paramName2 = expect(TokenType::IDENT, "Expected parameter name");
            p2.name = paramName2.value;
            expect(TokenType::COLON, "Expected ':' after parameter");
            p2.type = parseType();
            le.params.push_back(p2);
        }
    }
    expect(TokenType::RPAREN, "Expected ')' after parameters");
    if (match(TokenType::ARROW)) {
        le.returnType = parseType();
    }
    le.body = parseBlock();
    expr->data = le;
    return expr;
}

ExprPtr Parser::parseInterpolatedString(const std::string& str) {
    auto wrapper = std::make_shared<Expr>();
    StringLitExpr sle;
    sle.value = str;
    sle.interpolated = true;
    wrapper->data = sle;
    wrapper->type = std::make_shared<Type>();
    wrapper->type->kind = Type::STRING;
    return wrapper;
}

ExprPtr Parser::parsePrimary() {
    if (match(TokenType::DOLLAR)) {
        Token strTok = expect(TokenType::STRING_LIT, "Expected string after $");
        return parseInterpolatedString(strTok.value);
    }
    
    if (check(TokenType::FN)) return parseLambda();
    
    if (match(TokenType::TRUE)) { 
        auto e = std::make_shared<Expr>(); 
        e->data = BoolLitExpr{true}; 
        return e; 
    }
    
    if (match(TokenType::FALSE)) { 
        auto e = std::make_shared<Expr>(); 
        e->data = BoolLitExpr{false}; 
        return e; 
    }
    
    if (match(TokenType::NONE)) { 
        auto e = std::make_shared<Expr>(); 
        e->data = NoneExpr{}; 
        return e; 
    }
    
    if (match(TokenType::SOME)) {
        expect(TokenType::LPAREN, "Expected '(' after 'Some'");
        auto val = parseExpr();
        expect(TokenType::RPAREN, "Expected ')' after Some value");
        auto e = std::make_shared<Expr>(); 
        e->data = SomeExpr{val}; 
        return e;
    }
    
    if (match(TokenType::THIS)) { 
        auto e = std::make_shared<Expr>(); 
        e->data = ThisExpr{}; 
        return e; 
    }
    
    if (match(TokenType::NEW)) {
        Token className = expect(TokenType::IDENT, "Expected class name after 'new'");
        expect(TokenType::LPAREN, "Expected '(' after class name");
        std::vector<ExprPtr> args;
        if (!check(TokenType::RPAREN)) {
            args.push_back(parseExpr());
            while (match(TokenType::COMMA)) {
                args.push_back(parseExpr());
            }
        }
        expect(TokenType::RPAREN, "Expected ')' after constructor arguments");
        auto e = std::make_shared<Expr>(); 
        e->data = NewExpr{className.value, args}; 
        return e;
    }
    
    if (check(TokenType::INT_LIT)) {
        Token t = advance();
        auto e = std::make_shared<Expr>(); 
        e->data = IntLitExpr{std::stoi(t.value)}; 
        return e;
    }
    
    if (check(TokenType::FLOAT_LIT)) {
        Token t = advance();
        auto e = std::make_shared<Expr>(); 
        e->data = FloatLitExpr{std::stod(t.value)}; 
        return e;
    }
    
    if (check(TokenType::STRING_LIT)) {
        Token t = advance();
        auto e = std::make_shared<Expr>(); 
        e->data = StringLitExpr{t.value, false}; 
        return e;
    }
    
    if (check(TokenType::IDENT)) {
        Token t = advance();
        auto e = std::make_shared<Expr>(); 
        e->data = IdentExpr{t.value}; 
        return e;
    }
    
    if (match(TokenType::LPAREN)) {
        auto e = parseExpr();
        expect(TokenType::RPAREN, "Expected ')' after expression");
        return e;
    }
    
    if (match(TokenType::LBRACKET)) {
        auto e = std::make_shared<Expr>();
        ArrayExpr ae;
        if (!check(TokenType::RBRACKET)) {
            ae.elements.push_back(parseExpr());
            while (match(TokenType::COMMA)) {
                ae.elements.push_back(parseExpr());
            }
        }
        expect(TokenType::RBRACKET, "Expected ']' after array elements");
        e->data = ae;
        return e;
    }
    
    Token bad = peek();
    errorWithHint("Unexpected token in expression: " + bad.value, bad,
                  "expected a literal, identifier, or '('");
    
    // Return a dummy expression to continue parsing
    auto e = std::make_shared<Expr>();
    e->data = IntLitExpr{0};
    advance(); // skip the bad token
    return e;
}
