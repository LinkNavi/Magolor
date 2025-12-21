// src/modules/statement_parser.rs - Clean, error-free statement parser

use crate::modules::tokenizer::Token;
use crate::modules::ast::{Statement, ASTValue, Type, BinaryOp};

pub struct StatementParser<'a> {
    tokens: &'a [Token],
    pos: usize,
}

impl<'a> StatementParser<'a> {
    pub fn new(tokens: &'a [Token], start_pos: usize) -> Self {
        Self { tokens, pos: start_pos }
    }

    pub fn current_pos(&self) -> usize {
        self.pos
    }

    fn peek(&self) -> Option<&Token> {
        self.tokens.get(self.pos)
    }

    fn advance(&mut self) -> Option<&Token> {
        let token = self.tokens.get(self.pos);
        if token.is_some() {
            self.pos += 1;
        }
        token
    }

    fn expect(&mut self, expected: Token) -> Result<(), String> {
        match self.peek() {
            Some(token) if std::mem::discriminant(token) == std::mem::discriminant(&expected) => {
                self.advance();
                Ok(())
            }
            Some(token) => Err(format!("Expected {:?}, found {:?}", expected, token)),
            None => Err(format!("Expected {:?}, found end of input", expected)),
        }
    }

    pub fn parse_statement(&mut self) -> Result<Option<Statement>, String> {
        match self.peek() {
            Some(Token::Let) | Some(Token::Const) => self.parse_var_decl(),
            Some(Token::If) => self.parse_if_statement(),
            Some(Token::While) => self.parse_while_statement(),
            Some(Token::For) => self.parse_for_statement(),
            Some(Token::ForEach) => self.parse_foreach_statement(),
            Some(Token::Return) => self.parse_return_statement(),
            Some(Token::Break) => {
                self.advance();
                self.expect(Token::Semicolon)?;
                Ok(Some(Statement::Break))
            }
            Some(Token::Continue) => {
                self.advance();
                self.expect(Token::Semicolon)?;
                Ok(Some(Statement::Continue))
            }
            Some(Token::LBrace) => self.parse_block(),
            Some(Token::Semicolon) => {
                self.advance();
                Ok(None)
            }
            _ => self.parse_expression_or_assignment(),
        }
    }

    fn parse_type(&mut self) -> Result<Type, String> {
        let base_type = match self.peek() {
            Some(Token::I32Type) => { self.advance(); Type::I32 }
            Some(Token::I64Type) => { self.advance(); Type::I64 }
            Some(Token::F32Type) => { self.advance(); Type::F32 }
            Some(Token::F64Type) => { self.advance(); Type::F64 }
            Some(Token::StringType) => { self.advance(); Type::String }
            Some(Token::BoolType) => { self.advance(); Type::Bool }
            Some(Token::Void) => { self.advance(); Type::Void }
            Some(Token::VarType) => { self.advance(); Type::Void } // Type inference -> defaults to void
            Some(Token::Ident(name)) => {
                let type_name = name.clone();
                self.advance();
                Type::Custom(type_name)
            }
            Some(token) => return Err(format!("Expected type, found {:?}", token)),
            None => return Err("Expected type, found end of input".to_string()),
        };

        // Check for array type: Type[]
        if matches!(self.peek(), Some(Token::LBracket)) {
            self.advance();
            self.expect(Token::RBracket)?;
            Ok(Type::Array(Box::new(base_type)))
        } else {
            Ok(base_type)
        }
    }

    fn parse_var_decl(&mut self) -> Result<Option<Statement>, String> {
        let is_const = matches!(self.peek(), Some(Token::Const));
        self.advance(); // Skip 'let' or 'const'

        let var_type = self.parse_type()?;
        
        let name = if let Some(Token::Ident(n)) = self.peek() {
            let name = n.clone();
            self.advance();
            name
        } else {
            return Err("Expected variable name after type".to_string());
        };

        let value = if matches!(self.peek(), Some(Token::Eq)) {
            self.advance();
            let expr = self.parse_expression()?;
            Some(expr)
        } else {
            None
        };

        self.expect(Token::Semicolon)?;

        Ok(Some(Statement::VarDecl {
            name,
            var_type,
            value,
            is_mutable: !is_const,
        }))
    }

    fn parse_if_statement(&mut self) -> Result<Option<Statement>, String> {
        self.advance(); // Skip 'if'

        let condition = self.parse_expression()?;
        let then_body = self.parse_block_or_statement()?;

        let mut elif_branches = Vec::new();
        while matches!(self.peek(), Some(Token::Elif)) {
            self.advance();
            let elif_condition = self.parse_expression()?;
            let elif_body = self.parse_block_or_statement()?;
            elif_branches.push((elif_condition, elif_body));
        }

        let else_body = if matches!(self.peek(), Some(Token::Else)) {
            self.advance();
            Some(self.parse_block_or_statement()?)
        } else {
            None
        };

        Ok(Some(Statement::If {
            condition,
            then_body,
            elif_branches,
            else_body,
        }))
    }

    fn parse_while_statement(&mut self) -> Result<Option<Statement>, String> {
        self.advance(); // Skip 'while'
        let condition = self.parse_expression()?;
        let body = self.parse_block_or_statement()?;

        Ok(Some(Statement::While { condition, body }))
    }

    fn parse_for_statement(&mut self) -> Result<Option<Statement>, String> {
        self.advance(); // Skip 'for'
        self.expect(Token::LParen)?;

        let init = if matches!(self.peek(), Some(Token::Semicolon)) {
            self.advance();
            None
        } else {
            let stmt = self.parse_statement()?;
            stmt.map(Box::new)
        };

        let condition = if matches!(self.peek(), Some(Token::Semicolon)) {
            self.advance();
            None
        } else {
            let cond = self.parse_expression()?;
            self.expect(Token::Semicolon)?;
            Some(cond)
        };

        let increment = if matches!(self.peek(), Some(Token::RParen)) {
            None
        } else {
            let expr = self.parse_expression()?;
            Some(Box::new(Statement::Expression(expr)))
        };

        self.expect(Token::RParen)?;

        let body = self.parse_block_or_statement()?;

        Ok(Some(Statement::For {
            init,
            condition,
            increment,
            body,
        }))
    }

    fn parse_foreach_statement(&mut self) -> Result<Option<Statement>, String> {
        self.advance(); // Skip 'foreach'
        self.expect(Token::LParen)?;

        let item_type = self.parse_type()?;
        
        let item = if let Some(Token::Ident(n)) = self.peek() {
            let name = n.clone();
            self.advance();
            name
        } else {
            return Err("Expected item name in foreach".to_string());
        };
        
        self.expect(Token::In)?;
        
        let collection = self.parse_expression()?;
        
        self.expect(Token::RParen)?;
        
        let body = self.parse_block_or_statement()?;
        
        Ok(Some(Statement::ForEach {
            item,
            item_type,
            collection,
            body,
        }))
    }

    fn parse_return_statement(&mut self) -> Result<Option<Statement>, String> {
        self.advance(); // Skip 'return'

        let value = if matches!(self.peek(), Some(Token::Semicolon)) {
            None
        } else {
            Some(self.parse_expression()?)
        };

        self.expect(Token::Semicolon)?;
        Ok(Some(Statement::Return(value)))
    }

    fn parse_block(&mut self) -> Result<Option<Statement>, String> {
        let statements = self.parse_block_statements()?;
        Ok(Some(Statement::Block(statements)))
    }

    fn parse_block_statements(&mut self) -> Result<Vec<Statement>, String> {
        self.expect(Token::LBrace)?;
        
        let mut statements = Vec::new();
        
        while !matches!(self.peek(), Some(Token::RBrace)) {
            if let Some(stmt) = self.parse_statement()? {
                statements.push(stmt);
            }
        }
        
        self.expect(Token::RBrace)?;
        Ok(statements)
    }

    fn parse_block_or_statement(&mut self) -> Result<Vec<Statement>, String> {
        if matches!(self.peek(), Some(Token::LBrace)) {
            self.parse_block_statements()
        } else {
            let stmt = self.parse_statement()?;
            Ok(if let Some(s) = stmt { vec![s] } else { Vec::new() })
        }
    }

    fn parse_expression_or_assignment(&mut self) -> Result<Option<Statement>, String> {
        let expr = self.parse_expression()?;

        match self.peek() {
            Some(Token::Eq) => {
                self.advance();
                let value = self.parse_expression()?;
                self.expect(Token::Semicolon)?;
                Ok(Some(Statement::Assignment { target: expr, value }))
            }
            Some(Token::PlusEq) | Some(Token::MinusEq) | Some(Token::StarEq) 
            | Some(Token::SlashEq) | Some(Token::PercentEq) => {
                let op = match self.peek().unwrap() {
                    Token::PlusEq => BinaryOp::Add,
                    Token::MinusEq => BinaryOp::Sub,
                    Token::StarEq => BinaryOp::Mul,
                    Token::SlashEq => BinaryOp::Div,
                    Token::PercentEq => BinaryOp::Mod,
                    _ => unreachable!(),
                };
                self.advance();
                let value = self.parse_expression()?;
                self.expect(Token::Semicolon)?;
                Ok(Some(Statement::CompoundAssignment { target: expr, op, value }))
            }
            _ => {
                self.expect(Token::Semicolon)?;
                Ok(Some(Statement::Expression(expr)))
            }
        }
    }

    // Expression parsing - delegates to expression_parser
    fn parse_expression(&mut self) -> Result<ASTValue, String> {
        let mut expr_parser = crate::modules::expression_parser::ExpressionParser::new(self.tokens, self.pos);
        let result = expr_parser.parse_expression()?;
        self.pos = expr_parser.current_pos();
        Ok(result)
    }
}
