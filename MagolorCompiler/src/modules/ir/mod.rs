// src/modules/ir/mod.rs
// IR module structure - Core intermediate representation

pub mod ir_types;
pub mod ir_builder;
pub mod ir_optimizer;
pub mod control_flow;
pub mod ssa_transform;
pub mod register_allocator;
pub mod constant_folding;
pub mod dead_code_elimination;
pub mod inline_optimizer;
pub mod loop_optimizer;
pub mod peephole;
pub mod codegen;
pub mod system_packages;

pub use ir_types::{IRProgram, IRFunction, IRBasicBlock, IRInstruction, IRValue, IRType};
pub use ir_builder::IRBuilder;
pub use ir_optimizer::IROptimizer;
pub use system_packages::SystemPackageRegistry;
