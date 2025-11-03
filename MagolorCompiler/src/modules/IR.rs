// src/modules/IR.rs
use crate::modules::ast::Program;
use crate::modules::ir::{IRBuilder, IROptimizer, SystemPackageRegistry};
use crate::modules::ir::ir_optimizer::OptimizationLevel;
use crate::modules::ir::codegen::{CodeGenerator, Target};
use anyhow::Result;

pub fn compile_to_ir(ast: Program, optimization_level: OptimizationLevel) -> Result<String> {
    let _package_registry = SystemPackageRegistry::new();
    
    println!("Building IR...");
    let builder = IRBuilder::new();
    let mut ir_program = builder.build(ast)
        .map_err(|e| anyhow::anyhow!("IR build error: {}", e))?;
    
    println!("Generated {} functions", ir_program.functions.len());
    
    if optimization_level != OptimizationLevel::None {
        println!("Running optimizations at {:?} level...", optimization_level);
        let mut optimizer = IROptimizer::new(optimization_level);
        optimizer.optimize(&mut ir_program)
            .map_err(|e| anyhow::anyhow!("Optimization error: {}", e))?;
    }
    
    println!("Generating target code...");
    let mut codegen = CodeGenerator::new(Target::X86_64);
    let output = codegen.generate(&ir_program)
        .map_err(|e| anyhow::anyhow!("Code generation error: {}", e))?;
    
    Ok(output)
}

pub fn compile_to_x86_64(ast: Program, opt_level: OptimizationLevel) -> Result<String> {
    let builder = IRBuilder::new();
    let mut program = builder.build(ast)
        .map_err(|e| anyhow::anyhow!("IR build error: {}", e))?;
    
    if opt_level != OptimizationLevel::None {
        let mut optimizer = IROptimizer::new(opt_level);
        optimizer.optimize(&mut program)
            .map_err(|e| anyhow::anyhow!("Optimization error: {}", e))?;
    }
    
    let mut codegen = CodeGenerator::new(Target::X86_64);
    Ok(codegen.generate(&program)
        .map_err(|e| anyhow::anyhow!("Code generation error: {}", e))?)
}

pub fn compile_to_llvm_ir(ast: Program, opt_level: OptimizationLevel) -> Result<String> {
    let builder = IRBuilder::new();
    let mut program = builder.build(ast)
        .map_err(|e| anyhow::anyhow!("IR build error: {}", e))?;
    
    if opt_level != OptimizationLevel::None {
        let mut optimizer = IROptimizer::new(opt_level);
        optimizer.optimize(&mut program)
            .map_err(|e| anyhow::anyhow!("Optimization error: {}", e))?;
    }
    
    let mut codegen = CodeGenerator::new(Target::LLVM);
    Ok(codegen.generate(&program)
        .map_err(|e| anyhow::anyhow!("Code generation error: {}", e))?)
}

pub fn compile_to_wasm(ast: Program, opt_level: OptimizationLevel) -> Result<String> {
    let builder = IRBuilder::new();
    let mut program = builder.build(ast)
        .map_err(|e| anyhow::anyhow!("IR build error: {}", e))?;
    
    if opt_level != OptimizationLevel::None {
        let mut optimizer = IROptimizer::new(opt_level);
        optimizer.optimize(&mut program)
            .map_err(|e| anyhow::anyhow!("Optimization error: {}", e))?;
    }
    
    let mut codegen = CodeGenerator::new(Target::WASM);
    Ok(codegen.generate(&program)
        .map_err(|e| anyhow::anyhow!("Code generation error: {}", e))?)
}

pub fn add_custom_package(
    package_name: &str,
    functions: Vec<(&str, Vec<crate::modules::ir::ir_types::IRType>, crate::modules::ir::ir_types::IRType, bool)>,
) {
    let mut registry = SystemPackageRegistry::new();
    registry.add_custom_package(package_name, functions);
}
