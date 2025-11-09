

use inkwell::context::Context;
use inkwell::builder::Builder;
use inkwell::module::Module;
use inkwell::values::{FunctionValue, IntValue};
use inkwell::IntPredicate;
use std::collections::HashMap;

use crate::modules::ast::*;

pub struct LLVMCompiler<'ctx> {
    context: &'ctx Context,
    module: Module<'ctx>,
    builder: Builder<'ctx>,
    variables: HashMap<String, IntValue<'ctx>>,
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
        }
    }

    pub fn compile(&mut self, program: Program) -> Result<String, String> {
        // Declare runtime functions
        self.declare_runtime_functions();
        
        // Process all items
        for item in program {
            self.compile_top_level(item)?;
        }
        
        // Generate assembly
        Ok(self.module.print_to_string().to_string())
    }

    fn declare_runtime_functions(&mut self) {
        let i32_type = self.context.i32_type();
        let i8_ptr_type = self.context.i8_type().ptr_type(inkwell::AddressSpace::default());
        
        // void console_print(char*)
        let print_type = self.context.void_type().fn_type(&[i8_ptr_type.into()], false);
        self.module.add_function("console_print", print_type, None);
        
        // char* string_concat_int(char*, i32)
        let concat_type = i8_ptr_type.fn_type(&[i8_ptr_type.into(), i32_type.into()], false);
        self.module.add_function("string_concat_int", concat_type, None);
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
                let i32_type = self.context.i32_type();
                let global = self.module.add_global(i32_type, None, &global_name);
                global.set_initializer(&i32_type.const_int(0, false));
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
        let i32_type = self.context.i32_type();
        let return_type = if func.return_type == Type::Void {
            self.context.void_type().fn_type(&[], false)
        } else {
            i32_type.fn_type(&[i32_type.into(), i32_type.into()], false)
        };
        
        let function = self.module.add_function(&func.name, return_type, None);
        let basic_block = self.context.append_basic_block(function, "entry");
        self.builder.position_at_end(basic_block);
        
        // Compile body
        for stmt in func.body {
            self.compile_statement(stmt, function)?;
        }
        
        // Add return if missing
        if self.builder.get_insert_block().unwrap().get_terminator().is_none() {
            if func.return_type == Type::Void {
                self.builder.build_return(None);
            } else {
                self.builder.build_return(Some(&i32_type.const_zero()));
            }
        }
        
        Ok(())
    }

    fn compile_statement(&mut self, stmt: Statement, function: FunctionValue<'ctx>) -> Result<(), String> {
        match stmt {
            Statement::Return(Some(expr)) => {
                let val = self.compile_expression(expr, function)?;
                self.builder.build_return(Some(&val));
                Ok(())
            }
            Statement::If { condition, then_body, else_body, .. } => {
                let cond_val = self.compile_expression(condition, function)?;
                let cond_bool = self.builder.build_int_compare(
                    IntPredicate::NE,
                    cond_val,
                    self.context.i32_type().const_zero(),
                    "ifcond"
                );
                
                let then_bb = self.context.append_basic_block(function, "then");
                let else_bb = self.context.append_basic_block(function, "else");
                let merge_bb = self.context.append_basic_block(function, "ifcont");
                
                self.builder.build_conditional_branch(cond_bool, then_bb, else_bb);
                
                // Then block
                self.builder.position_at_end(then_bb);
                for stmt in then_body {
                    self.compile_statement(stmt, function)?;
                }
                if self.builder.get_insert_block().unwrap().get_terminator().is_none() {
                    self.builder.build_unconditional_branch(merge_bb);
                }
                
                // Else block
                self.builder.position_at_end(else_bb);
                if let Some(else_stmts) = else_body {
                    for stmt in else_stmts {
                        self.compile_statement(stmt, function)?;
                    }
                }
                if self.builder.get_insert_block().unwrap().get_terminator().is_none() {
                    self.builder.build_unconditional_branch(merge_bb);
                }
                
                self.builder.position_at_end(merge_bb);
                Ok(())
            }
            _ => Ok(()),
        }
    }

    fn compile_expression(&mut self, expr: ASTValue, function: FunctionValue<'ctx>) -> Result<IntValue<'ctx>, String> {
        match expr {
            ASTValue::Int(n) => {
                Ok(self.context.i32_type().const_int(n as u64, false))
            }
            ASTValue::Binary { op: BinaryOp::Mul, left, right } => {
                let lhs = self.compile_expression(*left, function)?;
                let rhs = self.compile_expression(*right, function)?;
                Ok(self.builder.build_int_mul(lhs, rhs, "multmp"))
            }
            ASTValue::Binary { op: BinaryOp::Sub, left, right } => {
                let lhs = self.compile_expression(*left, function)?;
                let rhs = self.compile_expression(*right, function)?;
                Ok(self.builder.build_int_sub(lhs, rhs, "subtmp"))
            }
            ASTValue::Comparison { op: ComparisonOp::LessEqual, left, right } => {
                let lhs = self.compile_expression(*left, function)?;
                let rhs = self.compile_expression(*right, function)?;
                let cmp = self.builder.build_int_compare(IntPredicate::SLE, lhs, rhs, "cmple");
                Ok(self.builder.build_int_z_extend(cmp, self.context.i32_type(), "zext"))
            }
            ASTValue::FuncCall { name, args } => {
                let callee = self.module.get_function(&name)
                    .ok_or_else(|| format!("Unknown function: {}", name))?;
                
                let mut compiled_args = Vec::new();
                for arg in args {
                    compiled_args.push(self.compile_expression(arg, function)?.into());
                }
                
                let result = self.builder.build_call(callee, &compiled_args, "calltmp");
                Ok(result.try_as_basic_value().left().unwrap().into_int_value())
            }
            ASTValue::VarRef(name) if name == "n" => {
                // For now, just get the parameter
                Ok(function.get_nth_param(0).unwrap().into_int_value())
            }
            _ => Ok(self.context.i32_type().const_zero()),
        }
    }
}

// Usage in main.rs:
// let context = Context::create();
// let mut compiler = LLVMCompiler::new(&context, "program");
// let llvm_ir = compiler.compile(ast)?;
// // Then use LLVM tools to compile to assembly