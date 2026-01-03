#include "parser.hpp"
#include <sstream>

Parser::Parser(std::vector<Token> tokens, const std::string &filename,
               ErrorReporter &reporter)
    : tokens(std::move(tokens)), filename(filename), reporter(reporter) {}

Token Parser::peek(int offset) {
  size_t idx = pos + offset;
  return idx < tokens.size() ? tokens[idx] : tokens.back();
}

Token Parser::advance() { return tokens[pos++]; }
bool Parser::check(TokenType t) { return peek().type == t; }
bool Parser::match(TokenType t) {
  if (check(t)) {
    advance();
    return true;
  }
  return false;
}
SourceLoc Parser::tokenToLoc(const Token &tok) {
  SourceLoc loc;
  loc.line = tok.line;
  loc.col = tok.col;
  loc.length = tok.length;
  return loc;
}
Token Parser::expect(TokenType t, const std::string &msg) {
  if (!check(t)) {
    error(msg, peek());
    return peek();
  }
  return advance();
}

void Parser::error(const std::string &msg) {
  Token tok = peek();
  reporter.error(msg, tok.loc(filename));
}

void Parser::error(const std::string &msg, const Token &tok) {
  reporter.error(msg, tok.loc(filename));
}

void Parser::errorWithHint(const std::string &msg, const Token &tok,
                           const std::string &hint) {
  reporter.error(msg, tok.loc(filename), hint);
}

void Parser::synchronize() {
  advance();

  while (!check(TokenType::EOF_TOK)) {
    if (check(TokenType::SEMICOLON)) {
      advance();
      return;
    }

    if (check(TokenType::FN) || check(TokenType::CLASS) ||
        check(TokenType::USING) || check(TokenType::CIMPORT)) {
      return;
    }

    if (check(TokenType::RBRACE)) {
      return;
    }

    advance();
  }
}

Program Parser::parse() {
  Program prog;
  while (!check(TokenType::EOF_TOK)) {
    try {
      if (check(TokenType::USING)) {
        prog.usings.push_back(parseUsing());
      } else if (check(TokenType::CIMPORT)) {
        prog.cimports.push_back(parseCImport());
      } else if (check(TokenType::CLASS)) {
        prog.classes.push_back(parseClass());
      } else if (check(TokenType::PUB) || check(TokenType::FN)) {
        prog.functions.push_back(parseFunction());
      } else {
        error("Unexpected token: " + peek().value, peek());
        synchronize();
      }
    } catch (const std::exception &e) {
      synchronize();
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

  if (match(TokenType::LT)) {
    // System header like <stdio.h>
    decl.isSystemHeader = true;

    // Read header name - could be multiple tokens
    std::string headerName;

    // Read identifier(s) and dots until we hit >
    while (!check(TokenType::GT) && !check(TokenType::EOF_TOK)) {
      if (check(TokenType::IDENT)) {
        Token part = advance();
        headerName += part.value;
      } else if (check(TokenType::DOT)) {
        advance();
        headerName += ".";
      } else if (check(TokenType::MINUS)) {
        // For headers like <sys-types.h>
        advance();
        headerName += "-";
      } else {
        error("Unexpected token in header name", peek());
        break;
      }
    }

    decl.header = headerName;
    expect(TokenType::GT, "Expected '>' after system header");
  } else if (check(TokenType::STRING_LIT)) {
    // Local header like "myheader.h"
    Token headerToken = advance();
    decl.header = headerToken.value;
    decl.isSystemHeader = false;
  } else {
    error("Expected header name in <brackets> or \"quotes\"", peek());
  }

  // Check for namespace alias (as Name)
  if (check(TokenType::IDENT) && peek().value == "as") {
    advance(); // consume 'as'
    Token ns = expect(TokenType::IDENT, "Expected namespace name after 'as'");
    decl.asNamespace = ns.value;
  }

  // Check for specific symbol imports (symbol1, symbol2)
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
  if (check(TokenType::FN))
    return parseFunctionType();

  Token t = advance();
  switch (t.type) {
  case TokenType::INT:
    type->kind = Type::INT;
    break;
  case TokenType::FLOAT:
    type->kind = Type::FLOAT;
    break;
  case TokenType::STRING:
    type->kind = Type::STRING;
    break;
  case TokenType::BOOL:
    type->kind = Type::BOOL;
    break;
  case TokenType::VOID:
    type->kind = Type::VOID;
    break;
  case TokenType::IDENT:
    type->kind = Type::CLASS;
    type->className = t.value;

    // NEW: Check for generic arguments
    if (check(TokenType::LT)) {
      advance(); // consume '<'

      // Parse first generic argument
      type->genericArgs.push_back(parseType());

      // Parse additional generic arguments
      while (match(TokenType::COMMA)) {
        type->genericArgs.push_back(parseType());
      }

      expect(TokenType::GT, "Expected '>' after generic arguments");

      // Handle special generic types
      if (type->className == "Option" && type->genericArgs.size() == 1) {
        type->kind = Type::OPTION;
        type->innerType = type->genericArgs[0];
        type->genericArgs.clear();
      } else if (type->className == "Array" && type->genericArgs.size() == 1) {
        type->kind = Type::ARRAY;
        type->innerType = type->genericArgs[0];
        type->genericArgs.clear();
      } else {
        // Keep as generic class type
        type->kind = Type::GENERIC;
      }
    }
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
    while (match(TokenType::COMMA))
      type->paramTypes.push_back(parseType());
  }
  expect(TokenType::RPAREN, "Expected ')'");
  expect(TokenType::ARROW, "Expected '->' in function type");
  type->returnType = parseType();
  return type;
}

Field Parser::parseField() {
  Field field;

  field.isPublic = false;
  if (check(TokenType::PUB)) {
    field.isPublic = true;
    advance();
  }

  field.isStatic = false;
  if (check(TokenType::STATIC)) {
    field.isStatic = true;
    advance();
  }

  Token nameToken = expect(TokenType::IDENT, "Expected field name");
  field.name = nameToken.value;

  expect(TokenType::COLON, "Expected ':' after field name");
  field.type = parseType();

  if (check(TokenType::ASSIGN)) {
    advance();
    field.initValue = parseExpr();
  }

  expect(TokenType::SEMICOLON, "Expected ';' after field declaration");
  return field;
}

FnDecl Parser::parseFunction() {
  FnDecl fn;

  fn.isPublic = false;
  if (check(TokenType::PUB)) {
    fn.isPublic = true;
    advance();
  }

  fn.isStatic = false;
  if (check(TokenType::STATIC)) {
    fn.isStatic = true;
    advance();
  }

  expect(TokenType::FN, "Expected 'fn'");
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
    bool isStatic = match(TokenType::STATIC);

    if (check(TokenType::FN)) {
      FnDecl m = parseFunction();
      m.isPublic = isPublic;
      m.isStatic = isStatic;
      cls.methods.push_back(m);
    } else {
      Field f;
      f.isPublic = isPublic;
      f.isStatic = isStatic;
      Token fieldName = expect(TokenType::IDENT, "Expected field name");
      f.name = fieldName.value;
      expect(TokenType::COLON, "Expected ':' after field name");
      f.type = parseType();

      if (check(TokenType::ASSIGN)) {
        advance();
        f.initValue = parseExpr();
      }

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
  if (check(TokenType::LET))
    return parseLet();
  if (check(TokenType::RETURN))
    return parseReturn();
  if (check(TokenType::IF))
    return parseIf();
  if (check(TokenType::WHILE))
    return parseWhile();
  if (check(TokenType::FOR))
    return parseFor();
  if (check(TokenType::MATCH))
    return parseMatch();
  if (check(TokenType::CPP_BLOCK)) {
    auto stmt = std::make_shared<Stmt>();
    CppStmt cpp;
    cpp.code = advance().value;
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
  if (match(TokenType::COLON))
    ls.type = parseType();
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
  if (!check(TokenType::SEMICOLON))
    rs.value = parseExpr();
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
    errorWithHint("Expected 'in' keyword", inToken,
                  "use 'for (x in array)' syntax");
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

// FIX: Handle assignment
ExprPtr Parser::parseExpr() {
  auto left = parseOr();

  // Check for assignment
  if (check(TokenType::ASSIGN)) {
    // Validate l-value
    bool isLValue = std::holds_alternative<IdentExpr>(left->data) ||
                    std::holds_alternative<MemberExpr>(left->data) ||
                    std::holds_alternative<IndexExpr>(left->data);

    if (!isLValue) {
      error("Cannot assign to non-variable expression", peek());
      advance();
      parseExpr();
      return left;
    }

    advance();
    auto expr = std::make_shared<Expr>();
    AssignExpr ae;
    ae.target = left;
    ae.value = parseExpr();
    expr->data = ae;
    return expr;
  }

  return left;
}

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
  while (check(TokenType::LT) || check(TokenType::GT) || check(TokenType::LE) ||
         check(TokenType::GE)) {
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
  while (check(TokenType::STAR) || check(TokenType::SLASH) ||
         check(TokenType::PERCENT)) {
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
bool Parser::skipGenericArgs() {
  if (!check(TokenType::LT))
    return false;

  // Save position in case this isn't generic args
  int savedPos = pos;
  advance(); // consume '<'

  int depth = 1;
  bool foundComma = false;
  bool foundType = false;

  while (depth > 0 && pos < tokens.size()) {
    TokenType t = peek().type;

    if (t == TokenType::LT) {
      depth++;
      advance();
    } else if (t == TokenType::GT) {
      depth--;
      advance();
      if (depth == 0) {
        // Successfully found closing '>'
        // Must be followed by '(' for method call
        if (check(TokenType::LPAREN)) {
          return true;
        }
        break;
      }
    } else if (t == TokenType::COMMA) {
      foundComma = true;
      advance();
    } else if (t == TokenType::IDENT || t == TokenType::INT ||
               t == TokenType::FLOAT || t == TokenType::STRING ||
               t == TokenType::BOOL || t == TokenType::VOID) {
      foundType = true;
      advance();
    } else if (t == TokenType::ASSIGN || t == TokenType::SEMICOLON ||
               t == TokenType::EOF_TOK) {
      // Definitely not generic args
      break;
    } else {
      // Unexpected token
      advance();
    }
  }

  // Not generic args, restore position
  pos = savedPos;
  return false;
}
ExprPtr Parser::parseCall() {
  auto expr = parsePrimary();

  while (true) {
    if (match(TokenType::LPAREN)) {
      Token tok = tokens[pos - 1];
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
      callExpr->loc = expr->loc;
      expr = callExpr;

    } else if (match(TokenType::DOT)) {
      Token memberTok = expect(TokenType::IDENT, "Expected member name");
      auto memberExpr = std::make_shared<Expr>();
      MemberExpr me;
      me.object = expr;
      me.member = memberTok.value;
      memberExpr->data = me;
      memberExpr->loc = expr->loc;
      expr = memberExpr;

      // NEW: Check for generic arguments AFTER member access
      // This handles cases like: Array.create<int>()
      if (check(TokenType::LT)) {
        skipGenericArgs();
        // The '(' will be handled in the next iteration
      }

    } else if (match(TokenType::LBRACKET)) {
      auto indexExpr = std::make_shared<Expr>();
      IndexExpr ie;
      ie.object = expr;
      ie.index = parseExpr();
      expect(TokenType::RBRACKET, "Expected ']' after index");
      indexExpr->data = ie;
      indexExpr->loc = expr->loc;
      expr = indexExpr;

    } else if (match(TokenType::DOUBLE_COLON)) {
      Token memberTok =
          expect(TokenType::IDENT, "Expected function/member name after '::'");
      auto memberExpr = std::make_shared<Expr>();
      MemberExpr me;
      me.object = expr;
      me.member = memberTok.value;
      memberExpr->data = me;
      memberExpr->loc = expr->loc;
      expr = memberExpr;

      // NEW: Check for generic arguments AFTER :: access
      if (check(TokenType::LT)) {
        skipGenericArgs();
      }

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

    // Type annotation is now OPTIONAL
    if (match(TokenType::COLON)) {
      p.type = parseType();
    } else {
      // No type specified - create an auto type for type inference
      p.type = std::make_shared<Type>();
      p.type->kind = Type::VOID; // Will be inferred by type checker
    }
    le.params.push_back(p);

    while (match(TokenType::COMMA)) {
      Param p2;
      Token paramName2 = expect(TokenType::IDENT, "Expected parameter name");
      p2.name = paramName2.value;

      // Type annotation is now OPTIONAL
      if (match(TokenType::COLON)) {
        p2.type = parseType();
      } else {
        // No type specified - create an auto type for type inference
        p2.type = std::make_shared<Type>();
        p2.type->kind = Type::VOID; // Will be inferred by type checker
      }
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

ExprPtr Parser::parseInterpolatedString(const std::string &str) {
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
    auto expr = parseInterpolatedString(strTok.value);
    expr->loc = tokenToLoc(strTok);
    return expr;
  }

  if (check(TokenType::FN))
    return parseLambda();

  if (match(TokenType::TRUE)) {
    Token tok = tokens[pos - 1];
    auto e = std::make_shared<Expr>();
    e->data = BoolLitExpr{true};
    e->loc = tokenToLoc(tok);
    return e;
  }

  if (match(TokenType::FALSE)) {
    Token tok = tokens[pos - 1];
    auto e = std::make_shared<Expr>();
    e->data = BoolLitExpr{false};
    e->loc = tokenToLoc(tok);
    return e;
  }

  if (match(TokenType::NONE)) {
    Token tok = tokens[pos - 1];
    auto e = std::make_shared<Expr>();
    e->data = NoneExpr{};
    e->loc = tokenToLoc(tok);
    return e;
  }
  if (check(TokenType::IDENT)) {
    Token t = advance();
    auto e = std::make_shared<Expr>();
    e->data = IdentExpr{t.value};
    e->loc = tokenToLoc(t);

    // IMPORTANT: Check if this is followed by a dot - if so, DON'T treat it as
    // a call This allows Network.HTTP to be parsed as a namespace path The
    // parseCall() function will handle the member access

    return e;
  }
  if (match(TokenType::SOME)) {
    Token tok = tokens[pos - 1];
    expect(TokenType::LPAREN, "Expected '(' after 'Some'");
    auto val = parseExpr();
    expect(TokenType::RPAREN, "Expected ')' after Some value");
    auto e = std::make_shared<Expr>();
    e->data = SomeExpr{val};
    e->loc = tokenToLoc(tok);
    return e;
  }

  if (match(TokenType::THIS)) {
    Token tok = tokens[pos - 1];
    auto e = std::make_shared<Expr>();
    e->data = ThisExpr{};
    e->loc = tokenToLoc(tok);
    return e;
  }

  if (match(TokenType::NEW)) {
    Token tok = tokens[pos - 1];
    Token className =
        expect(TokenType::IDENT, "Expected class name after 'new'");
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
    e->loc = tokenToLoc(tok);
    return e;
  }

  if (check(TokenType::INT_LIT)) {
    Token t = advance();
    auto e = std::make_shared<Expr>();
    e->data = IntLitExpr{std::stoi(t.value)};
    e->loc = tokenToLoc(t);
    return e;
  }

  if (check(TokenType::FLOAT_LIT)) {
    Token t = advance();
    auto e = std::make_shared<Expr>();
    e->data = FloatLitExpr{std::stod(t.value)};
    e->loc = tokenToLoc(t);
    return e;
  }

  if (check(TokenType::STRING_LIT)) {
    Token t = advance();
    auto e = std::make_shared<Expr>();
    e->data = StringLitExpr{t.value, false};
    e->loc = tokenToLoc(t);
    return e;
  }

  if (check(TokenType::IDENT)) {
    Token t = advance();
    auto e = std::make_shared<Expr>();
    e->data = IdentExpr{t.value};
    e->loc = tokenToLoc(t);
    return e;
  }

  if (match(TokenType::LPAREN)) {
    Token tok = tokens[pos - 1];
    auto e = parseExpr();
    expect(TokenType::RPAREN, "Expected ')' after expression");
    e->loc = tokenToLoc(tok);
    return e;
  }

  if (match(TokenType::LBRACKET)) {
    Token tok = tokens[pos - 1];
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
    e->loc = tokenToLoc(tok);
    return e;
  }

  Token bad = peek();
  errorWithHint("Unexpected token in expression: " + bad.value, bad,
                "expected a literal, identifier, or '('");

  auto e = std::make_shared<Expr>();
  e->data = IntLitExpr{0};
  e->loc = tokenToLoc(bad);
  advance();
  return e;
}
