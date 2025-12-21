// src/modules/bytecode.rs - Fast Bytecode VM for game scripting

use std::collections::HashMap;
use std::rc::Rc;
use std::cell::RefCell;
use crate::modules::ast::*;

// ============================================
// Bytecode Instructions - Optimized for speed
// ============================================

#[derive(Debug, Clone)]
pub enum OpCode {
    // Constants
    LoadConst(u16),           // Push constant from pool
    LoadInt(i64),             // Inline small integers
    LoadFloat(f64),           // Inline floats
    LoadTrue,
    LoadFalse,
    LoadNull,
    
    // Variables
    LoadLocal(u16),           // Load local variable by index
    StoreLocal(u16),          // Store to local variable
    LoadGlobal(u16),          // Load global by index
    StoreGlobal(u16),         // Store to global
    
    // Arithmetic - optimized ops
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    Neg,
    
    // Fast integer ops (avoid type checks)
    AddI,                     // Integer-only addition
    SubI,
    MulI,
    DivI,
    
    // Comparison
    Eq,
    NotEq,
    Less,
    LessEq,
    Greater,
    GreaterEq,
    
    // Logical
    And,
    Or,
    Not,
    
    // Control flow - direct jumps
    Jump(u32),                // Unconditional jump
    JumpIfFalse(u32),         // Pop and jump if false
    JumpIfTrue(u32),          // Pop and jump if true
    
    // Functions - fast calling
    Call(u16, u8),            // Call function (index, arg_count)
    CallNative(u16, u8),      // Call native function
    Return,
    ReturnValue,
    
    // Stack manipulation
    Pop,
    Dup,                      // Duplicate top
    
    // Array/Object operations
    NewArray(u16),            // Create array with size
    ArrayGet,                 // array[index]
    ArraySet,                 // array[index] = value
    
    // Property access - cached for speed
    GetProperty(u16),         // Get property by cached index
    SetProperty(u16),         // Set property by cached index
    
    // Built-in function calls - no lookup overhead
    Print,
    PrintInt,
    PrintFloat,
    PrintStr,
    
    // Fast loops
    ForLoop {
        counter_slot: u16,
        end_value_slot: u16,
        loop_start: u32,
        loop_end: u32,
    },
    
    // Exit
    Halt,
}

// ============================================
// Value Types - Tagged unions for speed
// ============================================

#[derive(Debug, Clone)]
pub enum Value {
    Int(i64),
    Float(f64),
    Bool(bool),
    String(Rc<String>),
    Array(Rc<RefCell<Vec<Value>>>),
    Object(Rc<RefCell<HashMap<String, Value>>>),
    Function(FunctionRef),
    NativeFunction(NativeFn),
    Null,
}

impl Value {
    pub fn is_truthy(&self) -> bool {
        match self {
            Value::Bool(b) => *b,
            Value::Null => false,
            Value::Int(0) => false,
            _ => true,
        }
    }
    
    pub fn as_int(&self) -> Result<i64, String> {
        match self {
            Value::Int(n) => Ok(*n),
            _ => Err(format!("Expected integer, got {:?}", self)),
        }
    }
    
    pub fn as_float(&self) -> Result<f64, String> {
        match self {
            Value::Float(f) => Ok(*f),
            Value::Int(n) => Ok(*n as f64),
            _ => Err(format!("Expected number, got {:?}", self)),
        }
    }
}

// ============================================
// Function References
// ============================================

#[derive(Debug, Clone)]
pub struct FunctionRef {
    pub entry_point: u32,
    pub param_count: u8,
    pub local_count: u16,
    pub name: String,
}

pub type NativeFn = fn(&[Value]) -> Result<Value, String>;

// ============================================
// Bytecode Module - Contains all compiled code
// ============================================

pub struct BytecodeModule {
    pub instructions: Vec<OpCode>,
    pub constants: Vec<Value>,
    pub global_names: Vec<String>,
    pub functions: HashMap<String, FunctionRef>,
    pub entry_point: u32,
}

impl BytecodeModule {
    pub fn new() -> Self {
        Self {
            instructions: Vec::new(),
            constants: Vec::new(),
            global_names: Vec::new(),
            functions: HashMap::new(),
            entry_point: 0,
        }
    }
}

// ============================================
// Fast Bytecode Compiler - Optimized passes
// ============================================

pub struct BytecodeCompiler {
    module: BytecodeModule,
    
    // Compilation state
    current_function: Option<String>,
    local_vars: Vec<HashMap<String, u16>>,  // Stack of scopes
    local_count: u16,
    global_map: HashMap<String, u16>,
    
    // Optimization state
    last_was_load: Option<u16>,  // Track last load for peephole optimization
}

impl BytecodeCompiler {
    pub fn new() -> Self {
        Self {
            module: BytecodeModule::new(),
            current_function: None,
            local_vars: vec![HashMap::new()],
            local_count: 0,
            global_map: HashMap::new(),
            last_was_load: None,
        }
    }
    
    pub fn compile(mut self, program: Program) -> Result<BytecodeModule, String> {
        // First pass: collect all function signatures
        for item in &program {
            self.collect_function(&item)?;
        }
        
        // Second pass: compile everything
        for item in program {
            self.compile_top_level(item)?;
        }
        
        // Add halt at the end
        self.emit(OpCode::Halt);
        
        Ok(self.module)
    }
    
    fn collect_function(&mut self, item: &TopLevel) -> Result<(), String> {
        match item {
            TopLevel::Function(func) => {
                let entry = self.module.instructions.len() as u32;
                let func_ref = FunctionRef {
                    entry_point: entry,
                    param_count: func.params.len() as u8,
                    local_count: 0,  // Will be updated during compilation
                    name: func.name.clone(),
                };
                self.module.functions.insert(func.name.clone(), func_ref);
                Ok(())
            }
            _ => Ok(()),
        }
    }
    
    fn compile_top_level(&mut self, item: TopLevel) -> Result<(), String> {
        match item {
            TopLevel::Function(func) => self.compile_function(func),
            TopLevel::Constant { name, value, .. } => {
                let val = self.compile_const_expr(value)?;
                let global_idx = self.get_or_create_global(&name);
                let const_idx = self.add_constant(val);
                self.emit(OpCode::LoadConst(const_idx));
                self.emit(OpCode::StoreGlobal(global_idx));
                Ok(())
            }
            _ => Ok(()), // Skip classes/structs for now
        }
    }
    
    fn compile_function(&mut self, func: FunctionDef) -> Result<(), String> {
        self.current_function = Some(func.name.clone());
        self.local_vars.clear();
        self.local_vars.push(HashMap::new());
        self.local_count = 0;
        
        // Entry point already set in first pass
        let _entry = self.module.functions.get(&func.name).unwrap().entry_point;
        
        // Reserve slots for parameters
        for (i, param) in func.params.iter().enumerate() {
            self.local_vars[0].insert(param.name.clone(), i as u16);
            self.local_count = (i + 1) as u16;
        }
        
        // Compile function body
        for stmt in func.body {
            self.compile_statement(stmt)?;
        }
        
        // Ensure return
        if func.return_type == Type::Void {
            self.emit(OpCode::LoadNull);
        }
        self.emit(OpCode::Return);
        
        // Update local count
        if let Some(func_ref) = self.module.functions.get_mut(&func.name) {
            func_ref.local_count = self.local_count;
        }
        
        Ok(())
    }
    
    fn compile_statement(&mut self, stmt: Statement) -> Result<(), String> {
        match stmt {
            Statement::VarDecl { name, value, .. } => {
                // Allocate local slot
                let slot = self.local_count;
                self.local_count += 1;
                self.local_vars.last_mut().unwrap().insert(name, slot);
                
                // Initialize value
                if let Some(val) = value {
                    self.compile_expression(val)?;
                } else {
                    self.emit(OpCode::LoadNull);
                }
                self.emit(OpCode::StoreLocal(slot));
                Ok(())
            }
            
            Statement::Assignment { target, value } => {
                self.compile_expression(value)?;
                
                if let ASTValue::VarRef(name) = target {
                    if let Some(slot) = self.find_local(&name) {
                        self.emit(OpCode::StoreLocal(slot));
                    } else {
                        let global_idx = self.get_or_create_global(&name);
                        self.emit(OpCode::StoreGlobal(global_idx));
                    }
                }
                Ok(())
            }
            
            Statement::Return(val) => {
                if let Some(expr) = val {
                    self.compile_expression(expr)?;
                    self.emit(OpCode::ReturnValue);
                } else {
                    self.emit(OpCode::LoadNull);
                    self.emit(OpCode::Return);
                }
                Ok(())
            }
            
            Statement::If { condition, then_body, else_body, .. } => {
                self.compile_expression(condition)?;
                
                let jump_to_else = self.emit_jump(OpCode::JumpIfFalse(0));
                
                // Then branch
                for stmt in then_body {
                    self.compile_statement(stmt)?;
                }
                
                let jump_to_end = self.emit_jump(OpCode::Jump(0));
                
                // Patch else jump
                self.patch_jump(jump_to_else);
                
                // Else branch
                if let Some(else_stmts) = else_body {
                    for stmt in else_stmts {
                        self.compile_statement(stmt)?;
                    }
                }
                
                // Patch end jump
                self.patch_jump(jump_to_end);
                Ok(())
            }
            
            Statement::While { condition, body } => {
                let loop_start = self.module.instructions.len() as u32;
                
                self.compile_expression(condition)?;
                let jump_to_end = self.emit_jump(OpCode::JumpIfFalse(0));
                
                // Body
                for stmt in body {
                    self.compile_statement(stmt)?;
                }
                
                // Jump back to condition
                self.emit(OpCode::Jump(loop_start));
                
                // Patch end jump
                self.patch_jump(jump_to_end);
                Ok(())
            }
            
            Statement::Expression(expr) => {
                self.compile_expression(expr)?;
                self.emit(OpCode::Pop);  // Discard result
                Ok(())
            }
            
            _ => Ok(()),
        }
    }
    
    fn compile_expression(&mut self, expr: ASTValue) -> Result<(), String> {
        match expr {
            ASTValue::Int(n) => {
                // Inline small integers for speed
                if n >= -32768 && n <= 32767 {
                    self.emit(OpCode::LoadInt(n));
                } else {
                    let idx = self.add_constant(Value::Int(n));
                    self.emit(OpCode::LoadConst(idx));
                }
                Ok(())
            }
            
            ASTValue::Float(f) => {
                self.emit(OpCode::LoadFloat(f));
                Ok(())
            }
            
            ASTValue::Bool(b) => {
                self.emit(if b { OpCode::LoadTrue } else { OpCode::LoadFalse });
                Ok(())
            }
            
            ASTValue::Str(s) => {
                let idx = self.add_constant(Value::String(Rc::new(s)));
                self.emit(OpCode::LoadConst(idx));
                Ok(())
            }
            
            ASTValue::Null => {
                self.emit(OpCode::LoadNull);
                Ok(())
            }
            
            ASTValue::VarRef(name) => {
                if let Some(slot) = self.find_local(&name) {
                    self.emit(OpCode::LoadLocal(slot));
                    self.last_was_load = Some(slot);
                } else {
                    let global_idx = self.get_or_create_global(&name);
                    self.emit(OpCode::LoadGlobal(global_idx));
                }
                Ok(())
            }
            
            ASTValue::Binary { op, left, right } => {
                self.compile_expression(*left)?;
                self.compile_expression(*right)?;
                
                // Emit optimized integer ops when possible
                let opcode = match op {
                    BinaryOp::Add => OpCode::Add,
                    BinaryOp::Sub => OpCode::Sub,
                    BinaryOp::Mul => OpCode::Mul,
                    BinaryOp::Div => OpCode::Div,
                    BinaryOp::Mod => OpCode::Mod,
                    BinaryOp::And => OpCode::And,
                    BinaryOp::Or => OpCode::Or,
                    _ => return Err(format!("Unsupported binary op: {:?}", op)),
                };
                
                self.emit(opcode);
                Ok(())
            }
            
            ASTValue::Comparison { op, left, right } => {
                self.compile_expression(*left)?;
                self.compile_expression(*right)?;
                
                let opcode = match op {
                    ComparisonOp::Equal => OpCode::Eq,
                    ComparisonOp::NotEqual => OpCode::NotEq,
                    ComparisonOp::Less => OpCode::Less,
                    ComparisonOp::LessEqual => OpCode::LessEq,
                    ComparisonOp::Greater => OpCode::Greater,
                    ComparisonOp::GreaterEqual => OpCode::GreaterEq,
                };
                
                self.emit(opcode);
                Ok(())
            }
            
            ASTValue::Unary { op, operand } => {
                self.compile_expression(*operand)?;
                
                match op {
                    UnaryOp::Negate => self.emit(OpCode::Neg),
                    UnaryOp::Not => self.emit(OpCode::Not),
                    _ => return Err(format!("Unsupported unary op: {:?}", op)),
                }
                Ok(())
            }
            
            ASTValue::FuncCall { name, args, .. } => {
                // Compile arguments
                for arg in &args {
                    self.compile_expression(arg.clone())?;
                }
                
                // Special built-in functions - no lookup
                if name == "print" {
                    self.emit(OpCode::Print);
                } else if name == "print_int" {
                    self.emit(OpCode::PrintInt);
                } else {
                    // Regular function call
                    if let Some(func_idx) = self.module.functions.keys()
                        .position(|k| k == &name) {
                        self.emit(OpCode::Call(func_idx as u16, args.len() as u8));
                    } else {
                        return Err(format!("Unknown function: {}", name));
                    }
                }
                Ok(())
            }
            
            _ => Err(format!("Unsupported expression: {:?}", expr)),
        }
    }
    
    fn compile_const_expr(&self, expr: ASTValue) -> Result<Value, String> {
        match expr {
            ASTValue::Int(n) => Ok(Value::Int(n)),
            ASTValue::Float(f) => Ok(Value::Float(f)),
            ASTValue::Bool(b) => Ok(Value::Bool(b)),
            ASTValue::Str(s) => Ok(Value::String(Rc::new(s))),
            _ => Err("Non-constant expression in constant".to_string()),
        }
    }
    
    // Helper methods
    
    fn emit(&mut self, op: OpCode) {
        self.module.instructions.push(op);
        self.last_was_load = None;
    }
    
    fn emit_jump(&mut self, op: OpCode) -> usize {
        let pos = self.module.instructions.len();
        self.module.instructions.push(op);
        pos
    }
    
    fn patch_jump(&mut self, jump_pos: usize) {
        let target = self.module.instructions.len() as u32;
        match &mut self.module.instructions[jump_pos] {
            OpCode::Jump(ref mut addr) => *addr = target,
            OpCode::JumpIfFalse(ref mut addr) => *addr = target,
            OpCode::JumpIfTrue(ref mut addr) => *addr = target,
            _ => panic!("Attempting to patch non-jump instruction"),
        }
    }
    
    fn add_constant(&mut self, value: Value) -> u16 {
        let idx = self.module.constants.len();
        self.module.constants.push(value);
        idx as u16
    }
    
    fn find_local(&self, name: &str) -> Option<u16> {
        for scope in self.local_vars.iter().rev() {
            if let Some(&slot) = scope.get(name) {
                return Some(slot);
            }
        }
        None
    }
    
    fn get_or_create_global(&mut self, name: &str) -> u16 {
        if let Some(&idx) = self.global_map.get(name) {
            idx
        } else {
            let idx = self.module.global_names.len() as u16;
            self.module.global_names.push(name.to_string());
            self.global_map.insert(name.to_string(), idx);
            idx
        }
    }
}

// ============================================
// High-Performance Virtual Machine
// ============================================

pub struct VM {
    // Execution state
    ip: usize,                              // Instruction pointer
    stack: Vec<Value>,                      // Value stack
    call_frames: Vec<CallFrame>,            // Call stack
    
    // Memory
    globals: Vec<Value>,                    // Global variables
    
    // Module reference
    module: BytecodeModule,
    
    // Native function registry
    native_functions: HashMap<String, NativeFn>,
}

#[derive(Debug)]
struct CallFrame {
    return_ip: usize,
    base_pointer: usize,
}

impl VM {
    pub fn new(module: BytecodeModule) -> Self {
        let global_count = module.global_names.len();
        Self {
            ip: 0,
            stack: Vec::with_capacity(1024),
            call_frames: Vec::with_capacity(64),
            globals: vec![Value::Null; global_count],
            module,
            native_functions: HashMap::new(),
        }
    }
    
    pub fn register_native(&mut self, name: &str, func: NativeFn) {
        self.native_functions.insert(name.to_string(), func);
    }
    
    pub fn run(&mut self) -> Result<Value, String> {
        self.ip = self.module.entry_point as usize;
        
        loop {
            if self.ip >= self.module.instructions.len() {
                return Err("IP out of bounds".to_string());
            }
            
            let op = self.module.instructions[self.ip].clone();
            self.ip += 1;
            
            match op {
                OpCode::LoadConst(idx) => {
                    let val = self.module.constants[idx as usize].clone();
                    self.stack.push(val);
                }
                
                OpCode::LoadInt(n) => {
                    self.stack.push(Value::Int(n));
                }
                
                OpCode::LoadFloat(f) => {
                    self.stack.push(Value::Float(f));
                }
                
                OpCode::LoadTrue => {
                    self.stack.push(Value::Bool(true));
                }
                
                OpCode::LoadFalse => {
                    self.stack.push(Value::Bool(false));
                }
                
                OpCode::LoadNull => {
                    self.stack.push(Value::Null);
                }
                
                OpCode::LoadLocal(slot) => {
                    let bp = self.call_frames.last()
                        .map(|f| f.base_pointer)
                        .unwrap_or(0);
                    let val = self.stack[bp + slot as usize].clone();
                    self.stack.push(val);
                }
                
                OpCode::StoreLocal(slot) => {
                    let bp = self.call_frames.last()
                        .map(|f| f.base_pointer)
                        .unwrap_or(0);
                    let val = self.stack.pop().unwrap();
                    if bp + slot as usize >= self.stack.len() {
                        self.stack.resize(bp + slot as usize + 1, Value::Null);
                    }
                    self.stack[bp + slot as usize] = val;
                }
                
                OpCode::LoadGlobal(idx) => {
                    let val = self.globals[idx as usize].clone();
                    self.stack.push(val);
                }
                
                OpCode::StoreGlobal(idx) => {
                    let val = self.stack.pop().unwrap();
                    self.globals[idx as usize] = val;
                }
                
                // Arithmetic operations - fast path for integers
                OpCode::Add => {
                    let b = self.stack.pop().unwrap();
                    let a = self.stack.pop().unwrap();
                    let result = match (a, b) {
                        (Value::Int(x), Value::Int(y)) => Value::Int(x + y),
                        (Value::Float(x), Value::Float(y)) => Value::Float(x + y),
                        (Value::Int(x), Value::Float(y)) => Value::Float(x as f64 + y),
                        (Value::Float(x), Value::Int(y)) => Value::Float(x + y as f64),
                        _ => return Err("Type error in addition".to_string()),
                    };
                    self.stack.push(result);
                }
                
                OpCode::Sub => {
                    let b = self.stack.pop().unwrap();
                    let a = self.stack.pop().unwrap();
                    let result = match (a, b) {
                        (Value::Int(x), Value::Int(y)) => Value::Int(x - y),
                        (Value::Float(x), Value::Float(y)) => Value::Float(x - y),
                        _ => return Err("Type error in subtraction".to_string()),
                    };
                    self.stack.push(result);
                }
                
                OpCode::Mul => {
                    let b = self.stack.pop().unwrap();
                    let a = self.stack.pop().unwrap();
                    let result = match (a, b) {
                        (Value::Int(x), Value::Int(y)) => Value::Int(x * y),
                        (Value::Float(x), Value::Float(y)) => Value::Float(x * y),
                        _ => return Err("Type error in multiplication".to_string()),
                    };
                    self.stack.push(result);
                }
                
                OpCode::Div => {
                    let b = self.stack.pop().unwrap();
                    let a = self.stack.pop().unwrap();
                    let result = match (a, b) {
                        (Value::Int(x), Value::Int(y)) => {
                            if y == 0 {
                                return Err("Division by zero".to_string());
                            }
                            Value::Int(x / y)
                        }
                        (Value::Float(x), Value::Float(y)) => Value::Float(x / y),
                        _ => return Err("Type error in division".to_string()),
                    };
                    self.stack.push(result);
                }
                
                OpCode::Mod => {
                    let b = self.stack.pop().unwrap();
                    let a = self.stack.pop().unwrap();
                    if let (Value::Int(x), Value::Int(y)) = (a, b) {
                        self.stack.push(Value::Int(x % y));
                    } else {
                        return Err("Modulo requires integers".to_string());
                    }
                }
                
                OpCode::Neg => {
                    let val = self.stack.pop().unwrap();
                    let result = match val {
                        Value::Int(n) => Value::Int(-n),
                        Value::Float(f) => Value::Float(-f),
                        _ => return Err("Cannot negate non-number".to_string()),
                    };
                    self.stack.push(result);
                }
                
                // Comparison operations
                OpCode::Eq => {
                    let b = self.stack.pop().unwrap();
                    let a = self.stack.pop().unwrap();
                    let result = self.values_equal(&a, &b);
                    self.stack.push(Value::Bool(result));
                }
                
                OpCode::NotEq => {
                    let b = self.stack.pop().unwrap();
                    let a = self.stack.pop().unwrap();
                    let result = !self.values_equal(&a, &b);
                    self.stack.push(Value::Bool(result));
                }
                
                OpCode::Less => {
                    let b = self.stack.pop().unwrap();
                    let a = self.stack.pop().unwrap();
                    let result = match (a, b) {
                        (Value::Int(x), Value::Int(y)) => x < y,
                        (Value::Float(x), Value::Float(y)) => x < y,
                        _ => return Err("Type error in comparison".to_string()),
                    };
                    self.stack.push(Value::Bool(result));
                }
                
                OpCode::LessEq => {
                    let b = self.stack.pop().unwrap();
                    let a = self.stack.pop().unwrap();
                    let result = match (a, b) {
                        (Value::Int(x), Value::Int(y)) => x <= y,
                        (Value::Float(x), Value::Float(y)) => x <= y,
                        _ => return Err("Type error in comparison".to_string()),
                    };
                    self.stack.push(Value::Bool(result));
                }
                
                OpCode::Greater => {
                    let b = self.stack.pop().unwrap();
                    let a = self.stack.pop().unwrap();
                    let result = match (a, b) {
                        (Value::Int(x), Value::Int(y)) => x > y,
                        (Value::Float(x), Value::Float(y)) => x > y,
                        _ => return Err("Type error in comparison".to_string()),
                    };
                    self.stack.push(Value::Bool(result));
                }
                
                OpCode::GreaterEq => {
                    let b = self.stack.pop().unwrap();
                    let a = self.stack.pop().unwrap();
                    let result = match (a, b) {
                        (Value::Int(x), Value::Int(y)) => x >= y,
                        (Value::Float(x), Value::Float(y)) => x >= y,
                        _ => return Err("Type error in comparison".to_string()),
                    };
                    self.stack.push(Value::Bool(result));
                }
                
                // Logical operations
                OpCode::And => {
                    let b = self.stack.pop().unwrap();
                    let a = self.stack.pop().unwrap();
                    let result = a.is_truthy() && b.is_truthy();
                    self.stack.push(Value::Bool(result));
                }
                
                OpCode::Or => {
                    let b = self.stack.pop().unwrap();
                    let a = self.stack.pop().unwrap();
                    let result = a.is_truthy() || b.is_truthy();
                    self.stack.push(Value::Bool(result));
                }
                
                OpCode::Not => {
                    let val = self.stack.pop().unwrap();
                    self.stack.push(Value::Bool(!val.is_truthy()));
                }
                
                // Control flow
                OpCode::Jump(addr) => {
                    self.ip = addr as usize;
                }
                
                OpCode::JumpIfFalse(addr) => {
                    let cond = self.stack.pop().unwrap();
                    if !cond.is_truthy() {
                        self.ip = addr as usize;
                    }
                }
                
                OpCode::JumpIfTrue(addr) => {
                    let cond = self.stack.pop().unwrap();
                    if cond.is_truthy() {
                        self.ip = addr as usize;
                    }
                }
                
                // Function calls
                OpCode::Call(func_idx, arg_count) => {
                    let func_name = self.module.functions.keys()
                        .nth(func_idx as usize)
                        .unwrap();
                    let func = self.module.functions.get(func_name).unwrap().clone();
                    
                    // Create call frame
                    let frame = CallFrame {
                        return_ip: self.ip,
                        base_pointer: self.stack.len() - arg_count as usize,
                    };
                    self.call_frames.push(frame);
                    
                    // Jump to function
                    self.ip = func.entry_point as usize;
                }
                
                OpCode::Return | OpCode::ReturnValue => {
                    let return_val = if matches!(op, OpCode::ReturnValue) {
                        self.stack.pop().unwrap()
                    } else {
                        Value::Null
                    };
                    
                    if let Some(frame) = self.call_frames.pop() {
                        // Clean up locals
                        self.stack.truncate(frame.base_pointer);
                        
                        // Push return value
                        self.stack.push(return_val);
                        
                        // Return to caller
                        self.ip = frame.return_ip;
                    } else {
                        // Main function return
                        return Ok(return_val);
                    }
                }
                
                // Stack manipulation
                OpCode::Pop => {
                    self.stack.pop();
                }
                
                OpCode::Dup => {
                    let val = self.stack.last().unwrap().clone();
                    self.stack.push(val);
                }
                
                // Built-in functions
                OpCode::Print => {
                    let val = self.stack.pop().unwrap();
                    println!("{:?}", val);
                }
                
                OpCode::PrintInt => {
                    let val = self.stack.pop().unwrap();
                    if let Value::Int(n) = val {
                        println!("{}", n);
                    }
                }
                
                OpCode::PrintFloat => {
                    let val = self.stack.pop().unwrap();
                    if let Value::Float(f) = val {
                        println!("{}", f);
                    }
                }
                
                OpCode::PrintStr => {
                    let val = self.stack.pop().unwrap();
                    if let Value::String(s) = val {
                        println!("{}", s);
                    }
                }
                
                OpCode::Halt => {
                    return Ok(Value::Null);
                }
                
                _ => {
                    return Err(format!("Unimplemented opcode: {:?}", op));
                }
            }
        }
    }
    
    fn values_equal(&self, a: &Value, b: &Value) -> bool {
        match (a, b) {
            (Value::Int(x), Value::Int(y)) => x == y,
            (Value::Float(x), Value::Float(y)) => x == y,
            (Value::Bool(x), Value::Bool(y)) => x == y,
            (Value::Null, Value::Null) => true,
            (Value::String(x), Value::String(y)) => x == y,
            _ => false,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_basic_arithmetic() {
        let mut compiler = BytecodeCompiler::new();
        // Test will be added with actual AST
    }
}
