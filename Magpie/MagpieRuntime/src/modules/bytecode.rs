use crate::modules::ast::*;
use std::cell::RefCell;
use std::collections::HashMap;
use std::rc::Rc;

#[derive(Debug, Clone)]
pub enum Op {
    Const(u16),
    Int(i64),
    Float(f64),
    True,
    False,
    Null,
    LoadLocal(u16),
    StoreLocal(u16),
    LoadGlobal(u16),
    StoreGlobal(u16),
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    Neg,
    BitNot,
    BitAnd,
    BitOr,
    BitXor,
    Shl,
    Shr,
    Eq,
    NotEq,
    Lt,
    LtEq,
    Gt,
    GtEq,
    And,
    Or,
    Not,
    Jump(u32),
    JumpIfFalse(u32),
    JumpIfTrue(u32),
    Call(u16, u8),
    Return,
    ReturnVal,
    Pop,
    Dup,
    NewArray(u16),
    ArrayGet,
    ArraySet,
    ArrayLen,
    NewObject, // Create empty object
    GetField(u16),
    SetField(u16), // Get/set object field (u16 is const index for field name)
    Print,
    PrintInt,
    PrintStr,
    Inc,
    Dec,
    Halt,
}

#[derive(Debug, Clone)]
pub enum Val {
    Int(i64),
    Float(f64),
    Bool(bool),
    Str(Rc<String>),
    Array(Rc<RefCell<Vec<Val>>>),
    Obj(Rc<RefCell<HashMap<String, Val>>>),
    Fn(FnRef),
    Null,
}

impl Val {
    pub fn truthy(&self) -> bool {
        match self {
            Val::Bool(b) => *b,
            Val::Null => false,
            Val::Int(0) => false,
            _ => true,
        }
    }
    pub fn as_int(&self) -> Result<i64, String> {
        match self {
            Val::Int(n) => Ok(*n),
            _ => Err("Expected int".into()),
        }
    }
    pub fn as_float(&self) -> Result<f64, String> {
        match self {
            Val::Float(f) => Ok(*f),
            Val::Int(n) => Ok(*n as f64),
            _ => Err("Expected number".into()),
        }
    }
}

#[derive(Debug, Clone)]
pub struct FnRef {
    pub entry: u32,
    pub params: u8,
    pub locals: u16,
    pub name: String,
    pub idx: u16,
}

pub struct Module {
    pub code: Vec<Op>,
    pub consts: Vec<Val>,
    pub globals: Vec<String>,
    pub funcs: Vec<FnRef>,
    pub func_map: HashMap<String, u16>,
    pub entry: u32,
}

impl Module {
    pub fn new() -> Self {
        Self {
            code: Vec::new(),
            consts: Vec::new(),
            globals: Vec::new(),
            funcs: Vec::new(),
            func_map: HashMap::new(),
            entry: 0,
        }
    }
}

pub struct Compiler {
    module: Module,
    locals: Vec<HashMap<String, u16>>,
    local_count: u16,
    global_map: HashMap<String, u16>,
    loop_breaks: Vec<Vec<usize>>,
    loop_continues: Vec<u32>,
}

impl Compiler {
    pub fn new() -> Self {
        Self {
            module: Module::new(),
            locals: vec![HashMap::new()],
            local_count: 0,
            global_map: HashMap::new(),
            loop_breaks: Vec::new(),
            loop_continues: Vec::new(),
        }
    }

    pub fn compile(mut self, prog: Program) -> Result<Module, String> {
        // Reserve space for main call at entry
        let has_main = prog
            .iter()
            .any(|t| matches!(t, TopLevel::Function(f) if f.name == "main"));
        if has_main {
            self.emit(Op::Call(0, 0)); // Placeholder, will patch
            self.emit(Op::Halt);
        }

        // First pass: register function names with indices
        for item in &prog {
            if let TopLevel::Function(f) = item {
                let idx = self.module.funcs.len() as u16;
                self.module.func_map.insert(f.name.clone(), idx);
                self.module.funcs.push(FnRef {
                    entry: 0,
                    params: f.params.len() as u8,
                    locals: 0,
                    name: f.name.clone(),
                    idx,
                });
            }
        }

        // Second pass: compile all functions
        for item in prog {
            self.compile_top(item)?;
        }

        // Patch the main call now that we know the index
        if has_main {
            if let Some(&main_idx) = self.module.func_map.get("main") {
                self.module.code[0] = Op::Call(main_idx, 0);
            }
        }

        self.emit(Op::Halt);
        Ok(self.module)
    }

    fn collect_fn(&mut self, _item: &TopLevel) -> Result<(), String> {
        Ok(())
    }

    fn compile_top(&mut self, item: TopLevel) -> Result<(), String> {
        match item {
            TopLevel::Function(f) => self.compile_fn(f),
            TopLevel::Const { name, value, .. } => {
                let val = self.const_expr(&value)?;
                let gidx = self.get_global(&name);
                let cidx = self.add_const(val);
                self.emit(Op::Const(cidx));
                self.emit(Op::StoreGlobal(gidx));
                Ok(())
            }
            TopLevel::Enum(e) => {
                for v in e.variants {
                    if let Some(val) = v.value {
                        let gidx = self.get_global(&format!("{}::{}", e.name, v.name));
                        self.emit(Op::Int(val));
                        self.emit(Op::StoreGlobal(gidx));
                    }
                }
                Ok(())
            }
            _ => Ok(()),
        }
    }

    fn compile_fn(&mut self, f: FnDef) -> Result<(), String> {
        // Set entry point to current code position
        let entry = self.module.code.len() as u32;
        if let Some(&idx) = self.module.func_map.get(&f.name) {
            self.module.funcs[idx as usize].entry = entry;
        }

        self.locals.clear();
        self.locals.push(HashMap::new());
        self.local_count = 0;

        for (i, p) in f.params.iter().enumerate() {
            self.locals[0].insert(p.name.clone(), i as u16);
            self.local_count = (i + 1) as u16;
        }

        for stmt in f.body {
            self.compile_stmt(stmt)?;
        }

        if f.ret == Type::Void {
            self.emit(Op::Null);
        }
        self.emit(Op::Return);

        if let Some(&idx) = self.module.func_map.get(&f.name) {
            self.module.funcs[idx as usize].locals = self.local_count;
        }
        Ok(())
    }

    fn compile_stmt(&mut self, stmt: Stmt) -> Result<(), String> {
        match stmt {
            Stmt::Let { name, value, .. } => {
                let slot = self.local_count;
                self.local_count += 1;
                self.locals.last_mut().unwrap().insert(name, slot);
                if let Some(v) = value {
                    self.compile_expr(v)?;
                } else {
                    self.emit(Op::Null);
                }
                self.emit(Op::StoreLocal(slot));
            }
            Stmt::Assign { target, value } => {
                self.compile_expr(value)?;
                self.compile_assign_target(&target)?;
            }
            Stmt::CompoundAssign { target, op, value } => {
                self.compile_expr(target.clone())?;
                self.compile_expr(value)?;
                self.emit(match op {
                    BinaryOp::Add => Op::Add,
                    BinaryOp::Sub => Op::Sub,
                    BinaryOp::Mul => Op::Mul,
                    BinaryOp::Div => Op::Div,
                    BinaryOp::Mod => Op::Mod,
                    _ => return Err("Invalid compound op".into()),
                });
                self.compile_assign_target(&target)?;
            }
            Stmt::Return(v) => {
                if let Some(e) = v {
                    self.compile_expr(e)?;
                    self.emit(Op::ReturnVal);
                } else {
                    self.emit(Op::Null);
                    self.emit(Op::Return);
                }
            }
            Stmt::If {
                cond,
                then_body,
                elifs,
                else_body,
            } => {
                self.compile_expr(cond)?;
                let jmp_else = self.emit_jmp(Op::JumpIfFalse(0));
                for s in then_body {
                    self.compile_stmt(s)?;
                }
                let mut end_jmps = vec![self.emit_jmp(Op::Jump(0))];
                self.patch(jmp_else);
                for (c, b) in elifs {
                    self.compile_expr(c)?;
                    let j = self.emit_jmp(Op::JumpIfFalse(0));
                    for s in b {
                        self.compile_stmt(s)?;
                    }
                    end_jmps.push(self.emit_jmp(Op::Jump(0)));
                    self.patch(j);
                }
                if let Some(eb) = else_body {
                    for s in eb {
                        self.compile_stmt(s)?;
                    }
                }
                for j in end_jmps {
                    self.patch(j);
                }
            }
            Stmt::While { cond, body } => {
                let start = self.module.code.len() as u32;
                self.loop_continues.push(start);
                self.loop_breaks.push(Vec::new());
                self.compile_expr(cond)?;
                let jmp_end = self.emit_jmp(Op::JumpIfFalse(0));
                for s in body {
                    self.compile_stmt(s)?;
                }
                self.emit(Op::Jump(start));
                self.patch(jmp_end);
                let breaks = self.loop_breaks.pop().unwrap();
                for b in breaks {
                    self.patch(b);
                }
                self.loop_continues.pop();
            }
            Stmt::For {
                init,
                cond,
                inc,
                body,
            } => {
                if let Some(i) = init {
                    self.compile_stmt(*i)?;
                }
                let start = self.module.code.len() as u32;
                let inc_start = start;
                self.loop_continues.push(inc_start);
                self.loop_breaks.push(Vec::new());
                let jmp_end = if let Some(c) = cond {
                    self.compile_expr(c)?;
                    Some(self.emit_jmp(Op::JumpIfFalse(0)))
                } else {
                    None
                };
                for s in body {
                    self.compile_stmt(s)?;
                }
                if let Some(i) = inc {
                    self.compile_stmt(*i)?;
                }
                self.emit(Op::Jump(start));
                if let Some(j) = jmp_end {
                    self.patch(j);
                }
                let breaks = self.loop_breaks.pop().unwrap();
                for b in breaks {
                    self.patch(b);
                }
                self.loop_continues.pop();
            }
            Stmt::ForEach {
                item,
                collection,
                body,
                ..
            } => {
                let arr_slot = self.local_count;
                self.local_count += 1;
                let idx_slot = self.local_count;
                self.local_count += 1;
                let item_slot = self.local_count;
                self.local_count += 1;
                self.locals.last_mut().unwrap().insert(item, item_slot);
                self.compile_expr(collection)?;
                self.emit(Op::StoreLocal(arr_slot));
                self.emit(Op::Int(0));
                self.emit(Op::StoreLocal(idx_slot));
                let start = self.module.code.len() as u32;
                self.loop_continues.push(start);
                self.loop_breaks.push(Vec::new());
                self.emit(Op::LoadLocal(idx_slot));
                self.emit(Op::LoadLocal(arr_slot));
                self.emit(Op::ArrayLen);
                self.emit(Op::Lt);
                let jmp_end = self.emit_jmp(Op::JumpIfFalse(0));
                self.emit(Op::LoadLocal(arr_slot));
                self.emit(Op::LoadLocal(idx_slot));
                self.emit(Op::ArrayGet);
                self.emit(Op::StoreLocal(item_slot));
                for s in body {
                    self.compile_stmt(s)?;
                }
                self.emit(Op::LoadLocal(idx_slot));
                self.emit(Op::Int(1));
                self.emit(Op::Add);
                self.emit(Op::StoreLocal(idx_slot));
                self.emit(Op::Jump(start));
                self.patch(jmp_end);
                let breaks = self.loop_breaks.pop().unwrap();
                for b in breaks {
                    self.patch(b);
                }
                self.loop_continues.pop();
            }
            Stmt::Break => {
                let j = self.emit_jmp(Op::Jump(0));
                if let Some(breaks) = self.loop_breaks.last_mut() {
                    breaks.push(j);
                }
            }
            Stmt::Continue => {
                if let Some(&start) = self.loop_continues.last() {
                    self.emit(Op::Jump(start));
                }
            }
            Stmt::Block(stmts) => {
                self.locals.push(HashMap::new());
                for s in stmts {
                    self.compile_stmt(s)?;
                }
                self.locals.pop();
            }
            Stmt::Expr(e) => {
                self.compile_expr(e)?;
                self.emit(Op::Pop);
            }
        }
        Ok(())
    }

  fn compile_assign_target(&mut self, target: &Expr) -> Result<(), String> {
    match target {
        Expr::Var(name) => {
            if let Some(slot) = self.find_local(name) {
                self.emit(Op::StoreLocal(slot));
            } else {
                let g = self.get_global(name);
                self.emit(Op::StoreGlobal(g));
            }
        }
        Expr::Index { array, index } => {
            self.compile_expr(*array.clone())?;
            self.compile_expr(*index.clone())?;
            self.emit(Op::ArraySet);
        }
        Expr::Field { object, field } => {
            // Stack currently has: [value]
            // We need: [object, value]
            // So we need to store value, load object, load value back
            let temp_slot = self.local_count;
            self.local_count += 1;
            
            self.emit(Op::StoreLocal(temp_slot));  // Store value temporarily
            self.compile_expr(*object.clone())?;    // Load object
            self.emit(Op::LoadLocal(temp_slot));    // Load value back
            
            let field_const = self.add_const(Val::Str(Rc::new(field.clone())));
            self.emit(Op::SetField(field_const));
            self.emit(Op::Pop);  // Pop the returned object
            
            self.local_count -= 1;
        }
        _ => return Err("Invalid assignment target".into()),
    }
    Ok(())
}    fn compile_expr(&mut self, expr: Expr) -> Result<(), String> {
        match expr {
            Expr::Int(n) => {
                if n >= -32768 && n <= 32767 {
                    self.emit(Op::Int(n));
                } else {
                    let c = self.add_const(Val::Int(n));
                    self.emit(Op::Const(c));
                }
            }
            Expr::Float(f) => {
                self.emit(Op::Float(f));
            }
            Expr::Bool(b) => {
                self.emit(if b { Op::True } else { Op::False });
            }
            Expr::Str(s) => {
                let c = self.add_const(Val::Str(Rc::new(s)));
                self.emit(Op::Const(c));
            }
            Expr::Null => {
                self.emit(Op::Null);
            }
            Expr::Var(name) => {
                if let Some(slot) = self.find_local(&name) {
                    self.emit(Op::LoadLocal(slot));
                } else {
                    let g = self.get_global(&name);
                    self.emit(Op::LoadGlobal(g));
                }
            }
            Expr::Array(elems) => {
                let len = elems.len();
                for e in elems {
                    self.compile_expr(e)?;
                }
                self.emit(Op::NewArray(len as u16));
            }
Expr::Object(props) => {
    // Create empty object on stack
    self.emit(Op::NewObject);
    
    // For each property, duplicate the object, set field, pop the returned copy
    // Stack has: [obj]
    for (key, value) in props {
        self.emit(Op::Dup);                    // [obj, obj]
        self.compile_expr(value)?;             // [obj, obj, value]
        let key_const = self.add_const(Val::Str(Rc::new(key)));
        self.emit(Op::SetField(key_const));    // [obj, obj] (SetField pops obj+value, pushes obj)
        self.emit(Op::Pop);                    // [obj]
    }
    // Final object remains on stack
}
            Expr::Index { array, index } => {
                self.compile_expr(*array)?;
                self.compile_expr(*index)?;
                self.emit(Op::ArrayGet);
            }
     Expr::Field { object, field } => {
    self.compile_expr(*object)?;
    let field_const = self.add_const(Val::Str(Rc::new(field)));
    self.emit(Op::GetField(field_const));
}
            Expr::Binary { op, left, right } => {
                self.compile_expr(*left)?;
                self.compile_expr(*right)?;
                self.emit(match op {
                    BinaryOp::Add => Op::Add,
                    BinaryOp::Sub => Op::Sub,
                    BinaryOp::Mul => Op::Mul,
                    BinaryOp::Div => Op::Div,
                    BinaryOp::Mod => Op::Mod,
                    BinaryOp::And => Op::And,
                    BinaryOp::Or => Op::Or,
                    BinaryOp::BitAnd => Op::BitAnd,
                    BinaryOp::BitOr => Op::BitOr,
                    BinaryOp::BitXor => Op::BitXor,
                    BinaryOp::Shl => Op::Shl,
                    BinaryOp::Shr => Op::Shr,
                });
            }
            Expr::Compare { op, left, right } => {
                self.compile_expr(*left)?;
                self.compile_expr(*right)?;
                self.emit(match op {
                    ComparisonOp::Equal => Op::Eq,
                    ComparisonOp::NotEqual => Op::NotEq,
                    ComparisonOp::Less => Op::Lt,
                    ComparisonOp::LessEqual => Op::LtEq,
                    ComparisonOp::Greater => Op::Gt,
                    ComparisonOp::GreaterEqual => Op::GtEq,
                });
            }
            Expr::Unary { op, operand } => match op {
                UnaryOp::PreInc | UnaryOp::PreDec => {
                    self.compile_expr(*operand.clone())?;
                    self.emit(if matches!(op, UnaryOp::PreInc) {
                        Op::Inc
                    } else {
                        Op::Dec
                    });
                    self.emit(Op::Dup);
                    self.compile_assign_target(&operand)?;
                }
                UnaryOp::PostInc | UnaryOp::PostDec => {
                    self.compile_expr(*operand.clone())?;
                    self.emit(Op::Dup);
                    self.emit(if matches!(op, UnaryOp::PostInc) {
                        Op::Inc
                    } else {
                        Op::Dec
                    });
                    self.compile_assign_target(&operand)?;
                }
                _ => {
                    self.compile_expr(*operand)?;
                    self.emit(match op {
                        UnaryOp::Negate => Op::Neg,
                        UnaryOp::Not => Op::Not,
                        UnaryOp::BitNot => Op::BitNot,
                        _ => unreachable!(),
                    });
                }
            },
            Expr::Ternary {
                cond,
                then_val,
                else_val,
            } => {
                self.compile_expr(*cond)?;
                let jmp_else = self.emit_jmp(Op::JumpIfFalse(0));
                self.compile_expr(*then_val)?;
                let jmp_end = self.emit_jmp(Op::Jump(0));
                self.patch(jmp_else);
                self.compile_expr(*else_val)?;
                self.patch(jmp_end);
            }
            Expr::Call { name, args } => {
                for a in &args {
                    self.compile_expr(a.clone())?;
                }
                match name.as_str() {
                    "print" => self.emit(Op::Print),
                    "print_int" => self.emit(Op::PrintInt),
                    "print_str" => self.emit(Op::PrintStr),
                    "len" => self.emit(Op::ArrayLen),
                    _ => {
                        if let Some(&idx) = self.module.func_map.get(&name) {
                            self.emit(Op::Call(idx, args.len() as u8));
                        } else {
                            return Err(format!("Unknown function: {}", name));
                        }
                    }
                }
            }
            _ => {}
        }
        Ok(())
    }
    fn const_expr(&self, expr: &Expr) -> Result<Val, String> {
        match expr {
            Expr::Int(n) => Ok(Val::Int(*n)),
            Expr::Float(f) => Ok(Val::Float(*f)),
            Expr::Bool(b) => Ok(Val::Bool(*b)),
            Expr::Str(s) => Ok(Val::Str(Rc::new(s.clone()))),
            _ => Err("Non-constant expression".into()),
        }
    }

    fn emit(&mut self, op: Op) {
        self.module.code.push(op);
    }
    fn emit_jmp(&mut self, op: Op) -> usize {
        let pos = self.module.code.len();
        self.module.code.push(op);
        pos
    }
    fn patch(&mut self, pos: usize) {
        let target = self.module.code.len() as u32;
        match &mut self.module.code[pos] {
            Op::Jump(a) | Op::JumpIfFalse(a) | Op::JumpIfTrue(a) => *a = target,
            _ => panic!("Not a jump"),
        }
    }
    fn add_const(&mut self, v: Val) -> u16 {
        let i = self.module.consts.len();
        self.module.consts.push(v);
        i as u16
    }
    fn find_local(&self, name: &str) -> Option<u16> {
        for scope in self.locals.iter().rev() {
            if let Some(&s) = scope.get(name) {
                return Some(s);
            }
        }
        None
    }
    fn get_global(&mut self, name: &str) -> u16 {
        if let Some(&i) = self.global_map.get(name) {
            i
        } else {
            let i = self.module.globals.len() as u16;
            self.module.globals.push(name.to_string());
            self.global_map.insert(name.to_string(), i);
            i
        }
    }
}

pub struct VM {
    ip: usize,
    stack: Vec<Val>,
    frames: Vec<Frame>,
    globals: Vec<Val>,
    module: Module,
}

struct Frame {
    ret_ip: usize,
    base: usize,
}

impl VM {
    pub fn new(module: Module) -> Self {
        let gc = module.globals.len();
        Self {
            ip: 0,
            stack: Vec::with_capacity(4096),
            frames: Vec::with_capacity(256),
            globals: vec![Val::Null; gc],
            module,
        }
    }

    pub fn run(&mut self) -> Result<Val, String> {
        self.ip = self.module.entry as usize;
        loop {
            if self.ip >= self.module.code.len() {
                return Err("IP out of bounds".into());
            }
            let op = self.module.code[self.ip].clone();
            self.ip += 1;
            match op {
                Op::Const(i) => self.stack.push(self.module.consts[i as usize].clone()),
                Op::Int(n) => self.stack.push(Val::Int(n)),
                Op::Float(f) => self.stack.push(Val::Float(f)),
                Op::True => self.stack.push(Val::Bool(true)),
                Op::False => self.stack.push(Val::Bool(false)),
                Op::Null => self.stack.push(Val::Null),
                Op::LoadLocal(s) => {
                    let bp = self.frames.last().map(|f| f.base).unwrap_or(0);
                    self.stack.push(self.stack[bp + s as usize].clone());
                }
                Op::StoreLocal(s) => {
                    let bp = self.frames.last().map(|f| f.base).unwrap_or(0);
                    let v = self.stack.pop().unwrap();
                    let idx = bp + s as usize;
                    if idx >= self.stack.len() {
                        self.stack.resize(idx + 1, Val::Null);
                    }
                    self.stack[idx] = v;
                }
                Op::LoadGlobal(i) => self.stack.push(self.globals[i as usize].clone()),
                Op::StoreGlobal(i) => {
                    let v = self.stack.pop().unwrap();
                    self.globals[i as usize] = v;
                }
                Op::Add => {
                    let b = self.stack.pop().unwrap();
                    let a = self.stack.pop().unwrap();
                    self.stack.push(self.add(a, b)?);
                }
                Op::Sub => {
                    let b = self.stack.pop().unwrap();
                    let a = self.stack.pop().unwrap();
                    self.stack.push(self.sub(a, b)?);
                }
                Op::Mul => {
                    let b = self.stack.pop().unwrap();
                    let a = self.stack.pop().unwrap();
                    self.stack.push(self.mul(a, b)?);
                }
                Op::Div => {
                    let b = self.stack.pop().unwrap();
                    let a = self.stack.pop().unwrap();
                    self.stack.push(self.div(a, b)?);
                }
                Op::Mod => {
                    let (Val::Int(b), Val::Int(a)) =
                        (self.stack.pop().unwrap(), self.stack.pop().unwrap())
                    else {
                        return Err("Mod requires ints".into());
                    };
                    self.stack.push(Val::Int(a % b));
                }
                Op::Neg => {
                    let v = self.stack.pop().unwrap();
                    self.stack.push(match v {
                        Val::Int(n) => Val::Int(-n),
                        Val::Float(f) => Val::Float(-f),
                        _ => return Err("Cannot negate".into()),
                    });
                }
                Op::BitNot => {
                    let Val::Int(n) = self.stack.pop().unwrap() else {
                        return Err("BitNot requires int".into());
                    };
                    self.stack.push(Val::Int(!n));
                }
                Op::BitAnd => {
                    let (Val::Int(b), Val::Int(a)) =
                        (self.stack.pop().unwrap(), self.stack.pop().unwrap())
                    else {
                        return Err("BitAnd requires ints".into());
                    };
                    self.stack.push(Val::Int(a & b));
                }
                Op::BitOr => {
                    let (Val::Int(b), Val::Int(a)) =
                        (self.stack.pop().unwrap(), self.stack.pop().unwrap())
                    else {
                        return Err("BitOr requires ints".into());
                    };
                    self.stack.push(Val::Int(a | b));
                }
                Op::BitXor => {
                    let (Val::Int(b), Val::Int(a)) =
                        (self.stack.pop().unwrap(), self.stack.pop().unwrap())
                    else {
                        return Err("BitXor requires ints".into());
                    };
                    self.stack.push(Val::Int(a ^ b));
                }
                Op::Shl => {
                    let (Val::Int(b), Val::Int(a)) =
                        (self.stack.pop().unwrap(), self.stack.pop().unwrap())
                    else {
                        return Err("Shl requires ints".into());
                    };
                    self.stack.push(Val::Int(a << b));
                }
                Op::Shr => {
                    let (Val::Int(b), Val::Int(a)) =
                        (self.stack.pop().unwrap(), self.stack.pop().unwrap())
                    else {
                        return Err("Shr requires ints".into());
                    };
                    self.stack.push(Val::Int(a >> b));
                }
                Op::Eq => {
                    let b = self.stack.pop().unwrap();
                    let a = self.stack.pop().unwrap();
                    self.stack.push(Val::Bool(self.eq(&a, &b)));
                }
                Op::NotEq => {
                    let b = self.stack.pop().unwrap();
                    let a = self.stack.pop().unwrap();
                    self.stack.push(Val::Bool(!self.eq(&a, &b)));
                }
                Op::Lt => {
                    let b = self.stack.pop().unwrap();
                    let a = self.stack.pop().unwrap();
                    self.stack.push(Val::Bool(self.cmp(&a, &b)? < 0));
                }
                Op::LtEq => {
                    let b = self.stack.pop().unwrap();
                    let a = self.stack.pop().unwrap();
                    self.stack.push(Val::Bool(self.cmp(&a, &b)? <= 0));
                }
                Op::Gt => {
                    let b = self.stack.pop().unwrap();
                    let a = self.stack.pop().unwrap();
                    self.stack.push(Val::Bool(self.cmp(&a, &b)? > 0));
                }
                Op::GtEq => {
                    let b = self.stack.pop().unwrap();
                    let a = self.stack.pop().unwrap();
                    self.stack.push(Val::Bool(self.cmp(&a, &b)? >= 0));
                }
                Op::And => {
                    let b = self.stack.pop().unwrap();
                    let a = self.stack.pop().unwrap();
                    self.stack.push(Val::Bool(a.truthy() && b.truthy()));
                }
                Op::Or => {
                    let b = self.stack.pop().unwrap();
                    let a = self.stack.pop().unwrap();
                    self.stack.push(Val::Bool(a.truthy() || b.truthy()));
                }
                Op::Not => {
                    let v = self.stack.pop().unwrap();
                    self.stack.push(Val::Bool(!v.truthy()));
                }
                Op::Jump(a) => self.ip = a as usize,
                Op::JumpIfFalse(a) => {
                    if !self.stack.pop().unwrap().truthy() {
                        self.ip = a as usize;
                    }
                }
                Op::JumpIfTrue(a) => {
                    if self.stack.pop().unwrap().truthy() {
                        self.ip = a as usize;
                    }
                }
                Op::Call(fi, argc) => {
                    let func = self.module.funcs[fi as usize].clone();
                    self.frames.push(Frame {
                        ret_ip: self.ip,
                        base: self.stack.len() - argc as usize,
                    });
                    self.ip = func.entry as usize;
                }
                Op::Return | Op::ReturnVal => {
                    let ret = if matches!(op, Op::ReturnVal) {
                        self.stack.pop().unwrap()
                    } else {
                        Val::Null
                    };
                    if let Some(f) = self.frames.pop() {
                        self.stack.truncate(f.base);
                        self.stack.push(ret);
                        self.ip = f.ret_ip;
                    } else {
                        return Ok(ret);
                    }
                }
                Op::Pop => {
                    self.stack.pop();
                }
                Op::Dup => {
                    let v = self.stack.last().unwrap().clone();
                    self.stack.push(v);
                }
                Op::NewArray(n) => {
                    let mut arr = Vec::with_capacity(n as usize);
                    for _ in 0..n {
                        arr.push(self.stack.pop().unwrap());
                    }
                    arr.reverse();
                    self.stack.push(Val::Array(Rc::new(RefCell::new(arr))));
                }
                Op::NewObject => {
                    self.stack
                        .push(Val::Obj(Rc::new(RefCell::new(HashMap::new()))));
                }

         Op::GetField(field_idx) => {
    let Val::Str(field_name) = &self.module.consts[field_idx as usize] else {
        return Err("Field name must be string".into());
    };
    let obj = self.stack.pop().unwrap();

    match obj {
        Val::Obj(map) => {
            let val = map
                .borrow()
                .get(field_name.as_str())
                .cloned()
                .unwrap_or(Val::Null);
            self.stack.push(val);
        }
        Val::Array(arr) if field_name.as_str() == "length" => {
            self.stack.push(Val::Int(arr.borrow().len() as i64));
        }
        Val::Str(s) if field_name.as_str() == "length" => {
            self.stack.push(Val::Int(s.len() as i64));
        }
        _ => {
            return Err(format!(
                "Cannot get field '{}' from non-object (got {:?})",
                field_name, obj
            ))
        }
    }
}

    Op::SetField(field_idx) => {
    let value = self.stack.pop().unwrap();
    let Val::Str(field_name) = &self.module.consts[field_idx as usize] else {
        return Err("Field name must be string".into());
    };
    let obj_val = self.stack.pop().unwrap();
    
    // Better error message
    let Val::Obj(obj) = &obj_val else {
        return Err(format!(
            "Cannot set field '{}' on non-object (got {:?}). Stack before pop had {} items",
            field_name,
            obj_val,
            self.stack.len() + 2
        ));
    };

    obj.borrow_mut().insert(field_name.to_string(), value);
    self.stack.push(obj_val); // Push object back
}
                Op::ArrayGet => {
                    let idx_val = self.stack.pop().unwrap();
                    let container = self.stack.pop().unwrap();

                    match (container, idx_val) {
                        (Val::Array(arr), Val::Int(idx)) => {
                            self.stack.push(arr.borrow()[idx as usize].clone());
                        }
                        (Val::Obj(obj), Val::Str(key)) => {
                            let val = obj.borrow().get(key.as_str()).cloned().unwrap_or(Val::Null);
                            self.stack.push(val);
                        }
                        _ => return Err("Invalid index operation".into()),
                    }
                }

                Op::ArraySet => {
                    let val = self.stack.pop().unwrap();
                    let idx_val = self.stack.pop().unwrap();
                    let container = self.stack.pop().unwrap();

                    match (container, idx_val) {
                        (Val::Array(arr), Val::Int(idx)) => {
                            arr.borrow_mut()[idx as usize] = val;
                        }
                        (Val::Obj(obj), Val::Str(key)) => {
                            obj.borrow_mut().insert(key.to_string(), val);
                        }
                        _ => return Err("Invalid index assignment".into()),
                    }
                }
                Op::ArrayLen => {
                    let Val::Array(arr) = self.stack.pop().unwrap() else {
                        return Err("Not an array".into());
                    };
                    self.stack.push(Val::Int(arr.borrow().len() as i64));
                }
                Op::Inc => {
                    let v = self.stack.pop().unwrap();
                    self.stack.push(match v {
                        Val::Int(n) => Val::Int(n + 1),
                        Val::Float(f) => Val::Float(f + 1.0),
                        _ => return Err("Cannot increment".into()),
                    });
                }
                Op::Dec => {
                    let v = self.stack.pop().unwrap();
                    self.stack.push(match v {
                        Val::Int(n) => Val::Int(n - 1),
                        Val::Float(f) => Val::Float(f - 1.0),
                        _ => return Err("Cannot decrement".into()),
                    });
                }
                Op::Print => {
                    let v = self.stack.pop().unwrap();
                    println!("{:?}", v);
                    self.stack.push(Val::Null);
                }
                Op::PrintInt => {
                    let Val::Int(n) = self.stack.pop().unwrap() else {
                        return Err("Expected int".into());
                    };
                    println!("{}", n);
                    self.stack.push(Val::Null);
                }
                Op::PrintStr => {
                    let Val::Str(s) = self.stack.pop().unwrap() else {
                        return Err("Expected string".into());
                    };
                    println!("{}", s);
                    self.stack.push(Val::Null);
                }
                Op::Halt => return Ok(Val::Null),
                _ => return Err(format!("Unimplemented: {:?}", op)),
            }
        }
    }

    fn add(&self, a: Val, b: Val) -> Result<Val, String> {
        Ok(match (a, b) {
            (Val::Int(x), Val::Int(y)) => Val::Int(x + y),
            (Val::Float(x), Val::Float(y)) => Val::Float(x + y),
            (Val::Int(x), Val::Float(y)) => Val::Float(x as f64 + y),
            (Val::Float(x), Val::Int(y)) => Val::Float(x + y as f64),
            (Val::Str(x), Val::Str(y)) => Val::Str(Rc::new(format!("{}{}", x, y))),
            _ => return Err("Type error in add".into()),
        })
    }
    fn sub(&self, a: Val, b: Val) -> Result<Val, String> {
        Ok(match (a, b) {
            (Val::Int(x), Val::Int(y)) => Val::Int(x - y),
            (Val::Float(x), Val::Float(y)) => Val::Float(x - y),
            (Val::Int(x), Val::Float(y)) => Val::Float(x as f64 - y),
            (Val::Float(x), Val::Int(y)) => Val::Float(x - y as f64),
            _ => return Err("Type error in sub".into()),
        })
    }
    fn mul(&self, a: Val, b: Val) -> Result<Val, String> {
        Ok(match (a, b) {
            (Val::Int(x), Val::Int(y)) => Val::Int(x * y),
            (Val::Float(x), Val::Float(y)) => Val::Float(x * y),
            (Val::Int(x), Val::Float(y)) => Val::Float(x as f64 * y),
            (Val::Float(x), Val::Int(y)) => Val::Float(x * y as f64),
            _ => return Err("Type error in mul".into()),
        })
    }
    fn div(&self, a: Val, b: Val) -> Result<Val, String> {
        Ok(match (a, b) {
            (Val::Int(x), Val::Int(y)) => {
                if y == 0 {
                    return Err("Division by zero".into());
                }
                Val::Int(x / y)
            }
            (Val::Float(x), Val::Float(y)) => Val::Float(x / y),
            (Val::Int(x), Val::Float(y)) => Val::Float(x as f64 / y),
            (Val::Float(x), Val::Int(y)) => Val::Float(x / y as f64),
            _ => return Err("Type error in div".into()),
        })
    }
    fn eq(&self, a: &Val, b: &Val) -> bool {
        match (a, b) {
            (Val::Int(x), Val::Int(y)) => x == y,
            (Val::Float(x), Val::Float(y)) => x == y,
            (Val::Bool(x), Val::Bool(y)) => x == y,
            (Val::Str(x), Val::Str(y)) => x == y,
            (Val::Null, Val::Null) => true,
            _ => false,
        }
    }
    fn cmp(&self, a: &Val, b: &Val) -> Result<i32, String> {
        Ok(match (a, b) {
            (Val::Int(x), Val::Int(y)) => x.cmp(y) as i32,
            (Val::Float(x), Val::Float(y)) => x.partial_cmp(y).map(|o| o as i32).unwrap_or(0),
            (Val::Int(x), Val::Float(y)) => {
                (*x as f64).partial_cmp(y).map(|o| o as i32).unwrap_or(0)
            }
            (Val::Float(x), Val::Int(y)) => {
                x.partial_cmp(&(*y as f64)).map(|o| o as i32).unwrap_or(0)
            }
            _ => return Err("Cannot compare".into()),
        })
    }
}
