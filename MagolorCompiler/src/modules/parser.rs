// src/modules/parser.rs
// Main parser for top-level declarations

use crate::modules::tokenizer::Token;
use crate::modules::ast::*;
use crate::modules::statement_parser::StatementParser;
use crate::modules::expression_parser::ExpressionParser;

pub struct Parser {
    tokens: Vec<Token>,
    pos: usize,
}

impl Parser {
    pub fn new(tokens: Vec<Token>) -> Self {
        Self { tokens, pos: 0 }
    }

    fn peek(&self) -> Option<&Token> {
        self.tokens.get(self.pos)
    }

    fn peek_ahead(&self, n: usize) -> Option<&Token> {
        self.tokens.get(self.pos + n)
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
            Some(token) if token == &expected => {
                self.advance();
                Ok(())
            }
            Some(token) => Err(format!("Expected {:?}, found {:?}", expected, token)),
            None => Err(format!("Expected {:?}, found end of input", expected)),
        }
    }

    pub fn parse(&mut self) -> Result<Program, String> {
        let mut program = Vec::new();

        while self.pos < self.tokens.len() {
            if let Some(item) = self.parse_top_level()? {
                program.push(item);
            }
        }

        Ok(program)
    }

    fn parse_top_level(&mut self) -> Result<Option<TopLevel>, String> {
        match self.peek() {
            Some(Token::Use) => self.parse_import(),
            Some(Token::Public) | Some(Token::Private) | Some(Token::Protected) => {
                let access = self.parse_access_modifier()?;
                self.parse_declaration_with_access(access)
            }
            Some(Token::Class) => {
                self.parse_class(AccessModifier::Public)
            }
            Some(Token::Struct) => {
                self.parse_struct(AccessModifier::Public)
            }
            Some(Token::Enum) => {
                self.parse_enum(AccessModifier::Public)
            }
            Some(Token::Namespace) => {
                self.parse_namespace()
            }
            Some(Token::Void) | Some(Token::I32Type) | Some(Token::I64Type) 
            | Some(Token::F32Type) | Some(Token::F64Type) | Some(Token::StringType) 
            | Some(Token::BoolType) => {
                self.parse_function(AccessModifier::Public, false)
            }
            Some(Token::Func) => {
                self.parse_function(AccessModifier::Public, false)
            }
            Some(Token::Semicolon) => {
                self.advance();
                Ok(None)
            }
            Some(token) => Err(format!("Unexpected token at top level: {:?}", token)),
            None => Ok(None),
        }
    }

    fn parse_access_modifier(&mut self) -> Result<AccessModifier, String> {
        let access = match self.peek() {
            Some(Token::Public) => AccessModifier::Public,
            Some(Token::Private) => AccessModifier::Private,
            Some(Token::Protected) => AccessModifier::Protected,
            _ => return Err("Expected access modifier".to_string()),
        };
        self.advance();
        Ok(access)
    }

    fn parse_declaration_with_access(&mut self, access: AccessModifier) -> Result<Option<TopLevel>, String> {
        match self.peek() {
            Some(Token::Class) => self.parse_class(access),
            Some(Token::Struct) => self.parse_struct(access),
            Some(Token::Enum) => self.parse_enum(access),
            Some(Token::Static) => {
                self.advance();
                self.parse_function(access, true)
            }
            Some(Token::Void) | Some(Token::I32Type) | Some(Token::I64Type) 
            | Some(Token::F32Type) | Some(Token::F64Type) | Some(Token::StringType) 
            | Some(Token::BoolType) | Some(Token::Func) => {
                self.parse_function(access, false)
            }
            Some(token) => Err(format!("Unexpected token after access modifier: {:?}", token)),
            None => Err("Unexpected end of input after access modifier".to_string()),
        }
    }

    fn parse_import(&mut self) -> Result<Option<TopLevel>, String> {
        self.advance(); // Skip 'use'

        if let Some(Token::Ident(package)) = self.peek() {
            let package = package.clone();
            self.advance();
            self.expect(Token::Semicolon)?;
            Ok(Some(TopLevel::Import(package)))
        } else {
            Err("Expected package name after 'use'".to_string())
        }
    }

    fn parse_type(&mut self) -> Result<Type, String> {
        match self.peek() {
            Some(Token::I32Type) => {
                self.advance();
                Ok(Type::I32)
            }
            Some(Token::I64Type) => {
                self.advance();
                Ok(Type::I64)
            }
            Some(Token::F32Type) => {
                self.advance();
                Ok(Type::F32)
            }
            Some(Token::F64Type) => {
                self.advance();
                Ok(Type::F64)
            }
            Some(Token::StringType) => {
                self.advance();
                Ok(Type::String)
            }
            Some(Token::BoolType) => {
                self.advance();
                Ok(Type::Bool)
            }
            Some(Token::Void) => {
                self.advance();
                Ok(Type::Void)
            }
            Some(Token::VarType) => {
                self.advance();
                Ok(Type::Auto)
            }
            Some(Token::Ident(name)) => {
                let type_name = name.clone();
                self.advance();
                
                // Check for array type
                if matches!(self.peek(), Some(Token::LBracket)) {
                    self.advance();
                    self.expect(Token::RBracket)?;
                    Ok(Type::Array(Box::new(Type::Custom(type_name))))
                } else {
                    Ok(Type::Custom(type_name))
                }
            }
            Some(token) => Err(format!("Expected type, found {:?}", token)),
            None => Err("Expected type, found end of input".to_string()),
        }
    }

    fn parse_function(&mut self, access: AccessModifier, is_static: bool) -> Result<Option<TopLevel>, String> {
        // Parse return type
        let return_type = if matches!(self.peek(), Some(Token::Func)) {
            Type::Void
        } else {
            self.parse_type()?
        };

        self.expect(Token::Func)?;

        // Parse function name
        let name = if let Some(Token::Ident(n)) = self.peek() {
            let name = n.clone();
            self.advance();
            name
        } else {
            return Err("Expected function name".to_string());
        };

        // Parse parameters
        self.expect(Token::LParen)?;
        let params = self.parse_parameters()?;
        self.expect(Token::RParen)?;

        // Parse body
        let body = self.parse_function_body()?;

        Ok(Some(TopLevel::Function(FunctionDef {
            name,
            params,
            return_type,
            body,
            access,
            is_static,
        })))
    }

    fn parse_parameters(&mut self) -> Result<Vec<Parameter>, String> {
        let mut params = Vec::new();

        if matches!(self.peek(), Some(Token::RParen)) {
            return Ok(params);
        }

        loop {
            let param_type = self.parse_type()?;
            
            if let Some(Token::Ident(name)) = self.peek() {
                let name = name.clone();
                self.advance();
                
                // Check for default value
                let default_value = if matches!(self.peek(), Some(Token::Eq)) {
                    self.advance();
                    let mut expr_parser = ExpressionParser::new(&self.tokens, self.pos);
                    let val = expr_parser.parse_expression()?;
                    self.pos = expr_parser.current_pos();
                    Some(val)
                } else {
                    None
                };
                
                params.push(Parameter {
                    name,
                    param_type,
                    default_value,
                });
                
                if matches!(self.peek(), Some(Token::Comma)) {
                    self.advance();
                } else {
                    break;
                }
            } else {
                return Err("Expected parameter name".to_string());
            }
        }

        Ok(params)
    }

    fn parse_function_body(&mut self) -> Result<Vec<Statement>, String> {
        self.expect(Token::LBrace)?;
        
        let mut body = Vec::new();
        
        while !matches!(self.peek(), Some(Token::RBrace)) {
            let mut stmt_parser = StatementParser::new(&self.tokens, self.pos);
            if let Some(stmt) = stmt_parser.parse_statement()? {
                body.push(stmt);
            }
            self.pos = stmt_parser.current_pos();
        }
        
        self.expect(Token::RBrace)?;
        Ok(body)
    }

    fn parse_class(&mut self, access: AccessModifier) -> Result<Option<TopLevel>, String> {
        self.advance(); // Skip 'class'

        let name = if let Some(Token::Ident(n)) = self.peek() {
            let name = n.clone();
            self.advance();
            name
        } else {
            return Err("Expected class name".to_string());
        };

        self.expect(Token::LBrace)?;

        let mut fields = Vec::new();
        let mut methods = Vec::new();
        let mut constructor = None;

        while !matches!(self.peek(), Some(Token::RBrace)) {
            let member_access = if matches!(
                self.peek(),
                Some(Token::Public) | Some(Token::Private) | Some(Token::Protected)
            ) {
                self.parse_access_modifier()?
            } else {
                AccessModifier::Private // Default to private in classes
            };

            let is_static = if matches!(self.peek(), Some(Token::Static)) {
                self.advance();
                true
            } else {
                false
            };

            // Check if it's a method or field
            if self.is_type_token(self.peek()) || matches!(self.peek(), Some(Token::Func)) {
                let start_pos = self.pos;
                
                // Try to determine if it's a method
                let is_method = if matches!(self.peek(), Some(Token::Func)) {
                    true
                } else {
                    // Look ahead to see if there's 'fn' after the type
                    let mut lookahead = 1;
                    while let Some(token) = self.peek_ahead(lookahead) {
                        if matches!(token, Token::Func) {
                            break;
                        }
                        if matches!(token, Token::Eq | Token::Semicolon) {
                            break;
                        }
                        lookahead += 1;
                        if lookahead > 10 { break; } // Limit lookahead
                    }
                    matches!(self.peek_ahead(lookahead), Some(Token::Func))
                };

                if is_method {
                    let method = self.parse_method(member_access, is_static)?;
                    
                    // Check if it's a constructor
                    if method.name == name && method.return_type == Type::Void {
                        constructor = Some(method);
                    } else {
                        methods.push(method);
                    }
                } else {
                    let field = self.parse_field(member_access, is_static)?;
                    fields.push(field);
                }
            } else {
                return Err(format!("Unexpected token in class body: {:?}", self.peek()));
            }
        }

        self.expect(Token::RBrace)?;

        Ok(Some(TopLevel::Class(ClassDef {
            name,
            fields,
            methods,
            constructor,
            access,
        })))
    }

    fn parse_method(&mut self, access: AccessModifier, is_static: bool) -> Result<FunctionDef, String> {
        let return_type = if matches!(self.peek(), Some(Token::Func)) {
            Type::Void
        } else {
            self.parse_type()?
        };

        self.expect(Token::Func)?;

        let name = if let Some(Token::Ident(n)) = self.peek() {
            let name = n.clone();
            self.advance();
            name
        } else {
            return Err("Expected method name".to_string());
        };

        self.expect(Token::LParen)?;
        let params = self.parse_parameters()?;
        self.expect(Token::RParen)?;

        let body = self.parse_function_body()?;

        Ok(FunctionDef {
            name,
            params,
            return_type,
            body,
            access,
            is_static,
        })
    }

    fn parse_field(&mut self, access: AccessModifier, is_static: bool) -> Result<Field, String> {
        let field_type = self.parse_type()?;

        let name = if let Some(Token::Ident(n)) = self.peek() {
            let name = n.clone();
            self.advance();
            name
        } else {
            return Err("Expected field name".to_string());
        };

        let default_value = if matches!(self.peek(), Some(Token::Eq)) {
            self.advance();
            let mut expr_parser = ExpressionParser::new(&self.tokens, self.pos);
            let val = expr_parser.parse_expression()?;
            self.pos = expr_parser.current_pos();
            Some(val)
        } else {
            None
        };

        self.expect(Token::Semicolon)?;

        Ok(Field {
            name,
            field_type,
            access,
            is_static,
            default_value,
        })
    }

    fn parse_struct(&mut self, access: AccessModifier) -> Result<Option<TopLevel>, String> {
        self.advance(); // Skip 'struct'

        let name = if let Some(Token::Ident(n)) = self.peek() {
            let name = n.clone();
            self.advance();
            name
        } else {
            return Err("Expected struct name".to_string());
        };

        self.expect(Token::LBrace)?;

        let mut fields = Vec::new();

        while !matches!(self.peek(), Some(Token::RBrace)) {
            let field_access = if matches!(
                self.peek(),
                Some(Token::Public) | Some(Token::Private) | Some(Token::Protected)
            ) {
                self.parse_access_modifier()?
            } else {
                AccessModifier::Public // Structs default to public
            };

            let field = self.parse_field(field_access, false)?;
            fields.push(field);
        }

        self.expect(Token::RBrace)?;

        Ok(Some(TopLevel::Struct(StructDef {
            name,
            fields,
            access,
        })))
    }

    fn parse_enum(&mut self, access: AccessModifier) -> Result<Option<TopLevel>, String> {
        self.advance(); // Skip 'enum'

        let name = if let Some(Token::Ident(n)) = self.peek() {
            let name = n.clone();
            self.advance();
            name
        } else {
            return Err("Expected enum name".to_string());
        };

        self.expect(Token::LBrace)?;

        let mut variants = Vec::new();

        while !matches!(self.peek(), Some(Token::RBrace)) {
            if let Some(Token::Ident(variant)) = self.peek() {
                variants.push(variant.clone());
                self.advance();
                
                if matches!(self.peek(), Some(Token::Comma)) {
                    self.advance();
                }
            } else {
                return Err("Expected enum variant name".to_string());
            }
        }

        self.expect(Token::RBrace)?;

        Ok(Some(TopLevel::Enum(EnumDef {
            name,
            variants,
            access,
        })))
    }

    fn parse_namespace(&mut self) -> Result<Option<TopLevel>, String> {
        self.advance(); // Skip 'namespace'

        let name = if let Some(Token::Ident(n)) = self.peek() {
            let name = n.clone();
            self.advance();
            name
        } else {
            return Err("Expected namespace name".to_string());
        };

        self.expect(Token::LBrace)?;

        let mut items = Vec::new();

        while !matches!(self.peek(), Some(Token::RBrace)) {
            if let Some(item) = self.parse_top_level()? {
                items.push(item);
            }
        }

        self.expect(Token::RBrace)?;

        Ok(Some(TopLevel::Namespace { name, items }))
    }

    fn is_type_token(&self, token: Option<&Token>) -> bool {
        matches!(
            token,
            Some(Token::I32Type)
                | Some(Token::I64Type)
                | Some(Token::F32Type)
                | Some(Token::F64Type)
                | Some(Token::StringType)
                | Some(Token::BoolType)
                | Some(Token::Void)
                | Some(Token::VarType)
        )
    }
}

// Public API function
pub fn parse_tokens(tokens: &[Token]) -> Result<Program, String> {
    let mut parser = Parser::new(tokens.to_vec());
    parser.parse()
}