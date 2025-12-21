// src/modules/mod.rs
// Module declarations for the fast scripting language compiler

pub mod ast;
pub mod tokenizer;
pub mod ffi;

pub mod parser;
pub mod bytecode;  // Replaces IR module
