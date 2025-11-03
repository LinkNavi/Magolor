// src/main.rs
use anyhow::Result;
use std::env;
use std::fs;

mod modules;

use modules::IR;
use modules::parser;
use modules::tokenizer;

use crate::modules::IR::compile_to_ir;
use crate::modules::ir::ir_optimizer::OptimizationLevel;
fn main() -> Result<()> {
    let args: Vec<String> = env::args().collect();

    if args.len() < 2 {
        eprintln!("Usage: {} <source file>", args[0]);
        return Ok(());
    }

    let filename = &args[1];
    println!("Compiling: {}", filename);

    // Read source file
    let source = fs::read_to_string(filename)?;

    // Tokenize
    println!("\n=== Tokenization ===");
    let tokens = tokenizer::tokenize(&source);
    println!("Generated {} tokens", tokens.len());

    // Parse
    println!("\n=== Parsing ===");

    // Parse
    println!("\n=== Parsing ===");
    match parser::parse_tokens(&tokens) {
        Ok(ast) => {
            println!("Successfully parsed!");
            println!("\n=== AST ===");
            println!("{:#?}", ast);

            // === IR Generation ===
            println!("\n=== IR Generation ===");
            let output = compile_to_ir(ast, OptimizationLevel::Aggressive)?;
            println!("{:#?}", output);
        }
        Err(e) => {
            eprintln!("Parse error: {}", e);
            std::process::exit(1);
        }
    }

    Ok(())
}
