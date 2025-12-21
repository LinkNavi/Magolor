// src/modules/mod.rs
// Module declarations for the fast scripting language compiler

pub mod ast;
pub mod tokenizer;
pub mod expression_parser;
pub mod statement_parser;
pub mod parser;
pub mod bytecode;  // Replaces IR module
