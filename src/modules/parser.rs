// src/modules/parser_comprehensive.rs - Full-featured parser

use crate::modules::tokenizer::Token;
use crate::modules::ast::*;

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
        // Handle access modifiers
        let access = if matches!(self.peek(), Some(Token::Public) | Some(Token::Private) | Some(Token::Protected)) {
            self.parse_access_modifier()?
        } else {
            AccessModifier::Public
        };

        // Handle static modifier
        let is_static = if matches!(self.peek(), Some(Token::Static)) {
            self.advance();
            true
        } else {
            false
        };

        match self.peek() {
            Some(Token::Const) => self.parse_constant(access),
            Some(Token::Class) => self.parse_class(access),
            Some(Token::Struct) => self.parse_struct(access),
            Some(Token::Namespace) => self.parse_namespace(),
            Some(Token::Void) | Some(Token::I32Type) | Some(Token::I64Type) 
            | Some(Token::F32Type) | Some(Token::F64Type) | Some(Token::StringType) 
            | Some(Token::BoolType) => {
                self.parse_function(access, is_static)
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

    // ========================================
    // CONSTANT PARSING
    // ========================================
    
    fn parse_constant(&mut self, access: AccessModifier) -> Result<Option<TopLevel>, String> {
        self.advance(); // Skip 'const'
        
        let const_type = self.parse_type()?;
        
        let name = if let Some(Token::Ident(n)) = self.peek() {
            let name = n.clone();
            self.advance();
            name
        } else {
            return Err("Expected constant name".to_string());
        };
        
        self.expect(Token::Eq)?;
        
        let value = self.parse_expression()?;
        
        self.expect(Token::Semicolon)?;
        
        Ok(Some(TopLevel::Constant(ConstantDef {
            name,
            const_type,
            value,
            access,
        })))
    }

    // ========================================
    // NAMESPACE PARSING
    // ========================================
    
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
        
        Ok(Some(TopLevel::Namespace(NamespaceDef { name, items })))
    }

    // ========================================
    // CLASS PARSING
    // ========================================
    
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
        
        let mut class = ClassDef::new(name);
        class.access = access;
        
        while !matches!(self.peek(), Some(Token::RBrace)) {
            self.parse_class_member(&mut class)?;
        }
        
        self.expect(Token::RBrace)?;
        
        Ok(Some(TopLevel::Class(class)))
    }
    
    fn parse_class_member(&mut self, class: &mut ClassDef) -> Result<(), String> {
        // Parse access modifier
        let access = if matches!(self.peek(), Some(Token::Public) | Some(Token::Private) | Some(Token::Protected)) {
            self.parse_access_modifier()?
        } else {
            AccessModifier::Private // Default to private in classes
        };
        
        // Parse static modifier
        let is_static = if matches!(self.peek(), Some(Token::Static)) {
            self.advance();
            true
        } else {
            false
        };
        
        // Check if it's a constructor (class name as method name)
        if let Some(Token::Ident(name)) = self.peek() {
            if *name == class.name && !is_static {
                return self.parse_constructor(class, access);
            }
        }
        
        // Parse type for field or method
        let member_type = self.parse_type()?;
        
        // Check if it's a method (followed by 'fn')
        if matches!(self.peek(), Some(Token::Func)) {
            self.advance();
            
            let method_name = if let Some(Token::Ident(n)) = self.peek() {
                let name = n.clone();
                self.advance();
                name
            } else {
                return Err("Expected method name".to_string());
            };
            
            self.expect(Token::LParen)?;
            let params = self.parse_parameters()?;
            self.expect(Token::RParen)?;
            
            let body = self.parse_block_statements()?;
            
            let method = FunctionDef {
                name: method_name,
                params,
                return_type: member_type,
                body,
                access,
                is_static,
            };
            
            class.add_method(method);
        } else {
            // It's a field
            let field_name = if let Some(Token::Ident(n)) = self.peek() {
                let name = n.clone();
                self.advance();
                name
            } else {
                return Err("Expected field name".to_string());
            };
            
            let default_value = if matches!(self.peek(), Some(Token::Eq)) {
                self.advance();
                Some(self.parse_expression()?)
            } else {
                None
            };
            
            self.expect(Token::Semicolon)?;
            
            let field = Field {
                name: field_name,
                field_type: member_type,
                access,
                is_static,
                default_value,
            };
            
            class.add_field(field);
        }
        
        Ok(())
    }
    
    fn parse_constructor(&mut self, class: &mut ClassDef, access: AccessModifier) -> Result<(), String> {
        self.advance(); // Skip constructor name
        
        self.expect(Token::LParen)?;
        let params = self.parse_parameters()?;
        self.expect(Token::RParen)?;
        
        let body = self.parse_block_statements()?;
        
        class.add_constructor(Constructor {
            params,
            body,
            access,
        });
        
        Ok(())
    }

    // ========================================
    // STRUCT PARSING
    // ========================================
    
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
        
        let mut struct_def = StructDef::new(name);
        struct_def.access = access;
        
        while !matches!(self.peek(), Some(Token::RBrace)) {
            self.parse_struct_field(&mut struct_def)?;
        }
        
        self.expect(Token::RBrace)?;
        
        Ok(Some(TopLevel::Struct(struct_def)))
    }
    
    fn parse_struct_field(&mut self, struct_def: &mut StructDef) -> Result<(), String> {
        let access = if matches!(self.peek(), Some(Token::Public) | Some(Token::Private) | Some(Token::Protected)) {
            self.parse_access_modifier()?
        } else {
            AccessModifier::Public // Struct fields are public by default
        };
        
        let field_type = self.parse_type()?;
        
        let field_name = if let Some(Token::Ident(n)) = self.peek() {
            let name = n.clone();
            self.advance();
            name
        } else {
            return Err("Expected field name".to_string());
        };
        
        let default_value = if matches!(self.peek(), Some(Token::Eq)) {
            self.advance();
            Some(self.parse_expression()?)
        } else {
            None
        };
        
        self.expect(Token::Semicolon)?;
        
        struct_def.add_field(Field {
            name: field_name,
            field_type,
            access,
            is_static: false,
            default_value,
        });
        
        Ok(())
    }

    // ========================================
    // FUNCTION PARSING
    // ========================================
    
    fn parse_function(&mut self, access: AccessModifier, is_static: bool) -> Result<Option<TopLevel>, String> {
        let return_type = self.parse_type()?;
        
        self.expect(Token::Func)?;
        
        let name = if let Some(Token::Ident(n)) = self.peek() {
            let name = n.clone();
            self.advance();
            name
        } else {
            return Err("Expected function name".to_string());
        };
        
        self.expect(Token::LParen)?;
        let params = self.parse_parameters()?;
        self.expect(Token::RParen)?;
        
        let body = self.parse_block_statements()?;
        
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
                
                params.push(Parameter {
                    name,
                    param_type,
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

    // ========================================
    // TYPE PARSING
    // ========================================
    
    fn parse_type(&mut self) -> Result<Type, String> {
        let base_type = match self.peek() {
            Some(Token::I32Type) => { self.advance(); Type::I32 }
            Some(Token::I64Type) => { self.advance(); Type::I64 }
            Some(Token::F32Type) => { self.advance(); Type::F32 }
            Some(Token::F64Type) => { self.advance(); Type::F64 }
            Some(Token::StringType) => { self.advance(); Type::String }
            Some(Token::BoolType) => { self.advance(); Type::Bool }
            Some(Token::Void) => { self.advance(); Type::Void }
            Some(Token::VarType) => { self.advance(); Type::Inferred }
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

    // ========================================
    // STATEMENT PARSING
    // ========================================
    
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
    
    fn parse_statement(&mut self) -> Result<Option<Statement>, String> {
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
            Some(Token::LBrace) => {
                let block = self.parse_block_statements()?;
                Ok(Some(Statement::Block(block)))
            }
            Some(Token::Semicolon) => {
                self.advance();
                Ok(None)
            }
            _ => self.parse_expression_or_assignment(),
        }
    }
    
    fn parse_var_decl(&mut self) -> Result<Option<Statement>, String> {
        let is_const = matches!(self.peek(), Some(Token::Const));
        self.advance(); // Skip 'let' or 'const'
        
        // Check for 'mut' keyword (Rust style)
        let is_mutable = if matches!(self.peek(), Some(Token::Mut)) {
            self.advance();
            true
        } else {
            !is_const // Variables are mutable by default unless const
        };

        let var_type = self.parse_type()?;
        
        let name = if let Some(Token::Ident(n)) = self.peek() {
            let name = n.clone();
            self.advance();
            name
        } else {
            return Err("Expected variable name".to_string());
        };

        let value = if matches!(self.peek(), Some(Token::Eq)) {
            self.advance();
            Some(self.parse_expression()?)
        } else {
            None
        };

        self.expect(Token::Semicolon)?;

        Ok(Some(Statement::VarDecl {
            name,
            var_type,
            value,
            is_mutable,
        }))
    }
    
    fn parse_if_statement(&mut self) -> Result<Option<Statement>, String> {
        self.advance(); // Skip 'if'

        self.expect(Token::LParen)?;
        let condition = self.parse_expression()?;
        self.expect(Token::RParen)?;
        
        let then_body = self.parse_block_statements()?;

        let mut elif_branches = Vec::new();
        while matches!(self.peek(), Some(Token::Elif)) {
            self.advance();
            self.expect(Token::LParen)?;
            let elif_condition = self.parse_expression()?;
            self.expect(Token::RParen)?;
            let elif_body = self.parse_block_statements()?;
            elif_branches.push((elif_condition, elif_body));
        }

        let else_body = if matches!(self.peek(), Some(Token::Else)) {
            self.advance();
            Some(self.parse_block_statements()?)
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
        self.expect(Token::LParen)?;
        let condition = self.parse_expression()?;
        self.expect(Token::RParen)?;
        let body = self.parse_block_statements()?;

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

        let body = self.parse_block_statements()?;

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
            return Err("Expected item name".to_string());
        };
        
        self.expect(Token::In)?;
        
        let collection = self.parse_expression()?;
        
        self.expect(Token::RParen)?;
        
        let body = self.parse_block_statements()?;
        
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

    // ========================================
    // EXPRESSION PARSING
    // ========================================
    
    fn parse_expression(&mut self) -> Result<ASTValue, String> {
        let mut expr_parser = crate::modules::expression_parser::ExpressionParser::new(&self.tokens, self.pos);
        let result = expr_parser.parse_expression()?;
        self.pos = expr_parser.current_pos();
        Ok(result)
    }
}

// Public API
pub fn parse_tokens(tokens: &[Token]) -> Result<Program, String> {
    let mut parser = Parser::new(tokens.to_vec());
    parser.parse()
}
