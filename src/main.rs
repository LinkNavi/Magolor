// src/main.rs - Fast Scripting Language Compiler
use anyhow::Result;
use std::env;
use std::fs;
use std::time::Instant;

mod modules;

use modules::parser;
use modules::tokenizer;
use modules::bytecode::{BytecodeCompiler, VM};

fn main() -> Result<()> {
    let args: Vec<String> = env::args().collect();

    if args.len() < 2 {
        eprintln!("Usage: {} <source file> [options]", args[0]);
        eprintln!("\nOptions:");
        eprintln!("  --dump-bytecode    Show compiled bytecode");
        eprintln!("  --show-ast         Show abstract syntax tree");
        eprintln!("  --benchmark        Show execution timing");
        return Ok(());
    }

    let filename = &args[1];
    let dump_bytecode = args.contains(&"--dump-bytecode".to_string());
    let show_ast = args.contains(&"--show-ast".to_string());
    let benchmark = args.contains(&"--benchmark".to_string());

    // Read source
    let source = fs::read_to_string(filename)?;
    
    let total_start = Instant::now();

    // === Tokenization ===
    let tok_start = Instant::now();
    let tokens = tokenizer::tokenize(&source);
    let tok_time = tok_start.elapsed();
    
    if benchmark {
        eprintln!("⏱️  Tokenization: {:?} ({} tokens)", tok_time, tokens.len());
    }

    // === Parsing ===
    let parse_start = Instant::now();
    let ast = match parser::parse_tokens(&tokens) {
        Ok(ast) => ast,
        Err(e) => {
            eprintln!("❌ Parse error: {}", e);
            std::process::exit(1);
        }
    };
    let parse_time = parse_start.elapsed();
    
    if benchmark {
        eprintln!("⏱️  Parsing: {:?}", parse_time);
    }
    
    if show_ast {
        eprintln!("\n=== Abstract Syntax Tree ===");
        eprintln!("{:#?}", ast);
    }

    // === Bytecode Compilation ===
    let compile_start = Instant::now();
    let compiler = BytecodeCompiler::new();
    let module = match compiler.compile(ast) {
        Ok(m) => m,
        Err(e) => {
            eprintln!("❌ Compilation error: {}", e);
            std::process::exit(1);
        }
    };
    let compile_time = compile_start.elapsed();
    
    if benchmark {
        eprintln!("⏱️  Compilation: {:?} ({} instructions)", 
                  compile_time, module.instructions.len());
    }
    
    if dump_bytecode {
        eprintln!("\n=== Bytecode ===");
        dump_module(&module);
    }

    // === Execution ===
    let exec_start = Instant::now();
    let mut vm = VM::new(module);
    
    // Register native functions for game engine bindings
    register_native_functions(&mut vm);
    
    match vm.run() {
        Ok(result) => {
            let exec_time = exec_start.elapsed();
            
            if benchmark {
                eprintln!("⏱️  Execution: {:?}", exec_time);
                eprintln!("\n📊 Total time: {:?}", total_start.elapsed());
            }
            
            // Only show result if it's not null
            match result {
                modules::bytecode::Value::Null => {},
                _ => eprintln!("\n✅ Result: {:?}", result),
            }
        }
        Err(e) => {
            eprintln!("❌ Runtime error: {}", e);
            std::process::exit(1);
        }
    }

    Ok(())
}

fn dump_module(module: &modules::bytecode::BytecodeModule) {
    eprintln!("\nConstants ({}):", module.constants.len());
    for (i, c) in module.constants.iter().enumerate() {
        eprintln!("  [{}] {:?}", i, c);
    }
    
    eprintln!("\nGlobals ({}):", module.global_names.len());
    for (i, name) in module.global_names.iter().enumerate() {
        eprintln!("  [{}] {}", i, name);
    }
    
    eprintln!("\nFunctions ({}):", module.functions.len());
    for (name, func) in &module.functions {
        eprintln!("  {} (entry: {}, params: {}, locals: {})", 
                  name, func.entry_point, func.param_count, func.local_count);
    }
    
    eprintln!("\nInstructions ({}):", module.instructions.len());
    for (i, instr) in module.instructions.iter().enumerate() {
        eprintln!("  {:04} {:?}", i, instr);
    }
}

fn register_native_functions(vm: &mut VM) {
    // Math functions
    vm.register_native("sqrt", |args| {
        let n = args[0].as_float()?;
        Ok(modules::bytecode::Value::Float(n.sqrt()))
    });
    
    vm.register_native("abs", |args| {
        match &args[0] {
            modules::bytecode::Value::Int(n) => Ok(modules::bytecode::Value::Int(n.abs())),
            modules::bytecode::Value::Float(f) => Ok(modules::bytecode::Value::Float(f.abs())),
            _ => Err("abs requires number".to_string()),
        }
    });
    
    vm.register_native("pow", |args| {
        let base = args[0].as_float()?;
        let exp = args[1].as_float()?;
        Ok(modules::bytecode::Value::Float(base.powf(exp)))
    });
    
    vm.register_native("floor", |args| {
        let n = args[0].as_float()?;
        Ok(modules::bytecode::Value::Int(n.floor() as i64))
    });
    
    vm.register_native("ceil", |args| {
        let n = args[0].as_float()?;
        Ok(modules::bytecode::Value::Int(n.ceil() as i64))
    });
    
    vm.register_native("round", |args| {
        let n = args[0].as_float()?;
        Ok(modules::bytecode::Value::Int(n.round() as i64))
    });
    
    vm.register_native("sin", |args| {
        let n = args[0].as_float()?;
        Ok(modules::bytecode::Value::Float(n.sin()))
    });
    
    vm.register_native("cos", |args| {
        let n = args[0].as_float()?;
        Ok(modules::bytecode::Value::Float(n.cos()))
    });
    
    // String functions
    vm.register_native("string_length", |args| {
        if let modules::bytecode::Value::String(s) = &args[0] {
            Ok(modules::bytecode::Value::Int(s.len() as i64))
        } else {
            Err("Expected string".to_string())
        }
    });
    
    // Array functions
    vm.register_native("array_length", |args| {
        if let modules::bytecode::Value::Array(arr) = &args[0] {
            Ok(modules::bytecode::Value::Int(arr.borrow().len() as i64))
        } else {
            Err("Expected array".to_string())
        }
    });
    
    // Random (you can add more game-specific functions)
    vm.register_native("random", |_args| {
        use std::collections::hash_map::RandomState;
        use std::hash::{BuildHasher, Hash, Hasher};
        let s = RandomState::new();
        let mut hasher = s.build_hasher();
        std::time::SystemTime::now().hash(&mut hasher);
        let n = hasher.finish();
        Ok(modules::bytecode::Value::Float((n % 1000000) as f64 / 1000000.0))
    });
}
