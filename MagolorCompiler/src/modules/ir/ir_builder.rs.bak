// src/modules/ir/ir_builder.rs - COMPREHENSIVE FIX
// Complete rewrite with proper control flow and variable handling

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
    current_namespace: Vec<String>,
    current_class: Option<String>,
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
            current_namespace: Vec::new(),
            current_class: None,
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
            TopLevel::Import(_) => Ok(()),
            TopLevel::Function(func) => self.build_function(func),
            TopLevel::Class(class) => self.build_class(class),
            TopLevel::Namespace { name, items } => {
                self.current_namespace.push(name);
                for item in items {
                    self.process_top_level(item)?;
                }
                self.current_namespace.pop();
                Ok(())
            }
            _ => Ok(()),
        }
    }

    fn get_full_name(&self, base_name: &str) -> String {
        let mut parts = self.current_namespace.clone();
        if let Some(ref class) = self.current_class {
            parts.push(class.clone());
        }
        parts.push(base_name.to_string());
        parts.join(".")
    }

    fn build_function(&mut self, func: FunctionDef) -> Result<(), String> {
        let func_name = self.get_full_name(&func.name);
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

        let entry_block = self.create_block();
        ir_func.blocks.push(entry_block);
        self.current_block = 0;

        self.push_scope();
        for (i, (name, ty)) in params.iter().enumerate() {
            self.symbol_table
                .last_mut()
                .unwrap()
                .insert(name.clone(), (IRValue::Argument(i), ty.clone()));
        }

        for stmt in func.body {
            self.build_statement(&mut ir_func, stmt)?;
            // Stop processing if we hit a return
            if self.has_terminator(&ir_func) {
                break;
            }
        }

        // Add default return if needed
        if !self.has_terminator(&ir_func) {
            if ir_func.return_type == IRType::Void {
                self.emit_instruction(&mut ir_func, IRInstruction::Return { value: None });
            } else {
                // Return default value for non-void functions
                let default_val = match ir_func.return_type {
                    IRType::I32 | IRType::I64 | IRType::I8 | IRType::I16 => {
                        IRValue::Constant(IRConstant::I32(0))
                    }
                    IRType::F32 => IRValue::Constant(IRConstant::F32(0.0.into())),
                    IRType::F64 => IRValue::Constant(IRConstant::F64(0.0.into())),
                    IRType::Bool => IRValue::Constant(IRConstant::Bool(false)),
                    _ => IRValue::Constant(IRConstant::Null),
                };
                self.emit_instruction(&mut ir_func, IRInstruction::Return { value: Some(default_val) });
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
        let class_name = class.name.clone();
        self.current_class = Some(class_name.clone());

        // Register static fields as globals
        for field in &class.fields {
            if field.is_static {
                let global_name = self.get_full_name(&field.name);
                let ty = self.convert_type(&field.field_type);
                let init = field
                    .default_value
                    .as_ref()
                    .and_then(|v| self.ast_value_to_constant(v));

                self.program.globals.insert(
                    global_name.clone(),
                    IRGlobal {
                        name: global_name.clone(),
                        ty: ty.clone(),
                        init,
                        is_const: false,
                        alignment: ty.alignment(),
                    },
                );

                self.symbol_table[0].insert(field.name.clone(), (IRValue::Global(global_name), ty));
            }
        }

        for method in class.methods {
            self.build_function(method)?;
        }

        self.current_class = None;
        Ok(())
    }

    fn build_statement(&mut self, func: &mut IRFunction, stmt: Statement) -> Result<(), String> {
        match stmt {
            Statement::VarDecl { name, var_type, value, is_mutable: _ } => {
                let ty = self.convert_type(&var_type);
                let local_idx = self.local_counter;
                self.local_counter += 1;
                let addr = IRValue::Local(local_idx);

                if let Some(val) = value {
                    let val_reg = self.build_expression(func, val)?;
                    self.emit_instruction(func, IRInstruction::Store {
                        addr: addr.clone(),
                        value: val_reg,
                        ty: ty.clone(),
                    });
                } else {
                    // Initialize to zero
                    let zero = match ty {
                        IRType::F32 => IRValue::Constant(IRConstant::F32(0.0.into())),
                        IRType::F64 => IRValue::Constant(IRConstant::F64(0.0.into())),
                        _ => IRValue::Constant(IRConstant::I32(0)),
                    };
                    self.emit_instruction(func, IRInstruction::Store {
                        addr: addr.clone(),
                        value: zero,
                        ty: ty.clone(),
                    });
                }

                self.symbol_table.last_mut().unwrap().insert(name, (addr, ty));
                Ok(())
            }

            Statement::Assignment { target, value } => {
                let addr = self.build_lvalue(func, target)?;
                let val = self.build_expression(func, value)?;
                let ty = self.infer_type(&val);
                self.emit_instruction(func, IRInstruction::Store { addr, value: val, ty });
                Ok(())
            }

            Statement::CompoundAssignment { target, op, value } => {
                // Load current value
                let addr = self.build_lvalue(func, target)?;
                let current_reg = self.next_register();
                let ty = IRType::I32; // TODO: proper type inference
                
                self.emit_instruction(func, IRInstruction::Load {
                    dst: current_reg,
                    addr: addr.clone(),
                    ty: ty.clone(),
                });

                // Compute new value
                let val_reg = self.build_expression(func, value)?;
                let result_reg = self.next_register();

                let instr = match op {
                    BinaryOp::Add => IRInstruction::Add {
                        dst: result_reg,
                        lhs: IRValue::Register(current_reg),
                        rhs: val_reg,
                        ty: ty.clone(),
                    },
                    BinaryOp::Sub => IRInstruction::Sub {
                        dst: result_reg,
                        lhs: IRValue::Register(current_reg),
                        rhs: val_reg,
                        ty: ty.clone(),
                    },
                    BinaryOp::Mul => IRInstruction::Mul {
                        dst: result_reg,
                        lhs: IRValue::Register(current_reg),
                        rhs: val_reg,
                        ty: ty.clone(),
                    },
                    BinaryOp::Div => IRInstruction::Div {
                        dst: result_reg,
                        lhs: IRValue::Register(current_reg),
                        rhs: val_reg,
                        ty: ty.clone(),
                    },
                    _ => return Err("Unsupported compound assignment operator".to_string()),
                };

                self.emit_instruction(func, instr);
                self.emit_instruction(func, IRInstruction::Store {
                    addr,
                    value: IRValue::Register(result_reg),
                    ty,
                });
                Ok(())
            }

            Statement::Expression(expr) => {
                self.build_expression(func, expr)?;
                Ok(())
            }

            Statement::Return(expr) => {
                let value = expr.map(|e| self.build_expression(func, e)).transpose()?;
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
                    self.emit_instruction(func, IRInstruction::Branch { target: ctx.break_block });
                    Ok(())
                } else {
                    Err("Break outside loop".to_string())
                }
            }

            Statement::Continue => {
                if let Some(ctx) = self.loop_stack.last() {
                    self.emit_instruction(func, IRInstruction::Branch { target: ctx.continue_block });
                    Ok(())
                } else {
                    Err("Continue outside loop".to_string())
                }
            }

            Statement::Block(stmts) => {
                self.push_scope();
                for stmt in stmts {
                    self.build_statement(func, stmt)?;
                    if self.has_terminator(func) {
                        break;
                    }
                }
                self.pop_scope();
                Ok(())
            }

            Statement::Match { value, arms } => {
                self.build_match_statement(func, value, arms)
            }

            _ => Ok(()),
        }
    }

    fn build_match_statement(
        &mut self,
        func: &mut IRFunction,
        value: ASTValue,
        arms: Vec<MatchArm>,
    ) -> Result<(), String> {
        let val_reg = self.build_expression(func, value)?;
        let merge_block = self.create_block_id();
        let mut next_check_block = self.create_block_id();

        for (i, arm) in arms.iter().enumerate() {
            self.switch_to_block(func, next_check_block);
            
            match &arm.pattern {
                Pattern::Literal(lit_val) => {
                    let lit_reg = self.build_expression(func, lit_val.clone())?;
                    let cmp_reg = self.next_register();
                    
                    self.emit_instruction(func, IRInstruction::Cmp {
                        dst: cmp_reg,
                        op: CmpOp::Eq,
                        lhs: val_reg.clone(),
                        rhs: lit_reg,
                        ty: IRType::I32,
                    });

                    let body_block = self.create_block_id();
                    let next_block = if i < arms.len() - 1 {
                        self.create_block_id()
                    } else {
                        merge_block
                    };

                    self.emit_instruction(func, IRInstruction::CondBranch {
                        cond: IRValue::Register(cmp_reg),
                        true_target: body_block,
                        false_target: next_block,
                    });

                    self.switch_to_block(func, body_block);
                    for stmt in &arm.body {
                        self.build_statement(func, stmt.clone())?;
                        if self.has_terminator(func) {
                            break;
                        }
                    }
                    if !self.has_terminator(func) {
                        self.emit_instruction(func, IRInstruction::Branch { target: merge_block });
                    }

                    next_check_block = next_block;
                }
                Pattern::Wildcard => {
                    // Default case - always matches
                    for stmt in &arm.body {
                        self.build_statement(func, stmt.clone())?;
                        if self.has_terminator(func) {
                            break;
                        }
                    }
                    if !self.has_terminator(func) {
                        self.emit_instruction(func, IRInstruction::Branch { target: merge_block });
                    }
                    break;
                }
                _ => return Err("Unsupported match pattern".to_string()),
            }
        }

        self.switch_to_block(func, merge_block);
        Ok(())
    }

    fn build_expression(&mut self, func: &mut IRFunction, expr: ASTValue) -> Result<IRValue, String> {
        match expr {
            ASTValue::Int(n) => Ok(IRValue::Constant(IRConstant::I32(n))),
            ASTValue::Int64(n) => Ok(IRValue::Constant(IRConstant::I64(n))),
            ASTValue::Float32(f) => Ok(IRValue::Constant(IRConstant::F32(f.into()))),
            ASTValue::Float64(f) => Ok(IRValue::Constant(IRConstant::F64(f.into()))),
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
                        IRValue::Local(_) | IRValue::Global(_) => {
                            let dst = self.next_register();
                            self.emit_instruction(func, IRInstruction::Load {
                                dst,
                                addr: val.clone(),
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

            ASTValue::Unary { op, operand } => {
                self.build_unary_op(func, op, *operand)
            }

            ASTValue::Binary { op: BinaryOp::Add, left, right } => {
                let lhs = self.build_expression(func, *left)?;
                let rhs = self.build_expression(func, *right)?;

                // Check for string concatenation
                let is_string = matches!(&lhs, IRValue::Global(s) if s.starts_with("__str_"))
                    || matches!(&rhs, IRValue::Global(s) if s.starts_with("__str_"));

                if is_string {
                    let dst = self.next_register();
                    self.emit_instruction(func, IRInstruction::Call {
                        dst: Some(dst),
                        func: "string_concat_int".to_string(),
                        args: vec![lhs, rhs],
                        ty: IRType::Ptr(Box::new(IRType::I8)),
                    });
                    return Ok(IRValue::Register(dst));
                }

                let ty = self.infer_type(&lhs);
                let dst = self.next_register();
                self.emit_instruction(func, IRInstruction::Add { dst, lhs, rhs, ty });
                Ok(IRValue::Register(dst))
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

            ASTValue::MethodCall { object, method, args } => {
                let func_name = match *object {
                    ASTValue::VarRef(obj_name) => format!("{}.{}", obj_name, method),
                    ASTValue::MemberAccess { object: nested_obj, member } => {
                        match *nested_obj {
                            ASTValue::VarRef(namespace) => format!("{}.{}.{}", namespace, member, method),
                            _ => return Err("Complex member access not supported".to_string()),
                        }
                    }
                    _ => return Err("Unsupported method call object".to_string()),
                };

                let mut arg_vals = Vec::new();
                for arg in args {
                    arg_vals.push(self.build_expression(func, arg)?);
                }

                let dst = self.next_register();
                self.emit_instruction(func, IRInstruction::Call {
                    dst: Some(dst),
                    func: func_name,
                    args: arg_vals,
                    ty: IRType::I32,
                });

                Ok(IRValue::Register(dst))
            }

            _ => Ok(IRValue::Undef),
        }
    }

    fn build_unary_op(&mut self, func: &mut IRFunction, op: UnaryOp, operand: ASTValue) -> Result<IRValue, String> {
        match op {
            UnaryOp::PostInc | UnaryOp::PreInc => {
                // Get the address
                let addr = self.build_lvalue(func, operand)?;
                
                // Load current value
                let current_reg = self.next_register();
                self.emit_instruction(func, IRInstruction::Load {
                    dst: current_reg,
                    addr: addr.clone(),
                    ty: IRType::I32,
                });

                // Increment
                let result_reg = self.next_register();
                self.emit_instruction(func, IRInstruction::Add {
                    dst: result_reg,
                    lhs: IRValue::Register(current_reg),
                    rhs: IRValue::Constant(IRConstant::I32(1)),
                    ty: IRType::I32,
                });

                // Store back
                self.emit_instruction(func, IRInstruction::Store {
                    addr,
                    value: IRValue::Register(result_reg),
                    ty: IRType::I32,
                });

                // Return appropriate value
                Ok(if matches!(op, UnaryOp::PreInc) {
                    IRValue::Register(result_reg)
                } else {
                    IRValue::Register(current_reg)
                })
            }
            _ => Err(format!("Unary operator {:?} not yet implemented", op)),
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

        // Resolve function name - if it's not qualified, try current class context
        let resolved_name = if !name.contains('.') && self.current_class.is_some() {
            self.get_full_name(&name)
        } else {
            name
        };

        let dst = self.next_register();
        self.emit_instruction(func, IRInstruction::Call {
            dst: Some(dst),
            func: resolved_name,
            args: arg_vals,
            ty: IRType::I32,
        });

        Ok(IRValue::Register(dst))
    }

    fn build_if_statement(
        &mut self,
        func: &mut IRFunction,
        condition: ASTValue,
        then_body: Vec<Statement>,
        _elif_branches: Vec<(ASTValue, Vec<Statement>)>,
        else_body: Option<Vec<Statement>>,
    ) -> Result<(), String> {
        let cond_val = self.build_expression(func, condition)?;

        let then_block = self.create_block_id();
        let else_block = self.create_block_id();
        let merge_block = self.create_block_id();

        self.emit_instruction(func, IRInstruction::CondBranch {
            cond: cond_val,
            true_target: then_block,
            false_target: else_block,
        });

        // Then branch
        self.switch_to_block(func, then_block);
        for stmt in then_body {
            self.build_statement(func, stmt)?;
            if self.has_terminator(func) {
                break;
            }
        }
        if !self.has_terminator(func) {
            self.emit_instruction(func, IRInstruction::Branch { target: merge_block });
        }

        // Else branch
        self.switch_to_block(func, else_block);
        if let Some(else_stmts) = else_body {
            for stmt in else_stmts {
                self.build_statement(func, stmt)?;
                if self.has_terminator(func) {
                    break;
                }
            }
        }
        if !self.has_terminator(func) {
            self.emit_instruction(func, IRInstruction::Branch { target: merge_block });
        }

        self.switch_to_block(func, merge_block);
        Ok(())
    }

    fn build_while_loop(&mut self, func: &mut IRFunction, condition: ASTValue, body: Vec<Statement>) -> Result<(), String> {
        let header_block = self.create_block_id();
        let body_block = self.create_block_id();
        let exit_block = self.create_block_id();

        self.emit_instruction(func, IRInstruction::Branch { target: header_block });

        self.switch_to_block(func, header_block);
        let cond_val = self.build_expression(func, condition)?;
        self.emit_instruction(func, IRInstruction::CondBranch {
            cond: cond_val,
            true_target: body_block,
            false_target: exit_block,
        });

        self.switch_to_block(func, body_block);
        self.loop_stack.push(LoopContext {
            continue_block: header_block,
            break_block: exit_block,
        });

        for stmt in body {
            self.build_statement(func, stmt)?;
            if self.has_terminator(func) {
                break;
            }
        }

        if !self.has_terminator(func) {
            self.emit_instruction(func, IRInstruction::Branch { target: header_block });
        }

        self.loop_stack.pop();
        self.switch_to_block(func, exit_block);
        Ok(())
    }

    fn build_for_loop(
        &mut self,
        func: &mut IRFunction,
        init: Option<Box<Statement>>,
        condition: Option<ASTValue>,
        increment: Option<Box<Statement>>,
        body: Vec<Statement>,
    ) -> Result<(), String> {
        if let Some(init_stmt) = init {
            self.build_statement(func, *init_stmt)?;
        }

        let header_block = self.create_block_id();
        let body_block = self.create_block_id();
        let inc_block = self.create_block_id();
        let exit_block = self.create_block_id();

        self.emit_instruction(func, IRInstruction::Branch { target: header_block });

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

        self.switch_to_block(func, body_block);
        self.loop_stack.push(LoopContext {
            continue_block: inc_block,
            break_block: exit_block,
        });

        for stmt in body {
            self.build_statement(func, stmt)?;
            if self.has_terminator(func) {
                break;
            }
        }

        if !self.has_terminator(func) {
            self.emit_instruction(func, IRInstruction::Branch { target: inc_block });
        }

        self.loop_stack.pop();

        self.switch_to_block(func, inc_block);
        if let Some(inc_stmt) = increment {
            self.build_statement(func, *inc_stmt)?;
        }
        self.emit_instruction(func, IRInstruction::Branch { target: header_block });

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

    fn build_lvalue(&mut self, _func: &mut IRFunction, expr: ASTValue) -> Result<IRValue, String> {
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

    fn ast_value_to_constant(&self, val: &ASTValue) -> Option<IRConstant> {
        match val {
            ASTValue::Int(n) => Some(IRConstant::I32(*n)),
            ASTValue::Int64(n) => Some(IRConstant::I64(*n)),
            ASTValue::Float32(f) => Some(IRConstant::F32((*f).into())),
            ASTValue::Float64(f) => Some(IRConstant::F64((*f).into())),
            ASTValue::Bool(b) => Some(IRConstant::Bool(*b)),
            ASTValue::Str(s) => Some(IRConstant::String(s.clone())),
            ASTValue::Null => Some(IRConstant::Null),
            _ => None,
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
                matches!(
                    last,
                    IRInstruction::Return { .. }
                        | IRInstruction::Branch { .. }
                        | IRInstruction::CondBranch { .. }
                )
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