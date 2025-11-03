// src/main.rs
use std::env;
use std::fs;
use anyhow::Result;

mod modules;

use modules::tokenizer;
use modules::parser;

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
    match parser::parse_tokens(&tokens) {
        Ok(ast) => {
            println!("Successfully parsed!");
            println!("\n=== AST ===");
            println!("{:#?}", ast);

            // TODO: Add IR generation
            println!("\n=== Code Generation ===");
            println!("Code generation not yet implemented");
        }
        Err(e) => {
            eprintln!("Parse error: {}", e);
            std::process::exit(1);
        }
    }

    Ok(())
}
