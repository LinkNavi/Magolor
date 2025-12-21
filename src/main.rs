use anyhow::Result;
use std::env;
use std::fs;
use std::time::Instant;

mod modules;

use modules::tokenizer;
use modules::parser;
use modules::bytecode::{Compiler, VM};

fn main() -> Result<()> {
    let args: Vec<String> = env::args().collect();
    if args.len() < 2 {
        eprintln!("Usage: {} <source.mg> [--dump] [--bench]", args[0]);
        return Ok(());
    }

    let source = fs::read_to_string(&args[1])?;
    let dump = args.contains(&"--dump".to_string());
    let bench = args.contains(&"--bench".to_string());
    
    let t0 = Instant::now();
    
    let tokens = tokenizer::tokenize(&source);
    let t1 = Instant::now();
    
    let ast = parser::parse(&tokens).map_err(|e| anyhow::anyhow!("{}", e))?;
    let t2 = Instant::now();
    
    let module = Compiler::new().compile(ast).map_err(|e| anyhow::anyhow!("{}", e))?;
    let t3 = Instant::now();
    
    if dump {
        eprintln!("\n=== Constants ({}) ===", module.consts.len());
        for (i, c) in module.consts.iter().enumerate() { eprintln!("[{}] {:?}", i, c); }
        eprintln!("\n=== Globals ({}) ===", module.globals.len());
        for (i, g) in module.globals.iter().enumerate() { eprintln!("[{}] {}", i, g); }
        eprintln!("\n=== Functions ({}) ===", module.funcs.len());
        for f in &module.funcs { eprintln!("{}: entry={}, params={}, locals={}", f.name, f.entry, f.params, f.locals); }
        eprintln!("\n=== Code ({}) ===", module.code.len());
        for (i, op) in module.code.iter().enumerate() { eprintln!("{:04} {:?}", i, op); }
    }
    
    let mut vm = VM::new(module);
    let result = vm.run().map_err(|e| anyhow::anyhow!("{}", e))?;
    let t4 = Instant::now();
    
    if bench {
        eprintln!("\n=== Timing ===");
        eprintln!("Tokenize: {:?}", t1 - t0);
        eprintln!("Parse:    {:?}", t2 - t1);
        eprintln!("Compile:  {:?}", t3 - t2);
        eprintln!("Execute:  {:?}", t4 - t3);
        eprintln!("Total:    {:?}", t4 - t0);
    }
    
    match result {
        modules::bytecode::Val::Null => {}
        v => eprintln!("\nResult: {:?}", v),
    }
    
    Ok(())
}
