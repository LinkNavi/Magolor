use crate::modules::tokenizer::Token;
use crate::modules::ast::*;

pub struct Parser {
    tokens: Vec<Token>,
    pos: usize,
}

impl Parser {
    pub fn new(tokens: Vec<Token>) -> Self { Self { tokens, pos: 0 } }
    
    fn peek(&self) -> Option<&Token> { self.tokens.get(self.pos) }
    fn advance(&mut self) -> Option<&Token> {
        let t = self.tokens.get(self.pos);
        if t.is_some() { self.pos += 1; }
        t
    }
    fn expect(&mut self, expected: Token) -> Result<(), String> {
        match self.peek() {
            Some(t) if std::mem::discriminant(t) == std::mem::discriminant(&expected) => { self.advance(); Ok(()) }
            Some(t) => Err(format!("Expected {:?}, found {:?}", expected, t)),
            None => Err(format!("Expected {:?}, found EOF", expected)),
        }
    }
    
    pub fn parse(&mut self) -> Result<Program, String> {
        let mut prog = Vec::new();
        while self.pos < self.tokens.len() {
            if let Some(item) = self.parse_top_level()? { prog.push(item); }
        }
        Ok(prog)
    }
    
    fn parse_top_level(&mut self) -> Result<Option<TopLevel>, String> {
        let access = self.parse_access();
        let is_static = if matches!(self.peek(), Some(Token::Static)) { self.advance(); true } else { false };
        
        match self.peek() {
            Some(Token::Const) => self.parse_const(access),
            Some(Token::Class) => self.parse_class(access),
            Some(Token::Struct) => self.parse_struct(access),
            Some(Token::Enum) => self.parse_enum(access),
            Some(Token::Semicolon) => { self.advance(); Ok(None) }
            Some(Token::Void) | Some(Token::I8Type) | Some(Token::I16Type) |
            Some(Token::I32Type) | Some(Token::I64Type) | Some(Token::U8Type) |
            Some(Token::U16Type) | Some(Token::U32Type) | Some(Token::U64Type) |
            Some(Token::F32Type) | Some(Token::F64Type) | Some(Token::StringType) |
            Some(Token::BoolType) | Some(Token::CharType) | Some(Token::Ident(_)) => {
                self.parse_function(access, is_static)
            }
            Some(t) => Err(format!("Unexpected token: {:?}", t)),
            None => Ok(None),
        }
    }
    
    fn parse_access(&mut self) -> Access {
        match self.peek() {
            Some(Token::Public) => { self.advance(); Access::Public }
            Some(Token::Private) => { self.advance(); Access::Private }
            Some(Token::Protected) => { self.advance(); Access::Protected }
            _ => Access::Public,
        }
    }
    
    fn parse_type(&mut self) -> Result<Type, String> {
        let base = match self.peek() {
            Some(Token::I8Type) => { self.advance(); Type::I8 }
            Some(Token::I16Type) => { self.advance(); Type::I16 }
            Some(Token::I32Type) => { self.advance(); Type::I32 }
            Some(Token::I64Type) => { self.advance(); Type::I64 }
            Some(Token::U8Type) => { self.advance(); Type::U8 }
            Some(Token::U16Type) => { self.advance(); Type::U16 }
            Some(Token::U32Type) => { self.advance(); Type::U32 }
            Some(Token::U64Type) => { self.advance(); Type::U64 }
            Some(Token::F32Type) => { self.advance(); Type::F32 }
            Some(Token::F64Type) => { self.advance(); Type::F64 }
            Some(Token::StringType) => { self.advance(); Type::String }
            Some(Token::BoolType) => { self.advance(); Type::Bool }
            Some(Token::CharType) => { self.advance(); Type::Char }
            Some(Token::Void) => { self.advance(); Type::Void }
            Some(Token::VarType) => { self.advance(); Type::Inferred }
            Some(Token::Ident(n)) => { let n = n.clone(); self.advance(); Type::Custom(n) }
            Some(t) => return Err(format!("Expected type, found {:?}", t)),
            None => return Err("Expected type".to_string()),
        };
        if matches!(self.peek(), Some(Token::LBracket)) {
            self.advance();
            self.expect(Token::RBracket)?;
            Ok(Type::Array(Box::new(base)))
        } else { Ok(base) }
    }
    
    fn parse_const(&mut self, access: Access) -> Result<Option<TopLevel>, String> {
        self.advance();
        let ty = self.parse_type()?;
        let name = self.expect_ident()?;
        self.expect(Token::Eq)?;
        let value = self.parse_expr()?;
        self.expect(Token::Semicolon)?;
        Ok(Some(TopLevel::Const { name, ty, value, access }))
    }
    
    fn parse_enum(&mut self, access: Access) -> Result<Option<TopLevel>, String> {
        self.advance();
        let name = self.expect_ident()?;
        self.expect(Token::LBrace)?;
        let mut variants = Vec::new();
        let mut next_val = 0i64;
        while !matches!(self.peek(), Some(Token::RBrace)) {
            let vname = self.expect_ident()?;
            let value = if matches!(self.peek(), Some(Token::Eq)) {
                self.advance();
                if let Some(Token::Integer(n)) = self.peek() {
                    let n = *n; self.advance(); next_val = n + 1; Some(n)
                } else { return Err("Expected integer".to_string()); }
            } else { let v = next_val; next_val += 1; Some(v) };
            variants.push(EnumVariant { name: vname, value });
            if matches!(self.peek(), Some(Token::Comma)) { self.advance(); }
        }
        self.expect(Token::RBrace)?;
        Ok(Some(TopLevel::Enum(EnumDef { name, variants, access })))
    }
    
    fn parse_class(&mut self, access: Access) -> Result<Option<TopLevel>, String> {
        self.advance();
        let name = self.expect_ident()?;
        self.expect(Token::LBrace)?;
        let mut fields = Vec::new();
        let mut methods = Vec::new();
        while !matches!(self.peek(), Some(Token::RBrace)) {
            let acc = self.parse_access();
            let is_static = if matches!(self.peek(), Some(Token::Static)) { self.advance(); true } else { false };
            let ty = self.parse_type()?;
            if matches!(self.peek(), Some(Token::Func)) {
                self.advance();
                let mname = self.expect_ident()?;
                self.expect(Token::LParen)?;
                let params = self.parse_params()?;
                self.expect(Token::RParen)?;
                let body = self.parse_block()?;
                methods.push(FnDef { name: mname, params, ret: ty, body, access: acc, is_static });
            } else {
                let fname = self.expect_ident()?;
                let default = if matches!(self.peek(), Some(Token::Eq)) {
                    self.advance(); Some(self.parse_expr()?)
                } else { None };
                self.expect(Token::Semicolon)?;
                fields.push(Field { name: fname, ty, access: acc, is_static, default });
            }
        }
        self.expect(Token::RBrace)?;
        Ok(Some(TopLevel::Class(ClassDef { name, fields, methods, access })))
    }
    
    fn parse_struct(&mut self, access: Access) -> Result<Option<TopLevel>, String> {
        self.advance();
        let name = self.expect_ident()?;
        self.expect(Token::LBrace)?;
        let mut fields = Vec::new();
        while !matches!(self.peek(), Some(Token::RBrace)) {
            let acc = self.parse_access();
            let ty = self.parse_type()?;
            let fname = self.expect_ident()?;
            let default = if matches!(self.peek(), Some(Token::Eq)) {
                self.advance(); Some(self.parse_expr()?)
            } else { None };
            self.expect(Token::Semicolon)?;
            fields.push(Field { name: fname, ty, access: acc, is_static: false, default });
        }
        self.expect(Token::RBrace)?;
        Ok(Some(TopLevel::Struct(StructDef { name, fields, access })))
    }
    
    fn parse_function(&mut self, access: Access, is_static: bool) -> Result<Option<TopLevel>, String> {
        let ret = self.parse_type()?;
        self.expect(Token::Func)?;
        let name = self.expect_ident()?;
        self.expect(Token::LParen)?;
        let params = self.parse_params()?;
        self.expect(Token::RParen)?;
        let body = self.parse_block()?;
        Ok(Some(TopLevel::Function(FnDef { name, params, ret, body, access, is_static })))
    }
    
    fn parse_params(&mut self) -> Result<Vec<Param>, String> {
        let mut params = Vec::new();
        if matches!(self.peek(), Some(Token::RParen)) { return Ok(params); }
        loop {
            let ty = self.parse_type()?;
            let name = self.expect_ident()?;
            params.push(Param { name, ty });
            if matches!(self.peek(), Some(Token::Comma)) { self.advance(); } else { break; }
        }
        Ok(params)
    }
    
    fn parse_block(&mut self) -> Result<Vec<Stmt>, String> {
        self.expect(Token::LBrace)?;
        let mut stmts = Vec::new();
        while !matches!(self.peek(), Some(Token::RBrace)) {
            if let Some(s) = self.parse_stmt()? { stmts.push(s); }
        }
        self.expect(Token::RBrace)?;
        Ok(stmts)
    }
    
    fn parse_stmt(&mut self) -> Result<Option<Stmt>, String> {
        match self.peek() {
            Some(Token::Let) | Some(Token::Const) => self.parse_let(),
            Some(Token::If) => self.parse_if(),
            Some(Token::While) => self.parse_while(),
            Some(Token::For) => self.parse_for(),
            Some(Token::ForEach) => self.parse_foreach(),
            Some(Token::Return) => self.parse_return(),
            Some(Token::Break) => { self.advance(); self.expect(Token::Semicolon)?; Ok(Some(Stmt::Break)) }
            Some(Token::Continue) => { self.advance(); self.expect(Token::Semicolon)?; Ok(Some(Stmt::Continue)) }
            Some(Token::LBrace) => Ok(Some(Stmt::Block(self.parse_block()?))),
            Some(Token::Semicolon) => { self.advance(); Ok(None) }
            _ => self.parse_expr_or_assign(),
        }
    }
    
    fn parse_let(&mut self) -> Result<Option<Stmt>, String> {
        let is_const = matches!(self.peek(), Some(Token::Const));
        self.advance();
        let mutable = if matches!(self.peek(), Some(Token::Mut)) { self.advance(); true } else { !is_const };
        let ty = self.parse_type()?;
        let name = self.expect_ident()?;
        let value = if matches!(self.peek(), Some(Token::Eq)) { self.advance(); Some(self.parse_expr()?) } else { None };
        self.expect(Token::Semicolon)?;
        Ok(Some(Stmt::Let { name, ty, value, mutable }))
    }
    
    fn parse_if(&mut self) -> Result<Option<Stmt>, String> {
        self.advance();
        self.expect(Token::LParen)?;
        let cond = self.parse_expr()?;
        self.expect(Token::RParen)?;
        let then_body = self.parse_block()?;
        let mut elifs = Vec::new();
        while matches!(self.peek(), Some(Token::Elif)) {
            self.advance();
            self.expect(Token::LParen)?;
            let c = self.parse_expr()?;
            self.expect(Token::RParen)?;
            let b = self.parse_block()?;
            elifs.push((c, b));
        }
        let else_body = if matches!(self.peek(), Some(Token::Else)) {
            self.advance(); Some(self.parse_block()?)
        } else { None };
        Ok(Some(Stmt::If { cond, then_body, elifs, else_body }))
    }
    
    fn parse_while(&mut self) -> Result<Option<Stmt>, String> {
        self.advance();
        self.expect(Token::LParen)?;
        let cond = self.parse_expr()?;
        self.expect(Token::RParen)?;
        let body = self.parse_block()?;
        Ok(Some(Stmt::While { cond, body }))
    }
    
    fn parse_for(&mut self) -> Result<Option<Stmt>, String> {
        self.advance();
        self.expect(Token::LParen)?;
        // Init: either empty (;) or a let/assignment statement (which includes its own ;)
        let init = if matches!(self.peek(), Some(Token::Semicolon)) { 
            self.advance(); None 
        } else { 
            self.parse_stmt()?.map(Box::new) 
        };
        // Condition: either empty (;) or expression followed by ;
        let cond = if matches!(self.peek(), Some(Token::Semicolon)) { 
            self.advance(); None 
        } else { 
            let c = self.parse_expr()?; 
            self.expect(Token::Semicolon)?; 
            Some(c) 
        };
        // Increment: expression or assignment (no trailing ;)
        let inc = if matches!(self.peek(), Some(Token::RParen)) { 
            None 
        } else { 
            let expr = self.parse_expr()?;
            // Check for assignment
            let stmt = if matches!(self.peek(), Some(Token::Eq)) {
                self.advance();
                let value = self.parse_expr()?;
                Stmt::Assign { target: expr, value }
            } else if matches!(self.peek(), Some(Token::PlusEq)) {
                self.advance();
                let value = self.parse_expr()?;
                Stmt::CompoundAssign { target: expr, op: BinaryOp::Add, value }
            } else if matches!(self.peek(), Some(Token::MinusEq)) {
                self.advance();
                let value = self.parse_expr()?;
                Stmt::CompoundAssign { target: expr, op: BinaryOp::Sub, value }
            } else if matches!(self.peek(), Some(Token::StarEq)) {
                self.advance();
                let value = self.parse_expr()?;
                Stmt::CompoundAssign { target: expr, op: BinaryOp::Mul, value }
            } else if matches!(self.peek(), Some(Token::SlashEq)) {
                self.advance();
                let value = self.parse_expr()?;
                Stmt::CompoundAssign { target: expr, op: BinaryOp::Div, value }
            } else {
                Stmt::Expr(expr)
            };
            Some(Box::new(stmt))
        };
        self.expect(Token::RParen)?;
        let body = self.parse_block()?;
        Ok(Some(Stmt::For { init, cond, inc, body }))
    }
    
    fn parse_foreach(&mut self) -> Result<Option<Stmt>, String> {
        self.advance();
        self.expect(Token::LParen)?;
        let ty = self.parse_type()?;
        let item = self.expect_ident()?;
        self.expect(Token::In)?;
        let collection = self.parse_expr()?;
        self.expect(Token::RParen)?;
        let body = self.parse_block()?;
        Ok(Some(Stmt::ForEach { item, ty, collection, body }))
    }
    
    fn parse_return(&mut self) -> Result<Option<Stmt>, String> {
        self.advance();
        let value = if matches!(self.peek(), Some(Token::Semicolon)) { None } else { Some(self.parse_expr()?) };
        self.expect(Token::Semicolon)?;
        Ok(Some(Stmt::Return(value)))
    }
    
    fn parse_expr_or_assign(&mut self) -> Result<Option<Stmt>, String> {
        let expr = self.parse_expr()?;
        match self.peek() {
            Some(Token::Eq) => { self.advance(); let v = self.parse_expr()?; self.expect(Token::Semicolon)?; Ok(Some(Stmt::Assign { target: expr, value: v })) }
            Some(Token::PlusEq) => { self.advance(); let v = self.parse_expr()?; self.expect(Token::Semicolon)?; Ok(Some(Stmt::CompoundAssign { target: expr, op: BinaryOp::Add, value: v })) }
            Some(Token::MinusEq) => { self.advance(); let v = self.parse_expr()?; self.expect(Token::Semicolon)?; Ok(Some(Stmt::CompoundAssign { target: expr, op: BinaryOp::Sub, value: v })) }
            Some(Token::StarEq) => { self.advance(); let v = self.parse_expr()?; self.expect(Token::Semicolon)?; Ok(Some(Stmt::CompoundAssign { target: expr, op: BinaryOp::Mul, value: v })) }
            Some(Token::SlashEq) => { self.advance(); let v = self.parse_expr()?; self.expect(Token::Semicolon)?; Ok(Some(Stmt::CompoundAssign { target: expr, op: BinaryOp::Div, value: v })) }
            Some(Token::PercentEq) => { self.advance(); let v = self.parse_expr()?; self.expect(Token::Semicolon)?; Ok(Some(Stmt::CompoundAssign { target: expr, op: BinaryOp::Mod, value: v })) }
            _ => { self.expect(Token::Semicolon)?; Ok(Some(Stmt::Expr(expr))) }
        }
    }
    
    fn parse_expr(&mut self) -> Result<Expr, String> { self.parse_ternary() }
    
    fn parse_ternary(&mut self) -> Result<Expr, String> {
        let mut e = self.parse_or()?;
        if matches!(self.peek(), Some(Token::Question)) {
            self.advance();
            let t = self.parse_expr()?;
            self.expect(Token::Colon)?;
            let f = self.parse_expr()?;
            e = Expr::Ternary { cond: Box::new(e), then_val: Box::new(t), else_val: Box::new(f) };
        }
        Ok(e)
    }
    
    fn parse_or(&mut self) -> Result<Expr, String> {
        let mut l = self.parse_and()?;
        while matches!(self.peek(), Some(Token::OrOr)) {
            self.advance();
            let r = self.parse_and()?;
            l = Expr::Binary { op: BinaryOp::Or, left: Box::new(l), right: Box::new(r) };
        }
        Ok(l)
    }
    
    fn parse_and(&mut self) -> Result<Expr, String> {
        let mut l = self.parse_bitor()?;
        while matches!(self.peek(), Some(Token::AndAnd)) {
            self.advance();
            let r = self.parse_bitor()?;
            l = Expr::Binary { op: BinaryOp::And, left: Box::new(l), right: Box::new(r) };
        }
        Ok(l)
    }
    
    fn parse_bitor(&mut self) -> Result<Expr, String> {
        let mut l = self.parse_bitxor()?;
        while matches!(self.peek(), Some(Token::Or)) {
            self.advance();
            let r = self.parse_bitxor()?;
            l = Expr::Binary { op: BinaryOp::BitOr, left: Box::new(l), right: Box::new(r) };
        }
        Ok(l)
    }
    
    fn parse_bitxor(&mut self) -> Result<Expr, String> {
        let mut l = self.parse_bitand()?;
        while matches!(self.peek(), Some(Token::Xor)) {
            self.advance();
            let r = self.parse_bitand()?;
            l = Expr::Binary { op: BinaryOp::BitXor, left: Box::new(l), right: Box::new(r) };
        }
        Ok(l)
    }
    
    fn parse_bitand(&mut self) -> Result<Expr, String> {
        let mut l = self.parse_cmp()?;
        while matches!(self.peek(), Some(Token::And)) {
            self.advance();
            let r = self.parse_cmp()?;
            l = Expr::Binary { op: BinaryOp::BitAnd, left: Box::new(l), right: Box::new(r) };
        }
        Ok(l)
    }
    
    fn parse_cmp(&mut self) -> Result<Expr, String> {
        let mut l = self.parse_shift()?;
        while let Some(op) = match self.peek() {
            Some(Token::EqEq) => Some(ComparisonOp::Equal),
            Some(Token::NotEq) => Some(ComparisonOp::NotEqual),
            Some(Token::Less) => Some(ComparisonOp::Less),
            Some(Token::LessEq) => Some(ComparisonOp::LessEqual),
            Some(Token::Greater) => Some(ComparisonOp::Greater),
            Some(Token::GreaterEq) => Some(ComparisonOp::GreaterEqual),
            _ => None,
        } {
            self.advance();
            let r = self.parse_shift()?;
            l = Expr::Compare { op, left: Box::new(l), right: Box::new(r) };
        }
        Ok(l)
    }
    
    fn parse_shift(&mut self) -> Result<Expr, String> {
        let mut l = self.parse_add()?;
        while let Some(op) = match self.peek() {
            Some(Token::Shl) => Some(BinaryOp::Shl),
            Some(Token::Shr) => Some(BinaryOp::Shr),
            _ => None,
        } {
            self.advance();
            let r = self.parse_add()?;
            l = Expr::Binary { op, left: Box::new(l), right: Box::new(r) };
        }
        Ok(l)
    }
    
    fn parse_add(&mut self) -> Result<Expr, String> {
        let mut l = self.parse_mul()?;
        while let Some(op) = match self.peek() {
            Some(Token::Plus) => Some(BinaryOp::Add),
            Some(Token::Minus) => Some(BinaryOp::Sub),
            _ => None,
        } {
            self.advance();
            let r = self.parse_mul()?;
            l = Expr::Binary { op, left: Box::new(l), right: Box::new(r) };
        }
        Ok(l)
    }
    
    fn parse_mul(&mut self) -> Result<Expr, String> {
        let mut l = self.parse_unary()?;
        while let Some(op) = match self.peek() {
            Some(Token::Star) => Some(BinaryOp::Mul),
            Some(Token::Slash) => Some(BinaryOp::Div),
            Some(Token::Percent) => Some(BinaryOp::Mod),
            _ => None,
        } {
            self.advance();
            let r = self.parse_unary()?;
            l = Expr::Binary { op, left: Box::new(l), right: Box::new(r) };
        }
        Ok(l)
    }
    
    fn parse_unary(&mut self) -> Result<Expr, String> {
        match self.peek() {
            Some(Token::Not) => { self.advance(); Ok(Expr::Unary { op: UnaryOp::Not, operand: Box::new(self.parse_unary()?) }) }
            Some(Token::Minus) => { self.advance(); Ok(Expr::Unary { op: UnaryOp::Negate, operand: Box::new(self.parse_unary()?) }) }
            Some(Token::BitwiseNot) => { self.advance(); Ok(Expr::Unary { op: UnaryOp::BitNot, operand: Box::new(self.parse_unary()?) }) }
            Some(Token::PlusPlus) => { self.advance(); Ok(Expr::Unary { op: UnaryOp::PreInc, operand: Box::new(self.parse_postfix()?) }) }
            Some(Token::MinusMinus) => { self.advance(); Ok(Expr::Unary { op: UnaryOp::PreDec, operand: Box::new(self.parse_postfix()?) }) }
            _ => self.parse_postfix(),
        }
    }
    
    fn parse_postfix(&mut self) -> Result<Expr, String> {
        let mut e = self.parse_primary()?;
        loop {
            match self.peek() {
                Some(Token::PlusPlus) => { self.advance(); e = Expr::Unary { op: UnaryOp::PostInc, operand: Box::new(e) }; }
                Some(Token::MinusMinus) => { self.advance(); e = Expr::Unary { op: UnaryOp::PostDec, operand: Box::new(e) }; }
                Some(Token::LBracket) => {
                    self.advance();
                    let idx = self.parse_expr()?;
                    self.expect(Token::RBracket)?;
                    e = Expr::Index { array: Box::new(e), index: Box::new(idx) };
                }
                Some(Token::Dot) => {
                    self.advance();
                    let member = self.expect_ident()?;
                    if matches!(self.peek(), Some(Token::LParen)) {
                        self.advance();
                        let args = self.parse_args()?;
                        self.expect(Token::RParen)?;
                        e = Expr::Method { object: Box::new(e), method: member, args };
                    } else {
                        e = Expr::Field { object: Box::new(e), field: member };
                    }
                }
                Some(Token::LParen) if matches!(e, Expr::Var(_)) => {
                    if let Expr::Var(name) = e {
                        self.advance();
                        let args = self.parse_args()?;
                        self.expect(Token::RParen)?;
                        e = Expr::Call { name, args };
                    }
                }
                _ => break,
            }
        }
        Ok(e)
    }
    
    fn parse_primary(&mut self) -> Result<Expr, String> {
        match self.peek() {
            Some(Token::Integer(n)) => { let v = *n; self.advance(); Ok(Expr::Int(v)) }
            Some(Token::Float(f)) => { let v = *f; self.advance(); Ok(Expr::Float(v)) }
            Some(Token::String(s)) => { let v = s.clone(); self.advance(); Ok(Expr::Str(v)) }
            Some(Token::True) => { self.advance(); Ok(Expr::Bool(true)) }
            Some(Token::False) => { self.advance(); Ok(Expr::Bool(false)) }
            Some(Token::Null) => { self.advance(); Ok(Expr::Null) }
            Some(Token::This) | Some(Token::SelfKw) => { self.advance(); Ok(Expr::This) }
            Some(Token::New) => {
                self.advance();
                let class = self.expect_ident()?;
                self.expect(Token::LParen)?;
                let args = self.parse_args()?;
                self.expect(Token::RParen)?;
                Ok(Expr::New { class, args })
            }
            Some(Token::Ident(n)) => { let n = n.clone(); self.advance(); Ok(Expr::Var(n)) }
            Some(Token::LParen) => { self.advance(); let e = self.parse_expr()?; self.expect(Token::RParen)?; Ok(e) }
            Some(Token::LBracket) => {
                self.advance();
                let mut elems = Vec::new();
                if !matches!(self.peek(), Some(Token::RBracket)) {
                    loop {
                        elems.push(self.parse_expr()?);
                        if matches!(self.peek(), Some(Token::Comma)) { self.advance(); } else { break; }
                    }
                }
                self.expect(Token::RBracket)?;
                Ok(Expr::Array(elems))
            }
            Some(t) => Err(format!("Unexpected token in expr: {:?}", t)),
            None => Err("Unexpected EOF".to_string()),
        }
    }
    
    fn parse_args(&mut self) -> Result<Vec<Expr>, String> {
        let mut args = Vec::new();
        if matches!(self.peek(), Some(Token::RParen)) { return Ok(args); }
        loop {
            args.push(self.parse_expr()?);
            if matches!(self.peek(), Some(Token::Comma)) { self.advance(); } else { break; }
        }
        Ok(args)
    }
    
    fn expect_ident(&mut self) -> Result<String, String> {
        if let Some(Token::Ident(n)) = self.peek() { let n = n.clone(); self.advance(); Ok(n) }
        else { Err("Expected identifier".to_string()) }
    }
}

pub fn parse(tokens: &[Token]) -> Result<Program, String> {
    Parser::new(tokens.to_vec()).parse()
}
