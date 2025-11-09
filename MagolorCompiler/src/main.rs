// src/main.rs - Fixed imports and structure
use anyhow::Result;
use std::env;
use std::fs;

mod modules;

use modules::parser;
use modules::tokenizer;
use modules::ir::ir_optimizer::OptimizationLevel;
use modules::ir::ir_builder::IRBuilder;
use modules::ir::codegen::{CodeGenerator, Target};
use modules::ir::ir_optimizer::IROptimizer;

fn main() -> Result<()> {
    let args: Vec<String> = env::args().collect();

    if args.len() < 2 {
        eprintln!("Usage: {} <source file> [-o output.s]", args[0]);
        return Ok(());
    }

    let filename = &args[1];
    
    let output_file = if args.len() >= 4 && args[2] == "-o" {
        args[3].clone()
    } else {
        filename.replace(".mg", ".s")
    };

    eprintln!("Compiling: {}", filename);

    let source = fs::read_to_string(filename)?;

    eprintln!("\n=== Tokenization ===");
    let tokens = tokenizer::tokenize(&source);
    eprintln!("Generated {} tokens", tokens.len());

    eprintln!("\n=== Parsing ===");
    match parser::parse_tokens(&tokens) {
        Ok(ast) => {
            eprintln!("Successfully parsed!");
            
            if env::var("SHOW_AST").is_ok() {
                eprintln!("\n=== AST ===");
                eprintln!("{:#?}", ast);
            }

            eprintln!("\n=== IR Generation ===");
            
            // Build IR
            let builder = IRBuilder::new();
            let mut ir_program = builder.build(ast)
                .map_err(|e| anyhow::anyhow!("IR build error: {}", e))?;
            
            eprintln!("Generated {} functions", ir_program.functions.len());
            
            // Optimize
            let mut optimizer = IROptimizer::new(OptimizationLevel::Aggressive);
            optimizer.optimize(&mut ir_program)
                .map_err(|e| anyhow::anyhow!("Optimization error: {}", e))?;
            
            // Generate code
            let mut codegen = CodeGenerator::new(Target::X86_64);
            let output = codegen.generate(&ir_program)
                .map_err(|e| anyhow::anyhow!("Code generation error: {}", e))?;
            
            fs::write(&output_file, &output)?;
            eprintln!("✅ Assembly written to: {}", output_file);
            
            print!("{}", output);
        }
        Err(e) => {
            eprintln!("Parse error: {}", e);
            std::process::exit(1);
        }
    }

    Ok(())
}