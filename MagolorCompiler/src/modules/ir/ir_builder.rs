// src/modules/ir/ir_builder.rs
// Converts AST to IR

use crate::modules::ast::*;
use crate::modules::ir::ir_types::*;
use std::collections::HashMap;

pub struct IRBuilder {
    program: IRProgram,
    current_function: Option<String>,
    current_block: usize,
    register_counter: usize,
    block_counter: usize,
    local_counter: usize,
    symbol_table: Vec<HashMap<String, (IRValue, IRType)>>,
    loop_stack: Vec<LoopContext>,
}

struct LoopContext {
    continue_block: usize,
    break_block: usize,
}

impl IRBuilder {
    pub fn new() -> Self {
        Self {
            program: IRProgram::new(),
            current_function: None,
            current_block: 0,
            register_counter: 0,
            block_counter: 0,
            local_counter: 0,
            symbol_table: vec![HashMap::new()],
            loop_stack: Vec::new(),
        }
    }

    pub fn build(mut self, ast: Program) -> Result<IRProgram, String> {
        for top_level in ast {
            self.process_top_level(top_level)?;
        }
        Ok(self.program)
    }

    fn process_top_level(&mut self, item: TopLevel) -> Result<(), String> {
        match item {
            TopLevel::Import(module) => {
                // Handle imports - register system packages
                Ok(())
            }
            TopLevel::Function(func) => self.build_function(func),
            TopLevel::Class(class) => self.build_class(class),
            TopLevel::Namespace { name, items } => {
                for item in items {
                    self.process_top_level(item)?;
                }
                Ok(())
            }
            _ => Ok(()),
        }
    }

    fn build_function(&mut self, func: FunctionDef) -> Result<(), String> {
        let func_name = func.name.clone();
        self.current_function = Some(func_name.clone());
        self.register_counter = 0;
        self.block_counter = 0;
        self.local_counter = 0;

        let params: Vec<(String, IRType)> = func
            .params
            .iter()
            .map(|p| (p.name.clone(), self.convert_type(&p.param_type)))
            .collect();

        let return_type = self.convert_type(&func.return_type);

        let mut ir_func = IRFunction {
            name: func_name.clone(),
            params: params.clone(),
            return_type: return_type.clone(),
            blocks: Vec::new(),
            local_count: 0,
            register_count: 0,
            is_inline: false,
            is_pure: false,
            attributes: FunctionAttributes::default(),
        };

        // Create entry block
        let entry_block = self.create_block();
        ir_func.blocks.push(entry_block);
        self.current_block = 0;

        // Register parameters in symbol table
        self.push_scope();
        for (i, (name, ty)) in params.iter().enumerate() {
            self.symbol_table.last_mut().unwrap()
                .insert(name.clone(), (IRValue::Argument(i), ty.clone()));
        }

        // Build function body
        for stmt in func.body {
            self.build_statement(&mut ir_func, stmt)?;
        }

        // Ensure function has return
        if !self.has_terminator(&ir_func) {
            if ir_func.return_type == IRType::Void {
                self.emit_instruction(&mut ir_func, IRInstruction::Return { value: None });
            } else {
                return Err(format!("Function '{}' missing return statement", func_name));
            }
        }

        ir_func.local_count = self.local_counter;
        ir_func.register_count = self.register_counter;

        self.pop_scope();
        self.program.functions.insert(func_name, ir_func);
        self.current_function = None;

        Ok(())
    }

    fn build_class(&mut self, class: ClassDef) -> Result<(), String> {
        // Build methods
        for method in class.methods {
            let method_name = format!("{}.{}", class.name, method.name);
            let mut new_method = method.clone();
            new_method.name = method_name;
            self.build_function(new_method)?;
        }
        Ok(())
    }

    fn build_statement(&mut self, func: &mut IRFunction, stmt: Statement) -> Result<(), String> {
        match stmt {
            Statement::VarDecl { name, var_type, value, is_mutable } => {
                let ty = self.convert_type(&var_type);
                let local_idx = self.local_counter;
                self.local_counter += 1;

                let reg = self.next_register();
                self.emit_instruction(func, IRInstruction::Alloca {
                    dst: reg,
                    ty: ty.clone(),
                    count: None,
                });

                if let Some(val) = value {
                    let val_reg = self.build_expression(func, val)?;
                    self.emit_instruction(func, IRInstruction::Store {
                        addr: IRValue::Register(reg),
                        value: val_reg,
                        ty: ty.clone(),
                    });
                }

                self.symbol_table.last_mut().unwrap()
                    .insert(name, (IRValue::Register(reg), ty));
                Ok(())
            }

            Statement::Assignment { target, value } => {
                let addr = self.build_lvalue(func, target)?;
                let val = self.build_expression(func, value)?;
                
                // Infer type from value
                let ty = self.infer_type(&val);
                
                self.emit_instruction(func, IRInstruction::Store {
                    addr,
                    value: val,
                    ty,
                });
                Ok(())
            }

            Statement::Expression(expr) => {
                self.build_expression(func, expr)?;
                Ok(())
            }

            Statement::Return(expr) => {
                let value = if let Some(e) = expr {
                    Some(self.build_expression(func, e)?)
                } else {
                    None
                };
                self.emit_instruction(func, IRInstruction::Return { value });
                Ok(())
            }

            Statement::If { condition, then_body, elif_branches, else_body } => {
                self.build_if_statement(func, condition, then_body, elif_branches, else_body)
            }

            Statement::While { condition, body } => {
                self.build_while_loop(func, condition, body)
            }

            Statement::For { init, condition, increment, body } => {
                self.build_for_loop(func, init, condition, increment, body)
            }

            Statement::Break => {
                if let Some(ctx) = self.loop_stack.last() {
                    self.emit_instruction(func, IRInstruction::Branch {
                        target: ctx.break_block,
                    });
                    Ok(())
                } else {
                    Err("Break outside loop".to_string())
                }
            }

            Statement::Continue => {
                if let Some(ctx) = self.loop_stack.last() {
                    self.emit_instruction(func, IRInstruction::Branch {
                        target: ctx.continue_block,
                    });
                    Ok(())
                } else {
                    Err("Continue outside loop".to_string())
                }
            }

            Statement::Block(stmts) => {
                self.push_scope();
                for stmt in stmts {
                    self.build_statement(func, stmt)?;
                }
                self.pop_scope();
                Ok(())
            }

            _ => Ok(()),
        }
    }

    fn build_expression(&mut self, func: &mut IRFunction, expr: ASTValue) -> Result<IRValue, String> {
        match expr {
            ASTValue::Int(n) => Ok(IRValue::Constant(IRConstant::I32(n))),
            ASTValue::Int64(n) => Ok(IRValue::Constant(IRConstant::I64(n))),
            ASTValue::Float32(f) => Ok(IRValue::Constant(IRConstant::F32(f))),
            ASTValue::Float64(f) => Ok(IRValue::Constant(IRConstant::F64(f))),
            ASTValue::Bool(b) => Ok(IRValue::Constant(IRConstant::Bool(b))),
            ASTValue::Str(s) => {
                let id = self.program.string_literals.len();
                self.program.string_literals.insert(s.clone(), id);
                Ok(IRValue::Global(format!("__str_{}", id)))
            }
            ASTValue::Null => Ok(IRValue::Constant(IRConstant::Null)),

            ASTValue::VarRef(name) => {
                if let Some((val, ty)) = self.lookup_symbol(&name) {
                    match val {
                        IRValue::Register(reg) => {
                            let dst = self.next_register();
                            self.emit_instruction(func, IRInstruction::Load {
                                dst,
                                addr: IRValue::Register(reg),
                                ty: ty.clone(),
                            });
                            Ok(IRValue::Register(dst))
                        }
                        _ => Ok(val.clone()),
                    }
                } else {
                    Err(format!("Undefined variable: {}", name))
                }
            }

            ASTValue::Binary { op, left, right } => {
                self.build_binary_op(func, op, *left, *right)
            }

            ASTValue::Comparison { op, left, right } => {
                self.build_comparison(func, op, *left, *right)
            }

            ASTValue::FuncCall { name, args } => {
                self.build_call(func, name, args)
            }

            _ => Ok(IRValue::Undef),
        }
    }

    fn build_binary_op(&mut self, func: &mut IRFunction, op: BinaryOp, left: ASTValue, right: ASTValue) -> Result<IRValue, String> {
        let lhs = self.build_expression(func, left)?;
        let rhs = self.build_expression(func, right)?;
        let ty = self.infer_type(&lhs);
        let dst = self.next_register();

        let instr = match op {
            BinaryOp::Add => IRInstruction::Add { dst, lhs, rhs, ty },
            BinaryOp::Sub => IRInstruction::Sub { dst, lhs, rhs, ty },
            BinaryOp::Mul => IRInstruction::Mul { dst, lhs, rhs, ty },
            BinaryOp::Div => IRInstruction::Div { dst, lhs, rhs, ty },
            BinaryOp::Mod => IRInstruction::Mod { dst, lhs, rhs, ty },
            BinaryOp::And => IRInstruction::And { dst, lhs, rhs },
            BinaryOp::Or => IRInstruction::Or { dst, lhs, rhs },
            BinaryOp::BitXor => IRInstruction::Xor { dst, lhs, rhs },
            BinaryOp::Shl => IRInstruction::Shl { dst, lhs, rhs },
            BinaryOp::Shr => IRInstruction::Shr { dst, lhs, rhs, signed: true },
            _ => return Err("Unsupported binary operation".to_string()),
        };

        self.emit_instruction(func, instr);
        Ok(IRValue::Register(dst))
    }

    fn build_comparison(&mut self, func: &mut IRFunction, op: ComparisonOp, left: ASTValue, right: ASTValue) -> Result<IRValue, String> {
        let lhs = self.build_expression(func, left)?;
        let rhs = self.build_expression(func, right)?;
        let ty = self.infer_type(&lhs);
        let dst = self.next_register();

        let cmp_op = match op {
            ComparisonOp::Equal => CmpOp::Eq,
            ComparisonOp::NotEqual => CmpOp::Ne,
            ComparisonOp::Less => CmpOp::Lt,
            ComparisonOp::LessEqual => CmpOp::Le,
            ComparisonOp::Greater => CmpOp::Gt,
            ComparisonOp::GreaterEqual => CmpOp::Ge,
        };

        self.emit_instruction(func, IRInstruction::Cmp { dst, op: cmp_op, lhs, rhs, ty });
        Ok(IRValue::Register(dst))
    }

    fn build_call(&mut self, func: &mut IRFunction, name: String, args: Vec<ASTValue>) -> Result<IRValue, String> {
        let mut arg_vals = Vec::new();
        for arg in args {
            arg_vals.push(self.build_expression(func, arg)?);
        }

        let dst = self.next_register();
        self.emit_instruction(func, IRInstruction::Call {
            dst: Some(dst),
            func: name,
            args: arg_vals,
            ty: IRType::I32, // TODO: lookup actual return type
        });

        Ok(IRValue::Register(dst))
    }

    fn build_if_statement(&mut self, func: &mut IRFunction, condition: ASTValue, then_body: Vec<Statement>, elif_branches: Vec<(ASTValue, Vec<Statement>)>, else_body: Option<Vec<Statement>>) -> Result<(), String> {
        let cond_val = self.build_expression(func, condition)?;
        
        let then_block = self.create_block_id();
        let else_block = if !elif_branches.is_empty() || else_body.is_some() {
            self.create_block_id()
        } else {
            self.create_block_id() // merge block
        };
        let merge_block = self.create_block_id();

        self.emit_instruction(func, IRInstruction::CondBranch {
            cond: cond_val,
            true_target: then_block,
            false_target: else_block,
        });

        // Then block
        self.switch_to_block(func, then_block);
        for stmt in then_body {
            self.build_statement(func, stmt)?;
        }
        if !self.has_terminator(func) {
            self.emit_instruction(func, IRInstruction::Branch { target: merge_block });
        }

        // Else/elif blocks
        if !elif_branches.is_empty() || else_body.is_some() {
            self.switch_to_block(func, else_block);
            
            if let Some(else_stmts) = else_body {
                for stmt in else_stmts {
                    self.build_statement(func, stmt)?;
                }
            }
            
            if !self.has_terminator(func) {
                self.emit_instruction(func, IRInstruction::Branch { target: merge_block });
            }
        }

        self.switch_to_block(func, merge_block);
        Ok(())
    }

    fn build_while_loop(&mut self, func: &mut IRFunction, condition: ASTValue, body: Vec<Statement>) -> Result<(), String> {
        let header_block = self.create_block_id();
        let body_block = self.create_block_id();
        let exit_block = self.create_block_id();

        self.emit_instruction(func, IRInstruction::Branch { target: header_block });

        // Header block
        self.switch_to_block(func, header_block);
        let cond_val = self.build_expression(func, condition)?;
        self.emit_instruction(func, IRInstruction::CondBranch {
            cond: cond_val,
            true_target: body_block,
            false_target: exit_block,
        });

        // Body block
        self.switch_to_block(func, body_block);
        self.loop_stack.push(LoopContext {
            continue_block: header_block,
            break_block: exit_block,
        });
        
        for stmt in body {
            self.build_statement(func, stmt)?;
        }
        
        if !self.has_terminator(func) {
            self.emit_instruction(func, IRInstruction::Branch { target: header_block });
        }
        
        self.loop_stack.pop();

        // Exit block
        self.switch_to_block(func, exit_block);
        Ok(())
    }

    fn build_for_loop(&mut self, func: &mut IRFunction, init: Option<Box<Statement>>, condition: Option<ASTValue>, increment: Option<Box<Statement>>, body: Vec<Statement>) -> Result<(), String> {
        if let Some(init_stmt) = init {
            self.build_statement(func, *init_stmt)?;
        }

        let header_block = self.create_block_id();
        let body_block = self.create_block_id();
        let inc_block = self.create_block_id();
        let exit_block = self.create_block_id();

        self.emit_instruction(func, IRInstruction::Branch { target: header_block });

        // Header
        self.switch_to_block(func, header_block);
        if let Some(cond) = condition {
            let cond_val = self.build_expression(func, cond)?;
            self.emit_instruction(func, IRInstruction::CondBranch {
                cond: cond_val,
                true_target: body_block,
                false_target: exit_block,
            });
        } else {
            self.emit_instruction(func, IRInstruction::Branch { target: body_block });
        }

        // Body
        self.switch_to_block(func, body_block);
        self.loop_stack.push(LoopContext {
            continue_block: inc_block,
            break_block: exit_block,
        });
        
        for stmt in body {
            self.build_statement(func, stmt)?;
        }
        
        if !self.has_terminator(func) {
            self.emit_instruction(func, IRInstruction::Branch { target: inc_block });
        }
        
        self.loop_stack.pop();

        // Increment
        self.switch_to_block(func, inc_block);
        if let Some(inc_stmt) = increment {
            self.build_statement(func, *inc_stmt)?;
        }
        self.emit_instruction(func, IRInstruction::Branch { target: header_block });

        // Exit
        self.switch_to_block(func, exit_block);
        Ok(())
    }

    // Helper methods
    fn convert_type(&self, ty: &Type) -> IRType {
        match ty {
            Type::I32 => IRType::I32,
            Type::I64 => IRType::I64,
            Type::F32 => IRType::F32,
            Type::F64 => IRType::F64,
            Type::Bool => IRType::Bool,
            Type::String => IRType::Ptr(Box::new(IRType::I8)),
            Type::Void => IRType::Void,
            Type::Array(inner) => IRType::Array(Box::new(self.convert_type(inner)), None),
            _ => IRType::I32,
        }
    }

    fn infer_type(&self, val: &IRValue) -> IRType {
        match val {
            IRValue::Constant(c) => match c {
                IRConstant::I8(_) => IRType::I8,
                IRConstant::I16(_) => IRType::I16,
                IRConstant::I32(_) => IRType::I32,
                IRConstant::I64(_) => IRType::I64,
                IRConstant::F32(_) => IRType::F32,
                IRConstant::F64(_) => IRType::F64,
                IRConstant::Bool(_) => IRType::Bool,
                _ => IRType::I32,
            },
            _ => IRType::I32,
        }
    }

    fn build_lvalue(&mut self, func: &mut IRFunction, expr: ASTValue) -> Result<IRValue, String> {
        match expr {
            ASTValue::VarRef(name) => {
                if let Some((val, _)) = self.lookup_symbol(&name) {
                    Ok(val.clone())
                } else {
                    Err(format!("Undefined variable: {}", name))
                }
            }
            _ => Err("Invalid lvalue".to_string()),
        }
    }

    fn next_register(&mut self) -> usize {
        let reg = self.register_counter;
        self.register_counter += 1;
        reg
    }

    fn create_block(&mut self) -> IRBasicBlock {
        let id = self.block_counter;
        self.block_counter += 1;
        IRBasicBlock::new(id)
    }

    fn create_block_id(&mut self) -> usize {
        self.block_counter += 1;
        self.block_counter - 1
    }

    fn switch_to_block(&mut self, func: &mut IRFunction, block_id: usize) {
        while func.blocks.len() <= block_id {
            func.blocks.push(IRBasicBlock::new(func.blocks.len()));
        }
        self.current_block = block_id;
    }

    fn emit_instruction(&mut self, func: &mut IRFunction, instr: IRInstruction) {
        if self.current_block >= func.blocks.len() {
            func.blocks.push(IRBasicBlock::new(self.current_block));
        }
        func.blocks[self.current_block].instructions.push(instr);
    }

    fn has_terminator(&self, func: &IRFunction) -> bool {
        if let Some(block) = func.blocks.get(self.current_block) {
            if let Some(last) = block.instructions.last() {
                matches!(last, IRInstruction::Return { .. } | IRInstruction::Branch { .. } | IRInstruction::CondBranch { .. })
            } else {
                false
            }
        } else {
            false
        }
    }

    fn push_scope(&mut self) {
        self.symbol_table.push(HashMap::new());
    }

    fn pop_scope(&mut self) {
        self.symbol_table.pop();
    }

    fn lookup_symbol(&self, name: &str) -> Option<(IRValue, IRType)> {
        for scope in self.symbol_table.iter().rev() {
            if let Some(val) = scope.get(name) {
                return Some(val.clone());
            }
        }
        None
    }
}
