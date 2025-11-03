// src/modules/IR.rs
// Main IR compilation pipeline - replaces the todo!() stub

use crate::modules::ast::Program;
use crate::modules::ir::{IRBuilder, IROptimizer, SystemPackageRegistry};
use crate::modules::ir::ir_optimizer::OptimizationLevel;
use crate::modules::ir::codegen::{CodeGenerator, Target};
use anyhow::Result;

pub fn compile_to_ir(ast: Program, optimization_level: OptimizationLevel) -> Result<String> {
    // Initialize system package registry
    let _package_registry = SystemPackageRegistry::new();
    
    // Build IR from AST
    println!("Building IR...");
    let builder = IRBuilder::new();
    let mut ir_program = builder.build(ast)
        .map_err(|e| anyhow::anyhow!("IR build error: {}", e))?;
    
    println!("Generated {} functions", ir_program.functions.len());
    
    // Optimize IR
    if optimization_level != OptimizationLevel::None {
        println!("Running optimizations at {:?} level...", optimization_level);
        let mut optimizer = IROptimizer::new(optimization_level);
        optimizer.optimize(&mut ir_program)
            .map_err(|e| anyhow::anyhow!("Optimization error: {}", e))?;
    }
    
    // Generate final code
    println!("Generating target code...");
    let mut codegen = CodeGenerator::new(Target::X86_64);
    let output = codegen.generate(&ir_program)
        .map_err(|e| anyhow::anyhow!("Code generation error: {}", e))?;
    
    Ok(output)
}

// Public API for different compilation targets
pub fn compile_to_x86_64(ast: Program, opt_level: OptimizationLevel) -> Result<String> {
    let builder = IRBuilder::new();
    let mut program = builder.build(ast)?;
    
    if opt_level != OptimizationLevel::None {
        let mut optimizer = IROptimizer::new(opt_level);
        optimizer.optimize(&mut program)?;
    }
    
    let mut codegen = CodeGenerator::new(Target::X86_64);
    Ok(codegen.generate(&program)?)
}

pub fn compile_to_llvm_ir(ast: Program, opt_level: OptimizationLevel) -> Result<String> {
    let builder = IRBuilder::new();
    let mut program = builder.build(ast)?;
    
    if opt_level != OptimizationLevel::None {
        let mut optimizer = IROptimizer::new(opt_level);
        optimizer.optimize(&mut program)?;
    }
    
    let mut codegen = CodeGenerator::new(Target::LLVM);
    Ok(codegen.generate(&program)?)
}

pub fn compile_to_wasm(ast: Program, opt_level: OptimizationLevel) -> Result<String> {
    let builder = IRBuilder::new();
    let mut program = builder.build(ast)?;
    
    if opt_level != OptimizationLevel::None {
        let mut optimizer = IROptimizer::new(opt_level);
        optimizer.optimize(&mut program)?;
    }
    
    let mut codegen = CodeGenerator::new(Target::WASM);
    Ok(codegen.generate(&program)?)
}

// Helper function to add custom system packages
pub fn add_custom_package(
    package_name: &str,
    functions: Vec<(&str, Vec<crate::modules::ir::ir_types::IRType>, crate::modules::ir::ir_types::IRType, bool)>,
) {
    let mut registry = SystemPackageRegistry::new();
    registry.add_custom_package(package_name, functions);
}
