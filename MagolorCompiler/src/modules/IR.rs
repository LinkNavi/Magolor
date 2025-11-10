// src/modules/IR.rs - Enhanced LLVM compiler with better optimizations

use inkwell::builder::Builder;
use inkwell::context::Context;
use inkwell::module::Module;
use inkwell::types::{BasicType, BasicTypeEnum, FunctionType};
use inkwell::values::{
    BasicValue, BasicValueEnum, FunctionValue, IntValue, PointerValue, FloatValue
};
use inkwell::AddressSpace;
use inkwell::IntPredicate;
use inkwell::FloatPredicate;
use inkwell::OptimizationLevel;
use std::collections::HashMap;

use crate::modules::ast::*;

pub struct LLVMCompiler<'ctx> {
    context: &'ctx Context,
    module: Module<'ctx>,
    builder: Builder<'ctx>,
    
    // Symbol tables
    variables: HashMap<String, (PointerValue<'ctx>, Type)>,
    functions: HashMap<String, FunctionValue<'ctx>>,
    
    // Current state
    current_function: Option<FunctionValue<'ctx>>,
    loop_stack: Vec<LoopContext<'ctx>>,
    
    // Optimization level
    opt_level: OptimizationLevel,
}

struct LoopContext<'ctx> {
    continue_block: inkwell::basic_block::BasicBlock<'ctx>,
    break_block: inkwell::basic_block::BasicBlock<'ctx>,
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
            functions: HashMap::new(),
            current_function: None,
            loop_stack: Vec::new(),
            opt_level: OptimizationLevel::Aggressive,
        }
    }
    
    pub fn set_optimization_level(&mut self, level: OptimizationLevel) {
        self.opt_level = level;
    }

    pub fn compile(&mut self, program: Program) -> Result<String, String> {
        // Declare runtime functions
        self.declare_runtime_functions()?;

        // First pass: collect function signatures
        for item in &program {
            self.collect_function_signatures(item)?;
        }

        // Second pass: compile everything
        for item in program {
            self.compile_top_level(item)?;
        }

        // Verify module
        if let Err(e) = self.module.verify() {
            return Err(format!("Module verification failed: {}", e.to_string()));
        }

        // Generate LLVM IR
        Ok(self.module.print_to_string().to_string())
    }

    fn declare_runtime_functions(&mut self) -> Result<(), String> {
        let i8_type = self.context.i8_type();
        let i32_type = self.context.i32_type();
        let i64_type = self.context.i64_type();
        let u32_type = self.context.i32_type(); // LLVM doesn't distinguish signedness in types
        let u64_type = self.context.i64_type();
        let f32_type = self.context.f32_type();
        let f64_type = self.context.f64_type();
        let bool_type = self.context.bool_type();
        let void_type = self.context.void_type();
        let ptr_type = self.context.ptr_type(AddressSpace::default());

        // Standard C functions
        self.declare_function("printf", i32_type.fn_type(&[ptr_type.into()], true))?;
        self.declare_function("malloc", ptr_type.fn_type(&[i64_type.into()], false))?;
        self.declare_function("calloc", ptr_type.fn_type(&[i64_type.into(), i64_type.into()], false))?;
        self.declare_function("realloc", ptr_type.fn_type(&[ptr_type.into(), i64_type.into()], false))?;
        self.declare_function("free", void_type.fn_type(&[ptr_type.into()], false))?;
        
        // Print functions
        self.declare_function("print_int", void_type.fn_type(&[i32_type.into()], false))?;
        self.declare_function("print_i64", void_type.fn_type(&[i64_type.into()], false))?;
        self.declare_function("print_u32", void_type.fn_type(&[u32_type.into()], false))?;
        self.declare_function("print_u64", void_type.fn_type(&[u64_type.into()], false))?;
        self.declare_function("print_f32", void_type.fn_type(&[f32_type.into()], false))?;
        self.declare_function("print_f64", void_type.fn_type(&[f64_type.into()], false))?;
        self.declare_function("print_str", void_type.fn_type(&[ptr_type.into()], false))?;
        self.declare_function("print_char", void_type.fn_type(&[i8_type.into()], false))?;
        self.declare_function("print_bool", void_type.fn_type(&[bool_type.into()], false))?;
        
        // String functions
        self.declare_function("string_length", i32_type.fn_type(&[ptr_type.into()], false))?;
        self.declare_function("string_concat_cstr", ptr_type.fn_type(&[ptr_type.into(), ptr_type.into()], false))?;
        self.declare_function("string_substring", ptr_type.fn_type(&[ptr_type.into(), i32_type.into(), i32_type.into()], false))?;
        self.declare_function("string_indexof", i32_type.fn_type(&[ptr_type.into(), ptr_type.into()], false))?;
        self.declare_function("string_contains", bool_type.fn_type(&[ptr_type.into(), ptr_type.into()], false))?;
        self.declare_function("string_equals", bool_type.fn_type(&[ptr_type.into(), ptr_type.into()], false))?;
        
        // Math functions
        self.declare_function("math_abs_i32", i32_type.fn_type(&[i32_type.into()], false))?;
        self.declare_function("math_abs_f32", f32_type.fn_type(&[f32_type.into()], false))?;
        self.declare_function("math_abs_f64", f64_type.fn_type(&[f64_type.into()], false))?;
        self.declare_function("math_sqrt", f64_type.fn_type(&[f64_type.into()], false))?;
        self.declare_function("math_pow", f64_type.fn_type(&[f64_type.into(), f64_type.into()], false))?;
        self.declare_function("math_sin", f64_type.fn_type(&[f64_type.into()], false))?;
        self.declare_function("math_cos", f64_type.fn_type(&[f64_type.into()], false))?;
        self.declare_function("math_floor", f64_type.fn_type(&[f64_type.into()], false))?;
        self.declare_function("math_ceil", f64_type.fn_type(&[f64_type.into()], false))?;
        
        // Memory functions
        self.declare_function("magolor_alloc", ptr_type.fn_type(&[i64_type.into()], false))?;
        self.declare_function("magolor_free", void_type.fn_type(&[ptr_type.into()], false))?;
        
        Ok(())
    }

    fn declare_function(&mut self, name: &str, fn_type: FunctionType<'ctx>) -> Result<(), String> {
        let function = self.module.add_function(name, fn_type, None);
        self.functions.insert(name.to_string(), function);
        Ok(())
    }

    fn collect_function_signatures(&mut self, item: &TopLevel) -> Result<(), String> {
        match item {
            TopLevel::Function(func) => {
                let param_types: Vec<BasicTypeEnum> = func
                    .params
                    .iter()
                    .map(|p| self.llvm_type(&p.param_type))
                    .collect::<Result<Vec<_>, _>>()?;

                let param_types_ref: Vec<_> = param_types.iter().map(|t| (*t).into()).collect();

                let fn_type = if func.return_type == Type::Void {
                    self.context.void_type().fn_type(&param_types_ref, false)
                } else {
                    let ret_type = self.llvm_type(&func.return_type)?;
                    ret_type.fn_type(&param_types_ref, false)
                };

                let function = self.module.add_function(&func.name, fn_type, None);
                self.functions.insert(func.name.clone(), function);
                Ok(())
            }
            TopLevel::Namespace { items, .. } => {
                for item in items {
                    self.collect_function_signatures(item)?;
                }
                Ok(())
            }
            _ => Ok(()),
        }
    }

    fn llvm_type(&self, ty: &Type) -> Result<BasicTypeEnum<'ctx>, String> {
        match ty {
            Type::I8 => Ok(self.context.i8_type().into()),
            Type::I16 => Ok(self.context.i16_type().into()),
            Type::I32 => Ok(self.context.i32_type().into()),
            Type::I64 => Ok(self.context.i64_type().into()),
            Type::U8 => Ok(self.context.i8_type().into()),
            Type::U16 => Ok(self.context.i16_type().into()),
            Type::U32 => Ok(self.context.i32_type().into()),
            Type::U64 => Ok(self.context.i64_type().into()),
            Type::F32 => Ok(self.context.f32_type().into()),
            Type::F64 => Ok(self.context.f64_type().into()),
            Type::Char => Ok(self.context.i8_type().into()),
            Type::Bool => Ok(self.context.bool_type().into()),
            Type::String => Ok(self.context.ptr_type(AddressSpace::default()).into()),
            Type::Ptr(_) | Type::Ref(_, _) => Ok(self.context.ptr_type(AddressSpace::default()).into()),
            Type::Array(inner, size) => {
                let elem_type = self.llvm_type(inner)?;
                if let Some(s) = size {
                    Ok(elem_type.array_type(*s as u32).into())
                } else {
                    // Dynamic array as pointer
                    Ok(self.context.ptr_type(AddressSpace::default()).into())
                }
            }
            Type::Tuple(types) => {
                let fields: Vec<BasicTypeEnum> = types
                    .iter()
                    .map(|t| self.llvm_type(t))
                    .collect::<Result<Vec<_>, _>>()?;
                Ok(self.context.struct_type(&fields, false).into())
            }
            _ => Ok(self.context.i32_type().into()), // Default to i32
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
            TopLevel::Constant { name, const_type, value, .. } => {
                let llvm_type = self.llvm_type(&const_type)?;
                let global = self.module.add_global(llvm_type, None, &name);
                // Set constant value here
                global.set_constant(true);
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

                // Initialize with default value
                if let BasicTypeEnum::IntType(int_type) = field_type {
                    global.set_initializer(&int_type.const_zero());
                } else if let BasicTypeEnum::FloatType(float_type) = field_type {
                    global.set_initializer(&float_type.const_zero());
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

    fn compile_function(&mut self, func: FunctionDef) -> Result<(), String> {
        let function = *self.functions.get(&func.name)
            .ok_or_else(|| format!("Function not found: {}", func.name))?;
        
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
                    .insert(param.name.clone(), (alloca, param.param_type.clone()));
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
                        let zero_val = self.get_zero_value(ret_type)?;
                        self.builder
                            .build_return(Some(&zero_val))
                            .map_err(|e| format!("Failed to build return: {:?}", e))?;
                    }
                }
            }
        }

        Ok(())
    }

    fn get_zero_value(&self, ty: BasicTypeEnum<'ctx>) -> Result<BasicValueEnum<'ctx>, String> {
        match ty {
            BasicTypeEnum::IntType(int_type) => Ok(int_type.const_zero().into()),
            BasicTypeEnum::FloatType(float_type) => Ok(float_type.const_zero().into()),
            BasicTypeEnum::PointerType(ptr_type) => Ok(ptr_type.const_null().into()),
            _ => Err("Cannot generate zero value for this type".to_string()),
        }
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
                } else {
                    // Initialize with zero
                    let zero = self.get_zero_value(llvm_type)?;
                    self.builder
                        .build_store(alloca, zero)
                        .map_err(|e| format!("Failed to store: {:?}", e))?;
                }

                self.variables.insert(name, (alloca, var_type));
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

            Statement::Break => {
                if let Some(loop_ctx) = self.loop_stack.last() {
                    self.builder
                        .build_unconditional_branch(loop_ctx.break_block)
                        .map_err(|e| format!("Failed to build break: {:?}", e))?;
                    Ok(())
                } else {
                    Err("Break statement outside loop".to_string())
                }
            }

            Statement::Continue => {
                if let Some(loop_ctx) = self.loop_stack.last() {
                    self.builder
                        .build_unconditional_branch(loop_ctx.continue_block)
                        .map_err(|e| format!("Failed to build continue: {:?}", e))?;
                    Ok(())
                } else {
                    Err("Continue statement outside loop".to_string())
                }
            }

            Statement::If {
                condition,
                then_body,
                elif_branches,
                else_body,
            } => {
                self.compile_if_statement(condition, then_body, elif_branches, else_body)
            }

            Statement::While { condition, body } => {
                self.compile_while_statement(condition, body)
            }

            Statement::Expression(expr) => {
                self.compile_expression(expr)?;
                Ok(())
            }

            _ => Ok(()),
        }
    }

    fn compile_if_statement(
        &mut self,
        condition: ASTValue,
        then_body: Vec<Statement>,
        _elif_branches: Vec<(ASTValue, Vec<Statement>)>,
        else_body: Option<Vec<Statement>>,
    ) -> Result<(), String> {
        let function = self.current_function
            .ok_or("If statement outside function")?;

        let cond_val = self.compile_expression(condition)?;
        let cond_bool = self.value_to_bool(cond_val)?;

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

    fn compile_while_statement(
        &mut self,
        condition: ASTValue,
        body: Vec<Statement>,
    ) -> Result<(), String> {
        let function = self.current_function
            .ok_or("While loop outside function")?;

        let cond_bb = self.context.append_basic_block(function, "whilecond");
        let body_bb = self.context.append_basic_block(function, "whilebody");
        let after_bb = self.context.append_basic_block(function, "afterwhile");

        // Push loop context
        self.loop_stack.push(LoopContext {
            continue_block: cond_bb,
            break_block: after_bb,
        });

        self.builder
            .build_unconditional_branch(cond_bb)
            .map_err(|e| format!("Failed to build branch: {:?}", e))?;

        // Condition
        self.builder.position_at_end(cond_bb);
        let cond_val = self.compile_expression(condition)?;
        let cond_bool = self.value_to_bool(cond_val)?;

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

        self.loop_stack.pop();

        self.builder.position_at_end(after_bb);
        Ok(())
    }

    fn value_to_bool(&mut self, val: BasicValueEnum<'ctx>) -> Result<IntValue<'ctx>, String> {
        if val.is_int_value() {
            let int_val = val.into_int_value();
            self.builder
                .build_int_compare(
                    IntPredicate::NE,
                    int_val,
                    int_val.get_type().const_zero(),
                    "tobool",
                )
                .map_err(|e| format!("Failed to build comparison: {:?}", e))
        } else {
            Err("Cannot convert non-integer value to bool".to_string())
        }
    }

    fn compile_expression(&mut self, expr: ASTValue) -> Result<BasicValueEnum<'ctx>, String> {
        match expr {
            ASTValue::Int(n) => Ok(self.context.i64_type().const_int(n as u64, true).into()),
            ASTValue::UInt(n) => Ok(self.context.i64_type().const_int(n, false).into()),
            ASTValue::Float32(f) => Ok(self.context.f32_type().const_float(f as f64).into()),
            ASTValue::Float64(f) => Ok(self.context.f64_type().const_float(f).into()),
            ASTValue::Bool(b) => Ok(self
                .context
                .bool_type()
                .const_int(if b { 1 } else { 0 }, false)
                .into()),

            ASTValue::Str(s) => {
                // Create a global string constant
                let global_str = self.builder
                    .build_global_string_ptr(&s, "str")
                    .map_err(|e| format!("Failed to create string: {:?}", e))?;
                Ok(global_str.as_basic_value_enum())
            }

            ASTValue::VarRef(name) => {
                if let Some((var_ptr, var_type)) = self.variables.get(&name).cloned() {
                    let llvm_type = self.llvm_type(&var_type)?;
                    self.builder
                        .build_load(llvm_type, var_ptr, &name)
                        .map_err(|e| format!("Failed to load variable: {:?}", e))
                } else {
                    Err(format!("Unknown variable: {}", name))
                }
            }

            ASTValue::Binary { op, left, right } => {
                self.compile_binary_op(op, *left, *right)
            }

            ASTValue::Comparison { op, left, right } => {
                self.compile_comparison(op, *left, *right)
            }

            ASTValue::FuncCall { name, args, .. } => {
                let callee = self.functions.get(&name)
                    .ok_or_else(|| format!("Unknown function: {}", name))?;

                let mut compiled_args = Vec::new();
                for arg in args {
                    compiled_args.push(self.compile_expression(arg)?.into());
                }

                let result = self
                    .builder
                    .build_call(*callee, &compiled_args, "call")
                    .map_err(|e| format!("Failed to build call: {:?}", e))?;

                if callee.get_type().get_return_type().is_none() {
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

    fn compile_binary_op(
        &mut self,
        op: BinaryOp,
        left: ASTValue,
        right: ASTValue,
    ) -> Result<BasicValueEnum<'ctx>, String> {
        let lhs = self.compile_expression(left)?;
        let rhs = self.compile_expression(right)?;

        if lhs.is_int_value() && rhs.is_int_value() {
            let lhs_int = lhs.into_int_value();
            let rhs_int = rhs.into_int_value();

            match op {
                BinaryOp::Add => self.builder
                    .build_int_add(lhs_int, rhs_int, "add")
                    .map(|v| v.into())
                    .map_err(|e| format!("Failed to build add: {:?}", e)),
                BinaryOp::Sub => self.builder
                    .build_int_sub(lhs_int, rhs_int, "sub")
                    .map(|v| v.into())
                    .map_err(|e| format!("Failed to build sub: {:?}", e)),
                BinaryOp::Mul => self.builder
                    .build_int_mul(lhs_int, rhs_int, "mul")
                    .map(|v| v.into())
                    .map_err(|e| format!("Failed to build mul: {:?}", e)),
                BinaryOp::Div => self.builder
                    .build_int_signed_div(lhs_int, rhs_int, "div")
                    .map(|v| v.into())
                    .map_err(|e| format!("Failed to build div: {:?}", e)),
                BinaryOp::Mod => self.builder
                    .build_int_signed_rem(lhs_int, rhs_int, "mod")
                    .map(|v| v.into())
                    .map_err(|e| format!("Failed to build mod: {:?}", e)),
                _ => Err(format!("Unsupported binary operation: {:?}", op)),
            }
        } else if lhs.is_float_value() && rhs.is_float_value() {
            let lhs_float = lhs.into_float_value();
            let rhs_float = rhs.into_float_value();

            match op {
                BinaryOp::Add => self.builder
                    .build_float_add(lhs_float, rhs_float, "fadd")
                    .map(|v| v.into())
                    .map_err(|e| format!("Failed to build fadd: {:?}", e)),
                BinaryOp::Sub => self.builder
                    .build_float_sub(lhs_float, rhs_float, "fsub")
                    .map(|v| v.into())
                    .map_err(|e| format!("Failed to build fsub: {:?}", e)),
                BinaryOp::Mul => self.builder
                    .build_float_mul(lhs_float, rhs_float, "fmul")
                    .map(|v| v.into())
                    .map_err(|e| format!("Failed to build fmul: {:?}", e)),
                BinaryOp::Div => self.builder
                    .build_float_div(lhs_float, rhs_float, "fdiv")
                    .map(|v| v.into())
                    .map_err(|e| format!("Failed to build fdiv: {:?}", e)),
                _ => Err(format!("Unsupported float operation: {:?}", op)),
            }
        } else {
            Err("Type mismatch in binary operation".to_string())
        }
    }

    fn compile_comparison(
        &mut self,
        op: ComparisonOp,
        left: ASTValue,
        right: ASTValue,
    ) -> Result<BasicValueEnum<'ctx>, String> {
        let lhs = self.compile_expression(left)?;
        let rhs = self.compile_expression(right)?;

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
            Err("Comparison requires compatible operand types".to_string())
        }
    }
}
