// src/main.rs - Complete implementation with LLVM
use anyhow::Result;
use std::env;
use std::fs;

mod modules;

use modules::parser;
use modules::tokenizer;
use modules::IR::LLVMCompiler;

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
        filename.replace(".mg", ".ll")
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

            eprintln!("\n=== LLVM IR Generation ===");
            
            // Create LLVM context and compiler
            let context = inkwell::context::Context::create();
            let mut compiler = LLVMCompiler::new(&context, "magolor_program");
            
            // Compile to LLVM IR
            let llvm_ir = compiler.compile(ast)
                .map_err(|e| anyhow::anyhow!("LLVM compilation error: {}", e))?;
            
            // Write LLVM IR to file
            fs::write(&output_file, &llvm_ir)?;
            eprintln!("✅ LLVM IR written to: {}", output_file);
            
            // Optionally compile to assembly using llc
            if env::var("COMPILE_TO_ASM").is_ok() {
                let asm_file = output_file.replace(".ll", ".s");
                eprintln!("\n=== Compiling to Assembly ===");
                let status = std::process::Command::new("llc")
                    .arg(&output_file)
                    .arg("-o")
                    .arg(&asm_file)
                    .status();
                
                match status {
                    Ok(s) if s.success() => {
                        eprintln!("✅ Assembly written to: {}", asm_file);
                    }
                    _ => {
                        eprintln!("⚠️  llc not found or failed. Install LLVM tools to generate assembly.");
                    }
                }
            }
            
            if env::var("SHOW_IR").is_ok() {
                println!("\n=== LLVM IR ===");
                print!("{}", llvm_ir);
            }
        }
        Err(e) => {
            eprintln!("Parse error: {}", e);
            std::process::exit(1);
        }
    }

    Ok(())
}
