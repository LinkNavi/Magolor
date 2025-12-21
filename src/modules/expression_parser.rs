// src/modules/expression_parser.rs - Clean, error-free expression parser

use crate::modules::tokenizer::Token;
use crate::modules::ast::{ASTValue, BinaryOp, UnaryOp, ComparisonOp};

pub struct ExpressionParser<'a> {
    tokens: &'a [Token],
    pos: usize,
}

impl<'a> ExpressionParser<'a> {
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

    pub fn parse_expression(&mut self) -> Result<ASTValue, String> {
        self.parse_ternary()
    }

    fn parse_ternary(&mut self) -> Result<ASTValue, String> {
        let mut expr = self.parse_logical_or()?;

        if matches!(self.peek(), Some(Token::Question)) {
            self.advance();
            let then_val = self.parse_expression()?;
            self.expect(Token::Colon)?;
            let else_val = self.parse_expression()?;
            expr = ASTValue::Ternary {
                condition: Box::new(expr),
                then_val: Box::new(then_val),
                else_val: Box::new(else_val),
            };
        }

        Ok(expr)
    }

    fn parse_logical_or(&mut self) -> Result<ASTValue, String> {
        let mut left = self.parse_logical_and()?;

        while matches!(self.peek(), Some(Token::OrOr)) {
            self.advance();
            let right = self.parse_logical_and()?;
            left = ASTValue::Binary {
                op: BinaryOp::Or,
                left: Box::new(left),
                right: Box::new(right),
            };
        }

        Ok(left)
    }

    fn parse_logical_and(&mut self) -> Result<ASTValue, String> {
        let mut left = self.parse_bitwise_or()?;

        while matches!(self.peek(), Some(Token::AndAnd)) {
            self.advance();
            let right = self.parse_bitwise_or()?;
            left = ASTValue::Binary {
                op: BinaryOp::And,
                left: Box::new(left),
                right: Box::new(right),
            };
        }

        Ok(left)
    }

    fn parse_bitwise_or(&mut self) -> Result<ASTValue, String> {
        let mut left = self.parse_bitwise_xor()?;

        while matches!(self.peek(), Some(Token::Or)) {
            self.advance();
            let right = self.parse_bitwise_xor()?;
            left = ASTValue::Binary {
                op: BinaryOp::BitOr,
                left: Box::new(left),
                right: Box::new(right),
            };
        }

        Ok(left)
    }

    fn parse_bitwise_xor(&mut self) -> Result<ASTValue, String> {
        let mut left = self.parse_bitwise_and()?;

        while matches!(self.peek(), Some(Token::Xor)) {
            self.advance();
            let right = self.parse_bitwise_and()?;
            left = ASTValue::Binary {
                op: BinaryOp::BitXor,
                left: Box::new(left),
                right: Box::new(right),
            };
        }

        Ok(left)
    }

    fn parse_bitwise_and(&mut self) -> Result<ASTValue, String> {
        let mut left = self.parse_comparison()?;

        while matches!(self.peek(), Some(Token::And)) {
            self.advance();
            let right = self.parse_comparison()?;
            left = ASTValue::Binary {
                op: BinaryOp::BitAnd,
                left: Box::new(left),
                right: Box::new(right),
            };
        }

        Ok(left)
    }

    fn parse_comparison(&mut self) -> Result<ASTValue, String> {
        let mut left = self.parse_shift()?;

        while let Some(token) = self.peek() {
            let op = match token {
                Token::EqEq => ComparisonOp::Equal,
                Token::NotEq => ComparisonOp::NotEqual,
                Token::Less => ComparisonOp::Less,
                Token::LessEq => ComparisonOp::LessEqual,
                Token::Greater => ComparisonOp::Greater,
                Token::GreaterEq => ComparisonOp::GreaterEqual,
                _ => break,
            };

            self.advance();
            let right = self.parse_shift()?;
            left = ASTValue::Comparison {
                op,
                left: Box::new(left),
                right: Box::new(right),
            };
        }

        Ok(left)
    }

    fn parse_shift(&mut self) -> Result<ASTValue, String> {
        let mut left = self.parse_additive()?;

        while let Some(token) = self.peek() {
            let op = match token {
                Token::Shl => BinaryOp::Shl,
                Token::Shr => BinaryOp::Shr,
                _ => break,
            };

            self.advance();
            let right = self.parse_additive()?;
            left = ASTValue::Binary {
                op,
                left: Box::new(left),
                right: Box::new(right),
            };
        }

        Ok(left)
    }

    fn parse_additive(&mut self) -> Result<ASTValue, String> {
        let mut left = self.parse_multiplicative()?;

        while let Some(token) = self.peek() {
            let op = match token {
                Token::Plus => BinaryOp::Add,
                Token::Minus => BinaryOp::Sub,
                _ => break,
            };

            self.advance();
            let right = self.parse_multiplicative()?;
            left = ASTValue::Binary {
                op,
                left: Box::new(left),
                right: Box::new(right),
            };
        }

        Ok(left)
    }

    fn parse_multiplicative(&mut self) -> Result<ASTValue, String> {
        let mut left = self.parse_unary()?;

        while let Some(token) = self.peek() {
            let op = match token {
                Token::Star => BinaryOp::Mul,
                Token::Slash => BinaryOp::Div,
                Token::Percent => BinaryOp::Mod,
                _ => break,
            };

            self.advance();
            let right = self.parse_unary()?;
            left = ASTValue::Binary {
                op,
                left: Box::new(left),
                right: Box::new(right),
            };
        }

        Ok(left)
    }

    fn parse_unary(&mut self) -> Result<ASTValue, String> {
        match self.peek() {
            Some(Token::Not) => {
                self.advance();
                let operand = self.parse_unary()?;
                Ok(ASTValue::Unary {
                    op: UnaryOp::Not,
                    operand: Box::new(operand),
                })
            }
            Some(Token::Minus) => {
                self.advance();
                let operand = self.parse_unary()?;
                Ok(ASTValue::Unary {
                    op: UnaryOp::Negate,
                    operand: Box::new(operand),
                })
            }
            Some(Token::PlusPlus) => {
                self.advance();
                let operand = self.parse_postfix()?;
                Ok(ASTValue::Unary {
                    op: UnaryOp::PreInc,
                    operand: Box::new(operand),
                })
            }
            Some(Token::MinusMinus) => {
                self.advance();
                let operand = self.parse_postfix()?;
                Ok(ASTValue::Unary {
                    op: UnaryOp::PreDec,
                    operand: Box::new(operand),
                })
            }
            _ => self.parse_postfix(),
        }
    }

    fn parse_postfix(&mut self) -> Result<ASTValue, String> {
        let mut expr = self.parse_primary()?;

        loop {
            match self.peek() {
                Some(Token::PlusPlus) => {
                    self.advance();
                    expr = ASTValue::Unary {
                        op: UnaryOp::PostInc,
                        operand: Box::new(expr),
                    };
                }
                Some(Token::MinusMinus) => {
                    self.advance();
                    expr = ASTValue::Unary {
                        op: UnaryOp::PostDec,
                        operand: Box::new(expr),
                    };
                }
                Some(Token::LBracket) => {
                    self.advance();
                    let index = self.parse_expression()?;
                    self.expect(Token::RBracket)?;
                    expr = ASTValue::ArrayAccess {
                        array: Box::new(expr),
                        index: Box::new(index),
                    };
                }
                Some(Token::Dot) => {
                    self.advance();
                    if let Some(Token::Ident(member)) = self.peek() {
                        let member = member.clone();
                        self.advance();
                        
                        if matches!(self.peek(), Some(Token::LParen)) {
                            self.advance();
                            let args = self.parse_arguments()?;
                            self.expect(Token::RParen)?;
                            expr = ASTValue::MethodCall {
                                object: Box::new(expr),
                                method: member,
                                args,
                            };
                        } else {
                            expr = ASTValue::MemberAccess {
                                object: Box::new(expr),
                                member,
                            };
                        }
                    } else {
                        return Err("Expected member name after '.'".to_string());
                    }
                }
                Some(Token::LParen) if matches!(expr, ASTValue::VarRef(_)) => {
                    if let ASTValue::VarRef(name) = expr {
                        self.advance();
                        let args = self.parse_arguments()?;
                        self.expect(Token::RParen)?;
                        expr = ASTValue::FuncCall { name, args };
                    }
                }
                _ => break,
            }
        }

        Ok(expr)
    }

    fn parse_primary(&mut self) -> Result<ASTValue, String> {
        match self.peek() {
            Some(Token::Integer(n)) => {
                let val = ASTValue::Int(*n);
                self.advance();
                Ok(val)
            }
            Some(Token::Float(f)) => {
                let val = ASTValue::Float(*f);
                self.advance();
                Ok(val)
            }
            Some(Token::String(s)) => {
                let val = ASTValue::Str(s.clone());
                self.advance();
                Ok(val)
            }
            Some(Token::True) => {
                self.advance();
                Ok(ASTValue::Bool(true))
            }
            Some(Token::False) => {
                self.advance();
                Ok(ASTValue::Bool(false))
            }
            Some(Token::Null) => {
                self.advance();
                Ok(ASTValue::Null)
            }
            Some(Token::This) => {
                self.advance();
                Ok(ASTValue::This)
            }
            Some(Token::New) => {
                self.advance();
                
                // Parse class name
                let class_name = if let Some(Token::Ident(name)) = self.peek() {
                    let name = name.clone();
                    self.advance();
                    name
                } else {
                    return Err("Expected class name after 'new'".to_string());
                };
                
                // Parse constructor arguments
                self.expect(Token::LParen)?;
                let args = self.parse_arguments()?;
                self.expect(Token::RParen)?;
                
                Ok(ASTValue::NewObject { class_name, args })
            }
            Some(Token::Ident(name)) => {
                let name = name.clone();
                self.advance();
                Ok(ASTValue::VarRef(name))
            }
            Some(Token::LParen) => {
                self.advance();
                let expr = self.parse_expression()?;
                self.expect(Token::RParen)?;
                Ok(expr)
            }
            Some(Token::LBracket) => {
                self.advance();
                let elements = self.parse_array_elements()?;
                self.expect(Token::RBracket)?;
                Ok(ASTValue::ArrayLiteral(elements))
            }
            Some(token) => Err(format!("Unexpected token in expression: {:?}", token)),
            None => Err("Unexpected end of input in expression".to_string()),
        }
    }

    fn parse_arguments(&mut self) -> Result<Vec<ASTValue>, String> {
        let mut args = Vec::new();

        if matches!(self.peek(), Some(Token::RParen)) {
            return Ok(args);
        }

        loop {
            args.push(self.parse_expression()?);
            
            match self.peek() {
                Some(Token::Comma) => {
                    self.advance();
                }
                _ => break,
            }
        }

        Ok(args)
    }

    fn parse_array_elements(&mut self) -> Result<Vec<ASTValue>, String> {
        let mut elements = Vec::new();

        if matches!(self.peek(), Some(Token::RBracket)) {
            return Ok(elements);
        }

        loop {
            elements.push(self.parse_expression()?);
            
            match self.peek() {
                Some(Token::Comma) => {
                    self.advance();
                }
                _ => break,
            }
        }

        Ok(elements)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_simple_expression() {
        let tokens = vec![
            Token::Integer(2),
            Token::Plus,
            Token::Integer(3),
            Token::Star,
            Token::Integer(4),
        ];
        
        let mut parser = ExpressionParser::new(&tokens, 0);
        let result = parser.parse_expression();
        assert!(result.is_ok());
    }
}
