// src/main.rs - Fixed to output assembly cleanly
use anyhow::Result;
use std::env;
use std::fs;

mod modules;

use modules::parser;
use modules::tokenizer;

use crate::modules::IR::compile_to_ir;
use crate::modules::ir::ir_optimizer::OptimizationLevel;

fn main() -> Result<()> {
    let args: Vec<String> = env::args().collect();

    if args.len() < 2 {
        eprintln!("Usage: {} <source file> [-o output.s]", args[0]);
        return Ok(());
    }

    let filename = &args[1];
    
    // Check for output file flag
    let output_file = if args.len() >= 4 && args[2] == "-o" {
        args[3].clone()
    } else {
        filename.replace(".mg", ".s")
    };

    eprintln!("Compiling: {}", filename);

    // Read source file
    let source = fs::read_to_string(filename)?;

    // Tokenize
    eprintln!("\n=== Tokenization ===");
    let tokens = tokenizer::tokenize(&source);
    eprintln!("Generated {} tokens", tokens.len());

    // Parse
    eprintln!("\n=== Parsing ===");
    match parser::parse_tokens(&tokens) {
        Ok(ast) => {
            eprintln!("Successfully parsed!");
            
            // Optionally show AST (comment out for clean output)
            if env::var("SHOW_AST").is_ok() {
                eprintln!("\n=== AST ===");
                eprintln!("{:#?}", ast);
            }

            // === IR Generation ===
            eprintln!("\n=== IR Generation ===");
            let output = compile_to_ir(ast, OptimizationLevel::Aggressive)?;
            
            // Write assembly to file
            fs::write(&output_file, &output)?;
            eprintln!("✅ Assembly written to: {}", output_file);
            
            // Print ONLY the assembly to stdout (for piping)
            print!("{}", output);
        }
        Err(e) => {
            eprintln!("Parse error: {}", e);
            std::process::exit(1);
        }
    }

    Ok(())
}