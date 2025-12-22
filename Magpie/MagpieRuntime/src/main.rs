use anyhow::Result;
use std::{env, fs, io::{self, Write}, time::Instant};

mod modules;
use modules::{tokenizer, parser, bytecode::{Compiler, VM, Val}};

fn main() -> Result<()> {
    let args: Vec<String> = env::args().collect();
    
    if args.len() < 2 {
        // REPL mode
        repl()?;
    } else {
        // Execute file
        run_file(&args[1], args.contains(&"--bench".to_string()))?;
    }
    
    Ok(())
}

fn repl() -> Result<()> {
    println!("Magolor v0.1.0 - Interactive REPL");
    println!("Type .exit to quit, .help for commands\n");
    
    let mut history = Vec::new();
    let mut line_num = 1;
    
    loop {
        print!("{}> ", line_num);
        io::stdout().flush()?;
        
        let mut input = String::new();
        io::stdin().read_line(&mut input)?;
        
        let trimmed = input.trim();
        
        if trimmed.is_empty() {
            continue;
        }
        
        // REPL commands
        match trimmed {
            ".exit" => break,
            ".quit" => break,
            ".clear" => {
                history.clear();
                line_num = 1;
                print!("\x1B[2J\x1B[1;1H"); // Clear screen
                continue;
            }
            ".history" => {
                for (i, line) in history.iter().enumerate() {
                    println!("[{}] {}", i + 1, line);
                }
                continue;
            }
            ".help" => {
                println!("Commands:");
                println!("  .exit      Exit REPL");
                println!("  .clear     Clear history");
                println!("  .history   Show command history");
                println!("  .run <file> Execute a file");
                println!("  .help      Show this help");
                continue;
            }
            _ if trimmed.starts_with(".run ") => {
                let file = trimmed.trim_start_matches(".run ").trim();
                if let Err(e) = run_file(file, false) {
                    eprintln!("Error: {}", e);
                }
                continue;
            }
            _ => {}
        }
        
        history.push(input.clone());
        
        // Try to execute as expression first
        if let Some(result) = eval_expr(&input) {
            match result {
                Val::Null => {}
                Val::Int(n) => println!("{}", n),
                Val::Float(f) => println!("{}", f),
                Val::Bool(b) => println!("{}", b),
                Val::Str(s) => println!("{}", s),
                v => println!("{:?}", v),
            }
        } else {
            // Try as statement
            if let Err(e) = eval_stmt(&input) {
                eprintln!("Error: {}", e);
            }
        }
        
        line_num += 1;
    }
    
    println!("Goodbye!");
    Ok(())
}

fn eval_expr(input: &str) -> Option<Val> {
    // Wrap in a main function that returns the expression
    let code = format!("i64 fn __repl_eval() {{ return {}; }}", input.trim_end_matches(';'));
    
    let tokens = tokenizer::tokenize(&code);
    let ast = parser::parse(&tokens).ok()?;
    let module = Compiler::new().compile(ast).ok()?;
    let mut vm = VM::new(module);
    vm.run().ok()
}

fn eval_stmt(input: &str) -> Result<()> {
    // Wrap in a main function
    let code = format!("void fn __repl_exec() {{ {} }}", input);
    
    let tokens = tokenizer::tokenize(&code);
    let ast = parser::parse(&tokens).map_err(|e| anyhow::anyhow!("{}", e))?;
    let module = Compiler::new().compile(ast).map_err(|e| anyhow::anyhow!("{}", e))?;
    let mut vm = VM::new(module);
    vm.run().map_err(|e| anyhow::anyhow!("{}", e))?;
    
    Ok(())
}

fn run_file(path: &str, bench: bool) -> Result<()> {
    let t0 = Instant::now();
    let src = fs::read_to_string(path)?;
    let t1 = Instant::now();
    
    let tokens = tokenizer::tokenize(&src);
    let t2 = Instant::now();
    
    let ast = parser::parse(&tokens).map_err(|e| anyhow::anyhow!("{}", e))?;
    let t3 = Instant::now();
    
    let module = Compiler::new().compile(ast).map_err(|e| anyhow::anyhow!("{}", e))?;
    let t4 = Instant::now();
    
    let mut vm = VM::new(module);
    let result = vm.run().map_err(|e| anyhow::anyhow!("{}", e))?;
    let t5 = Instant::now();
    
    if bench {
        eprintln!("\n=== Timing ===");
        eprintln!("Read:    {:?}", t1 - t0);
        eprintln!("Lex:     {:?}", t2 - t1);
        eprintln!("Parse:   {:?}", t3 - t2);
        eprintln!("Compile: {:?}", t4 - t3);
        eprintln!("Execute: {:?}", t5 - t4);
        eprintln!("Total:   {:?}", t5 - t0);
    }
    
    match result {
        Val::Null => {}
        v => eprintln!("=> {:?}", v),
    }
    
    Ok(())
}
