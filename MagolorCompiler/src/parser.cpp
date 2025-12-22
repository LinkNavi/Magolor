#include "parser.hpp"
#include <sstream>

Parser::Parser(std::vector<Token> tokens) : tokens(std::move(tokens)) {}

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
    if (!check(t)) throw std::runtime_error(msg + " at line " + std::to_string(peek().line));
    return advance();
}

Program Parser::parse() {
    Program prog;
    while (!check(TokenType::EOF_TOK)) {
        if (check(TokenType::USING)) {
            prog.usings.push_back(parseUsing());
        } else if (check(TokenType::CLASS)) {
            prog.classes.push_back(parseClass());
        } else if (check(TokenType::FN)) {
            prog.functions.push_back(parseFunction());
        } else {
            throw std::runtime_error("Unexpected token: " + peek().value);
        }
    }
    return prog;
}

UsingDecl Parser::parseUsing() {
    expect(TokenType::USING, "Expected 'using'");
    UsingDecl decl;
    decl.path.push_back(expect(TokenType::IDENT, "Expected module name").value);
    while (match(TokenType::DOT)) {
        decl.path.push_back(expect(TokenType::IDENT, "Expected module name").value);
    }
    expect(TokenType::SEMICOLON, "Expected ';'");
    return decl;
}

TypePtr Parser::parseType() {
    auto type = std::make_shared<Type>();
    
    if (check(TokenType::FN)) {
        return parseFunctionType();
    }
    
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
            throw std::runtime_error("Expected type");
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
        while (match(TokenType::COMMA)) {
            type->paramTypes.push_back(parseType());
        }
    }
    expect(TokenType::RPAREN, "Expected ')'");
    expect(TokenType::ARROW, "Expected '->'");
    type->returnType = parseType();
    return type;
}

FnDecl Parser::parseFunction() {
    expect(TokenType::FN, "Expected 'fn'");
    FnDecl fn;
    fn.name = expect(TokenType::IDENT, "Expected function name").value;
    expect(TokenType::LPAREN, "Expected '('");
    
    if (!check(TokenType::RPAREN)) {
        Param p;
        p.name = expect(TokenType::IDENT, "Expected param name").value;
        expect(TokenType::COLON, "Expected ':'");
        p.type = parseType();
        fn.params.push_back(p);
        
        while (match(TokenType::COMMA)) {
            Param p2;
            p2.name = expect(TokenType::IDENT, "Expected param name").value;
            expect(TokenType::COLON, "Expected ':'");
            p2.type = parseType();
            fn.params.push_back(p2);
        }
    }
    expect(TokenType::RPAREN, "Expected ')'");
    
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
    cls.name = expect(TokenType::IDENT, "Expected class name").value;
    expect(TokenType::LBRACE, "Expected '{'");
    
    while (!check(TokenType::RBRACE) && !check(TokenType::EOF_TOK)) {
        bool isPublic = match(TokenType::PUB);
        if (check(TokenType::FN)) {
            FnDecl m = parseFunction();
            m.isPublic = isPublic;
            cls.methods.push_back(m);
        } else {
            Field f;
            f.isPublic = isPublic;
            f.name = expect(TokenType::IDENT, "Expected field name").value;
            expect(TokenType::COLON, "Expected ':'");
            f.type = parseType();
            expect(TokenType::SEMICOLON, "Expected ';'");
            cls.fields.push_back(f);
        }
    }
    expect(TokenType::RBRACE, "Expected '}'");
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
    ls.name = expect(TokenType::IDENT, "Expected variable name").value;
    
    if (match(TokenType::COLON)) {
        ls.type = parseType();
    }
    
    expect(TokenType::ASSIGN, "Expected '='");
    ls.init = parseExpr();
    expect(TokenType::SEMICOLON, "Expected ';'");
    stmt->data = ls;
    return stmt;
}

StmtPtr Parser::parseReturn() {
    expect(TokenType::RETURN, "Expected 'return'");
    auto stmt = std::make_shared<Stmt>();
    ReturnStmt rs;
    if (!check(TokenType::SEMICOLON)) {
        rs.value = parseExpr();
    }
    expect(TokenType::SEMICOLON, "Expected ';'");
    stmt->data = rs;
    return stmt;
}

StmtPtr Parser::parseIf() {
    expect(TokenType::IF, "Expected 'if'");
    auto stmt = std::make_shared<Stmt>();
    IfStmt is;
    expect(TokenType::LPAREN, "Expected '('");
    is.cond = parseExpr();
    expect(TokenType::RPAREN, "Expected ')'");
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
    expect(TokenType::LPAREN, "Expected '('");
    ws.cond = parseExpr();
    expect(TokenType::RPAREN, "Expected ')'");
    ws.body = parseBlock();
    stmt->data = ws;
    return stmt;
}

StmtPtr Parser::parseFor() {
    expect(TokenType::FOR, "Expected 'for'");
    auto stmt = std::make_shared<Stmt>();
    ForStmt fs;
    expect(TokenType::LPAREN, "Expected '('");
    fs.var = expect(TokenType::IDENT, "Expected variable").value;
    expect(TokenType::IDENT, "Expected 'in'"); // "in" keyword
    fs.iterable = parseExpr();
    expect(TokenType::RPAREN, "Expected ')'");
    fs.body = parseBlock();
    stmt->data = fs;
    return stmt;
}

StmtPtr Parser::parseMatch() {
    expect(TokenType::MATCH, "Expected 'match'");
    auto stmt = std::make_shared<Stmt>();
    MatchStmt ms;
    ms.expr = parseExpr();
    expect(TokenType::LBRACE, "Expected '{'");
    
    while (!check(TokenType::RBRACE) && !check(TokenType::EOF_TOK)) {
        MatchArm arm;
        arm.pattern = expect(TokenType::IDENT, "Expected pattern").value;
        if (match(TokenType::LPAREN)) {
            arm.bindVar = expect(TokenType::IDENT, "Expected binding var").value;
            expect(TokenType::RPAREN, "Expected ')'");
        }
        expect(TokenType::FAT_ARROW, "Expected '=>'");
        
        if (check(TokenType::LBRACE)) {
            arm.body = parseBlock();
        } else {
            arm.body.push_back(parseStmt());
        }
        if (match(TokenType::COMMA)) {} // optional trailing comma
        ms.arms.push_back(arm);
    }
    expect(TokenType::RBRACE, "Expected '}'");
    stmt->data = ms;
    return stmt;
}

// Expression parsing (precedence climbing)
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
    while (check(TokenType::LT) || check(TokenType::GT) || check(TokenType::LE) || check(TokenType::GE)) {
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
            expect(TokenType::RPAREN, "Expected ')'");
            callExpr->data = ce;
            expr = callExpr;
        } else if (match(TokenType::DOT)) {
            auto memberExpr = std::make_shared<Expr>();
            MemberExpr me;
            me.object = expr;
            me.member = expect(TokenType::IDENT, "Expected member name").value;
            memberExpr->data = me;
            expr = memberExpr;
        } else if (match(TokenType::LBRACKET)) {
            auto indexExpr = std::make_shared<Expr>();
            IndexExpr ie;
            ie.object = expr;
            ie.index = parseExpr();
            expect(TokenType::RBRACKET, "Expected ']'");
            indexExpr->data = ie;
            expr = indexExpr;
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
        p.name = expect(TokenType::IDENT, "Expected param").value;
        expect(TokenType::COLON, "Expected ':'");
        p.type = parseType();
        le.params.push_back(p);
        
        while (match(TokenType::COMMA)) {
            Param p2;
            p2.name = expect(TokenType::IDENT, "Expected param").value;
            expect(TokenType::COLON, "Expected ':'");
            p2.type = parseType();
            le.params.push_back(p2);
        }
    }
    expect(TokenType::RPAREN, "Expected ')'");
    
    if (match(TokenType::ARROW)) {
        le.returnType = parseType();
    }
    
    le.body = parseBlock();
    expr->data = le;
    return expr;
}

ExprPtr Parser::parseInterpolatedString(const std::string& str) {
    // Parse $"..." strings with {expr} interpolation
    // For simplicity, generate concatenation of parts
    std::vector<ExprPtr> parts;
    std::string current;
    size_t i = 0;
    
    while (i < str.size()) {
        if (str[i] == '{') {
            if (!current.empty()) {
                auto strExpr = std::make_shared<Expr>();
                strExpr->data = StringLitExpr{current, false};
                parts.push_back(strExpr);
                current.clear();
            }
            i++;
            std::string varName;
            while (i < str.size() && str[i] != '}') {
                varName += str[i++];
            }
            i++; // skip }
            auto varExpr = std::make_shared<Expr>();
            varExpr->data = IdentExpr{varName};
            parts.push_back(varExpr);
        } else {
            current += str[i++];
        }
    }
    
    if (!current.empty()) {
        auto strExpr = std::make_shared<Expr>();
        strExpr->data = StringLitExpr{current, false};
        parts.push_back(strExpr);
    }
    
    if (parts.empty()) {
        auto strExpr = std::make_shared<Expr>();
        strExpr->data = StringLitExpr{"", true};
        return strExpr;
    }
    
    auto result = parts[0];
    for (size_t j = 1; j < parts.size(); j++) {
        auto binExpr = std::make_shared<Expr>();
        BinaryExpr be;
        be.op = "+";
        be.left = result;
        be.right = parts[j];
        binExpr->data = be;
        result = binExpr;
    }
    
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
        // Interpolated string
        Token strTok = expect(TokenType::STRING_LIT, "Expected string after $");
        return parseInterpolatedString(strTok.value);
    }
    
    if (check(TokenType::FN)) {
        return parseLambda();
    }
    
    if (match(TokenType::TRUE)) {
        auto expr = std::make_shared<Expr>();
        expr->data = BoolLitExpr{true};
        return expr;
    }
    
    if (match(TokenType::FALSE)) {
        auto expr = std::make_shared<Expr>();
        expr->data = BoolLitExpr{false};
        return expr;
    }
    
    if (match(TokenType::NONE)) {
        auto expr = std::make_shared<Expr>();
        expr->data = NoneExpr{};
        return expr;
    }
    
    if (match(TokenType::SOME)) {
        expect(TokenType::LPAREN, "Expected '('");
        auto val = parseExpr();
        expect(TokenType::RPAREN, "Expected ')'");
        auto expr = std::make_shared<Expr>();
        expr->data = SomeExpr{val};
        return expr;
    }
    
    if (match(TokenType::THIS)) {
        auto expr = std::make_shared<Expr>();
        expr->data = ThisExpr{};
        return expr;
    }
    
    if (match(TokenType::NEW)) {
        std::string className = expect(TokenType::IDENT, "Expected class name").value;
        expect(TokenType::LPAREN, "Expected '('");
        std::vector<ExprPtr> args;
        if (!check(TokenType::RPAREN)) {
            args.push_back(parseExpr());
            while (match(TokenType::COMMA)) {
                args.push_back(parseExpr());
            }
        }
        expect(TokenType::RPAREN, "Expected ')'");
        auto expr = std::make_shared<Expr>();
        expr->data = NewExpr{className, args};
        return expr;
    }
    
    if (check(TokenType::INT_LIT)) {
        Token t = advance();
        auto expr = std::make_shared<Expr>();
        expr->data = IntLitExpr{std::stoi(t.value)};
        return expr;
    }
    
    if (check(TokenType::FLOAT_LIT)) {
        Token t = advance();
        auto expr = std::make_shared<Expr>();
        expr->data = FloatLitExpr{std::stod(t.value)};
        return expr;
    }
    
    if (check(TokenType::STRING_LIT)) {
        Token t = advance();
        auto expr = std::make_shared<Expr>();
        expr->data = StringLitExpr{t.value, false};
        return expr;
    }
    
    if (check(TokenType::IDENT)) {
        Token t = advance();
        auto expr = std::make_shared<Expr>();
        expr->data = IdentExpr{t.value};
        return expr;
    }
    
    if (match(TokenType::LPAREN)) {
        auto expr = parseExpr();
        expect(TokenType::RPAREN, "Expected ')'");
        return expr;
    }
    
    if (match(TokenType::LBRACKET)) {
        auto expr = std::make_shared<Expr>();
        ArrayExpr ae;
        if (!check(TokenType::RBRACKET)) {
            ae.elements.push_back(parseExpr());
            while (match(TokenType::COMMA)) {
                ae.elements.push_back(parseExpr());
            }
        }
        expect(TokenType::RBRACKET, "Expected ']'");
        expr->data = ae;
        return expr;
    }
    
    throw std::runtime_error("Unexpected token in expression: " + peek().value);
}
