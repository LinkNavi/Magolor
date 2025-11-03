// src/modules/ir/ir_optimizer.rs
// Main optimization pipeline

use crate::modules::ir::ir_types::*;
use crate::modules::ir::constant_folding::ConstantFolder;
use crate::modules::ir::dead_code_elimination::DeadCodeEliminator;
use crate::modules::ir::inline_optimizer::InlineOptimizer;
use crate::modules::ir::loop_optimizer::LoopOptimizer;
use crate::modules::ir::peephole::PeepholeOptimizer;
use crate::modules::ir::ssa_transform::SSATransform;

pub struct IROptimizer {
    optimization_level: OptimizationLevel,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum OptimizationLevel {
    None,       // O0
    Basic,      // O1
    Aggressive, // O2
    Maximum,    // O3
}

impl IROptimizer {
    pub fn new(level: OptimizationLevel) -> Self {
        Self {
            optimization_level: level,
        }
    }

    pub fn optimize(&mut self, program: &mut IRProgram) -> Result<(), String> {
        if self.optimization_level == OptimizationLevel::None {
            return Ok(());
        }

        // Run optimization passes in order
        self.run_passes(program)?;

        Ok(())
    }

    fn run_passes(&mut self, program: &mut IRProgram) -> Result<(), String> {
        let passes = self.get_pass_pipeline();

        for _ in 0..self.get_iteration_count() {
            let mut changed = false;

            for pass in &passes {
                changed |= self.run_pass(program, pass)?;
            }

            if !changed {
                break; // Converged
            }
        }

        Ok(())
    }

    fn get_pass_pipeline(&self) -> Vec<OptPass> {
        match self.optimization_level {
            OptimizationLevel::None => vec![],
            OptimizationLevel::Basic => vec![
                OptPass::ConstantFolding,
                OptPass::DeadCodeElimination,
            ],
            OptimizationLevel::Aggressive => vec![
                OptPass::ConstantFolding,
                OptPass::DeadCodeElimination,
                OptPass::SSATransform,
                OptPass::Inlining,
                OptPass::LoopOptimization,
                OptPass::Peephole,
            ],
            OptimizationLevel::Maximum => vec![
                OptPass::ConstantFolding,
                OptPass::SSATransform,
                OptPass::Inlining,
                OptPass::LoopOptimization,
                OptPass::DeadCodeElimination,
                OptPass::Peephole,
                OptPass::ConstantPropagation,
                OptPass::CommonSubexpressionElimination,
            ],
        }
    }

    fn get_iteration_count(&self) -> usize {
        match self.optimization_level {
            OptimizationLevel::None => 0,
            OptimizationLevel::Basic => 1,
            OptimizationLevel::Aggressive => 3,
            OptimizationLevel::Maximum => 5,
        }
    }

    fn run_pass(&mut self, program: &mut IRProgram, pass: &OptPass) -> Result<bool, String> {
        match pass {
            OptPass::ConstantFolding => {
                let mut folder = ConstantFolder::new();
                Ok(folder.run(program))
            }
            OptPass::DeadCodeElimination => {
                let mut eliminator = DeadCodeEliminator::new();
                Ok(eliminator.run(program))
            }
            OptPass::SSATransform => {
                let mut ssa = SSATransform::new();
                Ok(ssa.run(program))
            }
            OptPass::Inlining => {
                let mut inliner = InlineOptimizer::new();
                Ok(inliner.run(program))
            }
            OptPass::LoopOptimization => {
                let mut loop_opt = LoopOptimizer::new();
                Ok(loop_opt.run(program))
            }
            OptPass::Peephole => {
                let mut peephole = PeepholeOptimizer::new();
                Ok(peephole.run(program))
            }
            OptPass::ConstantPropagation => {
                // TODO: Implement
                Ok(false)
            }
            OptPass::CommonSubexpressionElimination => {
                // TODO: Implement
                Ok(false)
            }
        }
    }
}

#[derive(Debug, Clone, Copy)]
enum OptPass {
    ConstantFolding,
    DeadCodeElimination,
    SSATransform,
    Inlining,
    LoopOptimization,
    Peephole,
    ConstantPropagation,
    CommonSubexpressionElimination,
}

// Function-level analysis utilities
pub struct FunctionAnalysis {
    pub call_graph: Vec<(String, Vec<String>)>,
    pub side_effects: Vec<(String, bool)>,
}

impl FunctionAnalysis {
    pub fn analyze(program: &IRProgram) -> Self {
        let mut call_graph = Vec::new();
        let mut side_effects = Vec::new();

        for (name, func) in &program.functions {
            let mut calls = Vec::new();
            let mut has_side_effects = false;

            for block in &func.blocks {
                for instr in &block.instructions {
                    match instr {
                        IRInstruction::Call { func: callee, .. } => {
                            calls.push(callee.clone());
                        }
                        IRInstruction::Store { .. } => {
                            has_side_effects = true;
                        }
                        _ => {}
                    }
                }
            }

            call_graph.push((name.clone(), calls));
            side_effects.push((name.clone(), has_side_effects));
        }

        Self {
            call_graph,
            side_effects,
        }
    }
}
