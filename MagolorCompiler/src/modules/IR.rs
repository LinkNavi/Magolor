// src/modules/IR.rs - Complete LLVM compiler implementation

use inkwell::builder::Builder;
use inkwell::context::Context;
use inkwell::module::Module;
use inkwell::types::{BasicType, BasicTypeEnum};
use inkwell::values::{BasicValue, BasicValueEnum, FunctionValue, IntValue, PointerValue};
use inkwell::AddressSpace;
use inkwell::IntPredicate;
use std::collections::HashMap;

use crate::modules::ast::*;

pub struct LLVMCompiler<'ctx> {
    context: &'ctx Context,
    module: Module<'ctx>,
    builder: Builder<'ctx>,
    variables: HashMap<String, (PointerValue<'ctx>, BasicTypeEnum<'ctx>)>,
    current_function: Option<FunctionValue<'ctx>>,
}

impl<'ctx> LLVMCompiler<'ctx> {
    pub fn new(context: &'ctx Context, module_name: &str) -> Self {
        let module = context.create_module(module_name);
        let builder = context.create_builder();

        Self {
            context,
            module,
            builder,
            variables: HashMap::new(),
            current_function: None,
        }
    }

    pub fn compile(&mut self, program: Program) -> Result<String, String> {
        // Declare runtime functions
        self.declare_runtime_functions();

        // Process all items
        for item in program {
            self.compile_top_level(item)?;
        }

        // Generate LLVM IR
        Ok(self.module.print_to_string().to_string())
    }

    fn declare_runtime_functions(&mut self) {
        let i32_type = self.context.i32_type();
        let i8_ptr_type = self.context.ptr_type(AddressSpace::default());

        // void printf(char*, ...)
        let printf_type = self.context.i32_type().fn_type(&[i8_ptr_type.into()], true);
        self.module.add_function("printf", printf_type, None);

        // void* malloc(i64)
        let malloc_type = i8_ptr_type.fn_type(&[self.context.i64_type().into()], false);
        self.module.add_function("malloc", malloc_type, None);

        // void free(void*)
        let free_type = self
            .context
            .void_type()
            .fn_type(&[i8_ptr_type.into()], false);
        self.module.add_function("free", free_type, None);

        // ADD THESE NEW DECLARATIONS:
        // void print_int(i32)
        let print_int_type = self.context.void_type().fn_type(&[i32_type.into()], false);
        self.module.add_function("print_int", print_int_type, None);

        // void print_str(char*)
        let print_str_type = self
            .context
            .void_type()
            .fn_type(&[i8_ptr_type.into()], false);
        self.module.add_function("print_str", print_str_type, None);

        // void print_f32(f32)
        let print_f32_type = self
            .context
            .void_type()
            .fn_type(&[self.context.f32_type().into()], false);
        self.module.add_function("print_f32", print_f32_type, None);

        // void print_f64(f64)
        let print_f64_type = self
            .context
            .void_type()
            .fn_type(&[self.context.f64_type().into()], false);
        self.module.add_function("print_f64", print_f64_type, None);

        println!("Declared runtime functions:");
        for f in self.module.get_functions() {
            println!(" - {}", f.get_name().to_str().unwrap_or("?"));
        }
    }
    fn compile_top_level(&mut self, item: TopLevel) -> Result<(), String> {
        match item {
            TopLevel::Function(func) => self.compile_function(func),
            TopLevel::Class(class) => self.compile_class(class),
            TopLevel::Namespace { items, .. } => {
                for item in items {
                    self.compile_top_level(item)?;
                }
                Ok(())
            }
            _ => Ok(()),
        }
    }

    fn compile_class(&mut self, class: ClassDef) -> Result<(), String> {
        // Compile static fields as globals
        for field in &class.fields {
            if field.is_static {
                let global_name = format!("{}.{}", class.name, field.name);
                let field_type = self.llvm_type(&field.field_type)?;
                let global = self.module.add_global(field_type, None, &global_name);

                // Set initializer
                if let Some(default_val) = &field.default_value {
                    // For now, just initialize to zero
                    if let BasicTypeEnum::IntType(int_type) = field_type {
                        global.set_initializer(&int_type.const_zero());
                    }
                }
            }
        }

        // Compile methods
        for method in class.methods {
            let full_name = format!("{}.{}", class.name, method.name);
            let mut modified_method = method;
            modified_method.name = full_name;
            self.compile_function(modified_method)?;
        }

        Ok(())
    }

    fn llvm_type(&self, ty: &Type) -> Result<BasicTypeEnum<'ctx>, String> {
        match ty {
            Type::I32 => Ok(self.context.i32_type().into()),
            Type::I64 => Ok(self.context.i64_type().into()),
            Type::F32 => Ok(self.context.f32_type().into()),
            Type::F64 => Ok(self.context.f64_type().into()),
            Type::Bool => Ok(self.context.bool_type().into()),
            Type::String => Ok(self.context.ptr_type(AddressSpace::default()).into()),
            Type::Array(inner) => {
                // Arrays as pointers for simplicity
                Ok(self.context.ptr_type(AddressSpace::default()).into())
            }
            _ => Ok(self.context.i32_type().into()), // Default to i32
        }
    }

    fn compile_function(&mut self, func: FunctionDef) -> Result<(), String> {
        // Build parameter types
        let param_types: Vec<BasicTypeEnum> = func
            .params
            .iter()
            .map(|p| self.llvm_type(&p.param_type))
            .collect::<Result<Vec<_>, _>>()?;

        let param_types_ref: Vec<_> = param_types.iter().map(|t| (*t).into()).collect();

        // Build return type
        let fn_type = if func.return_type == Type::Void {
            self.context.void_type().fn_type(&param_types_ref, false)
        } else {
            let ret_type = self.llvm_type(&func.return_type)?;
            ret_type.fn_type(&param_types_ref, false)
        };

        let function = self.module.add_function(&func.name, fn_type, None);
        self.current_function = Some(function);

        let entry = self.context.append_basic_block(function, "entry");
        self.builder.position_at_end(entry);

        // Clear variables for new function scope
        self.variables.clear();

        // Allocate and store parameters
        for (i, param) in func.params.iter().enumerate() {
            if let Some(arg) = function.get_nth_param(i as u32) {
                let param_type = self.llvm_type(&param.param_type)?;
                let alloca = self
                    .builder
                    .build_alloca(param_type, &param.name)
                    .map_err(|e| format!("Failed to build alloca: {:?}", e))?;
                self.builder
                    .build_store(alloca, arg)
                    .map_err(|e| format!("Failed to store param: {:?}", e))?;
                self.variables
                    .insert(param.name.clone(), (alloca, param_type));
            }
        }

        // Compile body
        let mut has_terminator = false;
        for stmt in func.body {
            self.compile_statement(stmt)?;
            if let Some(block) = self.builder.get_insert_block() {
                if block.get_terminator().is_some() {
                    has_terminator = true;
                    break;
                }
            }
        }

        // Add return if missing
        if !has_terminator {
            if let Some(block) = self.builder.get_insert_block() {
                if block.get_terminator().is_none() {
                    if func.return_type == Type::Void {
                        self.builder
                            .build_return(None)
                            .map_err(|e| format!("Failed to build return: {:?}", e))?;
                    } else {
                        let ret_type = self.llvm_type(&func.return_type)?;
                        if let BasicTypeEnum::IntType(int_type) = ret_type {
                            self.builder
                                .build_return(Some(&int_type.const_zero()))
                                .map_err(|e| format!("Failed to build return: {:?}", e))?;
                        }
                    }
                }
            }
        }

        Ok(())
    }

    fn compile_statement(&mut self, stmt: Statement) -> Result<(), String> {
        match stmt {
            Statement::VarDecl {
                name,
                var_type,
                value,
                ..
            } => {
                let llvm_type = self.llvm_type(&var_type)?;
                let alloca = self
                    .builder
                    .build_alloca(llvm_type, &name)
                    .map_err(|e| format!("Failed to build alloca: {:?}", e))?;

                if let Some(init_val) = value {
                    let val = self.compile_expression(init_val)?;
                    self.builder
                        .build_store(alloca, val)
                        .map_err(|e| format!("Failed to store: {:?}", e))?;
                }

                self.variables.insert(name, (alloca, llvm_type));
                Ok(())
            }

            Statement::Assignment { target, value } => {
                let val = self.compile_expression(value)?;

                if let ASTValue::VarRef(name) = target {
                    if let Some((var_ptr, _)) = self.variables.get(&name) {
                        self.builder
                            .build_store(*var_ptr, val)
                            .map_err(|e| format!("Failed to store: {:?}", e))?;
                    } else {
                        return Err(format!("Unknown variable: {}", name));
                    }
                }
                Ok(())
            }

            Statement::Return(val) => {
                let function = self.current_function.ok_or("Return outside function")?;

                if let Some(expr) = val {
                    let ret_val = self.compile_expression(expr)?;
                    self.builder
                        .build_return(Some(&ret_val))
                        .map_err(|e| format!("Failed to build return: {:?}", e))?;
                } else {
                    self.builder
                        .build_return(None)
                        .map_err(|e| format!("Failed to build return: {:?}", e))?;
                }
                Ok(())
            }

            Statement::If {
                condition,
                then_body,
                elif_branches,
                else_body,
            } => {
                let function = self
                    .current_function
                    .ok_or("If statement outside function")?;

                let cond_val = self.compile_expression(condition)?;
                let cond_bool = if cond_val.is_int_value() {
                    let int_val = cond_val.into_int_value();
                    self.builder
                        .build_int_compare(
                            IntPredicate::NE,
                            int_val,
                            int_val.get_type().const_zero(),
                            "ifcond",
                        )
                        .map_err(|e| format!("Failed to build comparison: {:?}", e))?
                } else {
                    return Err("Condition must be integer type".to_string());
                };

                let then_bb = self.context.append_basic_block(function, "then");
                let else_bb = self.context.append_basic_block(function, "else");
                let merge_bb = self.context.append_basic_block(function, "ifcont");

                self.builder
                    .build_conditional_branch(cond_bool, then_bb, else_bb)
                    .map_err(|e| format!("Failed to build conditional branch: {:?}", e))?;

                // Then block
                self.builder.position_at_end(then_bb);
                for stmt in then_body {
                    self.compile_statement(stmt)?;
                }
                if self
                    .builder
                    .get_insert_block()
                    .unwrap()
                    .get_terminator()
                    .is_none()
                {
                    self.builder
                        .build_unconditional_branch(merge_bb)
                        .map_err(|e| format!("Failed to build branch: {:?}", e))?;
                }

                // Else block
                self.builder.position_at_end(else_bb);
                if let Some(else_stmts) = else_body {
                    for stmt in else_stmts {
                        self.compile_statement(stmt)?;
                    }
                }
                if self
                    .builder
                    .get_insert_block()
                    .unwrap()
                    .get_terminator()
                    .is_none()
                {
                    self.builder
                        .build_unconditional_branch(merge_bb)
                        .map_err(|e| format!("Failed to build branch: {:?}", e))?;
                }

                self.builder.position_at_end(merge_bb);
                Ok(())
            }

            Statement::While { condition, body } => {
                let function = self.current_function.ok_or("While loop outside function")?;

                let cond_bb = self.context.append_basic_block(function, "whilecond");
                let body_bb = self.context.append_basic_block(function, "whilebody");
                let after_bb = self.context.append_basic_block(function, "afterwhile");

                self.builder
                    .build_unconditional_branch(cond_bb)
                    .map_err(|e| format!("Failed to build branch: {:?}", e))?;

                // Condition
                self.builder.position_at_end(cond_bb);
                let cond_val = self.compile_expression(condition)?;
                let cond_bool = if cond_val.is_int_value() {
                    let int_val = cond_val.into_int_value();
                    self.builder
                        .build_int_compare(
                            IntPredicate::NE,
                            int_val,
                            int_val.get_type().const_zero(),
                            "whilecond",
                        )
                        .map_err(|e| format!("Failed to build comparison: {:?}", e))?
                } else {
                    return Err("Condition must be integer type".to_string());
                };

                self.builder
                    .build_conditional_branch(cond_bool, body_bb, after_bb)
                    .map_err(|e| format!("Failed to build conditional branch: {:?}", e))?;

                // Body
                self.builder.position_at_end(body_bb);
                for stmt in body {
                    self.compile_statement(stmt)?;
                }
                if self
                    .builder
                    .get_insert_block()
                    .unwrap()
                    .get_terminator()
                    .is_none()
                {
                    self.builder
                        .build_unconditional_branch(cond_bb)
                        .map_err(|e| format!("Failed to build branch: {:?}", e))?;
                }

                self.builder.position_at_end(after_bb);
                Ok(())
            }

            Statement::Expression(expr) => {
                self.compile_expression(expr)?;
                Ok(())
            }

            _ => Ok(()),
        }
    }

    fn compile_expression(&mut self, expr: ASTValue) -> Result<BasicValueEnum<'ctx>, String> {
        match expr {
            ASTValue::Int(n) => Ok(self.context.i32_type().const_int(n as u64, true).into()),

            ASTValue::Int64(n) => Ok(self.context.i64_type().const_int(n as u64, true).into()),

            ASTValue::Float32(f) => Ok(self.context.f32_type().const_float(f as f64).into()),

            ASTValue::Float64(f) => Ok(self.context.f64_type().const_float(f).into()),

            ASTValue::Bool(b) => Ok(self
                .context
                .bool_type()
                .const_int(if b { 1 } else { 0 }, false)
                .into()),

            ASTValue::VarRef(name) => {
                if let Some((var_ptr, var_type)) = self.variables.get(&name) {
                    self.builder
                        .build_load(*var_type, *var_ptr, &name)
                        .map_err(|e| format!("Failed to load variable: {:?}", e))
                } else if let Some(func) = self.current_function {
                    // Check function parameters
                    for (i, param) in func.get_params().iter().enumerate() {
                        if func
                            .get_nth_param(i as u32)
                            .map(|p| p.get_name().to_str().ok() == Some(&name))
                            .unwrap_or(false)
                        {
                            return Ok(*param);
                        }
                    }
                    Err(format!("Unknown variable: {}", name))
                } else {
                    Err(format!("Unknown variable: {}", name))
                }
            }

            ASTValue::Binary { op, left, right } => {
                let lhs = self.compile_expression(*left)?;
                let rhs = self.compile_expression(*right)?;

                if lhs.is_int_value() && rhs.is_int_value() {
                    let lhs_int = lhs.into_int_value();
                    let rhs_int = rhs.into_int_value();

                    match op {
                        BinaryOp::Add => self
                            .builder
                            .build_int_add(lhs_int, rhs_int, "add")
                            .map(|v| v.into())
                            .map_err(|e| format!("Failed to build add: {:?}", e)),
                        BinaryOp::Sub => self
                            .builder
                            .build_int_sub(lhs_int, rhs_int, "sub")
                            .map(|v| v.into())
                            .map_err(|e| format!("Failed to build sub: {:?}", e)),
                        BinaryOp::Mul => self
                            .builder
                            .build_int_mul(lhs_int, rhs_int, "mul")
                            .map(|v| v.into())
                            .map_err(|e| format!("Failed to build mul: {:?}", e)),
                        BinaryOp::Div => self
                            .builder
                            .build_int_signed_div(lhs_int, rhs_int, "div")
                            .map(|v| v.into())
                            .map_err(|e| format!("Failed to build div: {:?}", e)),
                        BinaryOp::Mod => self
                            .builder
                            .build_int_signed_rem(lhs_int, rhs_int, "mod")
                            .map(|v| v.into())
                            .map_err(|e| format!("Failed to build mod: {:?}", e)),
                        _ => Err(format!("Unsupported binary operation: {:?}", op)),
                    }
                } else {
                    Err("Binary operations require integer operands".to_string())
                }
            }

            ASTValue::Comparison { op, left, right } => {
                let lhs = self.compile_expression(*left)?;
                let rhs = self.compile_expression(*right)?;

                if lhs.is_int_value() && rhs.is_int_value() {
                    let lhs_int = lhs.into_int_value();
                    let rhs_int = rhs.into_int_value();

                    let predicate = match op {
                        ComparisonOp::Equal => IntPredicate::EQ,
                        ComparisonOp::NotEqual => IntPredicate::NE,
                        ComparisonOp::Less => IntPredicate::SLT,
                        ComparisonOp::LessEqual => IntPredicate::SLE,
                        ComparisonOp::Greater => IntPredicate::SGT,
                        ComparisonOp::GreaterEqual => IntPredicate::SGE,
                    };

                    let cmp = self
                        .builder
                        .build_int_compare(predicate, lhs_int, rhs_int, "cmp")
                        .map_err(|e| format!("Failed to build comparison: {:?}", e))?;

                    self.builder
                        .build_int_z_extend(cmp, self.context.i32_type(), "zext")
                        .map(|v| v.into())
                        .map_err(|e| format!("Failed to build zext: {:?}", e))
                } else {
                    Err("Comparison requires integer operands".to_string())
                }
            }

            ASTValue::FuncCall { name, args } => {
                let callee = self
                    .module
                    .get_function(&name)
                    .ok_or_else(|| format!("Unknown function: {}", name))?;

                let mut compiled_args = Vec::new();
                for arg in args {
                    compiled_args.push(self.compile_expression(arg)?.into());
                }

                let result = self
                    .builder
                    .build_call(callee, &compiled_args, "call")
                    .map_err(|e| format!("Failed to build call: {:?}", e))?;

                // Check if the function returns void
                if callee.get_type().get_return_type().is_none() {
                    // return dummy i32 0 for void calls
                    Ok(self.context.i32_type().const_zero().into())
                } else {
                    result
                        .try_as_basic_value()
                        .left()
                        .ok_or_else(|| "Function call did not return a value".to_string())
                }
            }

            _ => Ok(self.context.i32_type().const_zero().into()),
        }
    }
}
