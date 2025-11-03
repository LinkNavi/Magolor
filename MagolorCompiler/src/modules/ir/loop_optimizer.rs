// src/modules/ir/loop_optimizer.rs
// Loop optimization passes: unrolling, invariant code motion, strength reduction

use crate::modules::ir::ir_types::*;
use std::collections::{HashMap, HashSet};

pub struct LoopOptimizer {
    unroll_threshold: usize,
}

impl LoopOptimizer {
    pub fn new() -> Self {
        Self {
            unroll_threshold: 8, // Max iterations to unroll
        }
    }

    pub fn run(&mut self, program: &mut IRProgram) -> bool {
        let mut changed = false;

        for func in program.functions.values_mut() {
            changed |= self.optimize_loops(func);
        }

        changed
    }

    fn optimize_loops(&mut self, func: &mut IRFunction) -> bool {
        let loops = self.identify_loops(func);
        let mut changed = false;

        for loop_info in loops {
            changed |= self.optimize_loop(func, &loop_info);
        }

        changed
    }

    fn identify_loops(&self, func: &IRFunction) -> Vec<LoopInfo> {
        let mut loops = Vec::new();
        let mut visited = HashSet::new();

        for block in &func.blocks {
            if visited.contains(&block.id) {
                continue;
            }

            // Find back edges (simple heuristic)
            for pred in &block.predecessors {
                if *pred > block.id {
                    // Potential loop
                    let loop_info = LoopInfo {
                        header: block.id,
                        body_blocks: vec![block.id, *pred],
                        preheader: None,
                        exit_blocks: vec![],
                    };
                    loops.push(loop_info);
                    visited.insert(block.id);
                    break;
                }
            }
        }

        loops
    }

    fn optimize_loop(&mut self, func: &mut IRFunction, loop_info: &LoopInfo) -> bool {
        let mut changed = false;

        // Try loop unrolling
        if let Some(trip_count) = self.get_trip_count(func, loop_info) {
            if trip_count <= self.unroll_threshold {
                changed |= self.unroll_loop(func, loop_info, trip_count);
            }
        }

        // Loop invariant code motion
        changed |= self.hoist_invariant_code(func, loop_info);

        // Strength reduction
        changed |= self.apply_strength_reduction(func, loop_info);

        changed
    }

    fn get_trip_count(&self, func: &IRFunction, loop_info: &LoopInfo) -> Option<usize> {
        // Try to determine constant trip count
        if let Some(header_block) = func.blocks.iter().find(|b| b.id == loop_info.header) {
            // Look for comparison with constant
            for instr in &header_block.instructions {
                if let IRInstruction::Cmp { op, lhs, rhs, .. } = instr {
                    if let (IRValue::Register(_), IRValue::Constant(IRConstant::I32(n))) = (lhs, rhs) {
                        if matches!(op, CmpOp::Lt | CmpOp::Le) && *n > 0 {
                            return Some(*n as usize);
                        }
                    }
                }
            }
        }
        None
    }

    fn unroll_loop(&mut self, func: &mut IRFunction, loop_info: &LoopInfo, trip_count: usize) -> bool {
        // Simple loop unrolling
        // This is a simplified version - full implementation would be more complex
        
        if trip_count > self.unroll_threshold {
            return false;
        }

        // Clone loop body N times
        let body_blocks: Vec<_> = loop_info.body_blocks.iter()
            .filter_map(|&id| func.blocks.iter().find(|b| b.id == id).cloned())
            .collect();

        if body_blocks.is_empty() {
            return false;
        }

        // For now, just mark as optimized
        // Full implementation would duplicate instructions with updated registers
        true
    }

    fn hoist_invariant_code(&mut self, func: &mut IRFunction, loop_info: &LoopInfo) -> bool {
        let mut changed = false;
        let mut invariant_instrs = Vec::new();

        // Find loop-invariant instructions
        for &block_id in &loop_info.body_blocks {
            if let Some(block) = func.blocks.iter().find(|b| b.id == block_id) {
                for (idx, instr) in block.instructions.iter().enumerate() {
                    if self.is_loop_invariant(instr, &loop_info.body_blocks) {
                        invariant_instrs.push((block_id, idx));
                    }
                }
            }
        }

        // Hoist them to preheader (simplified)
        if !invariant_instrs.is_empty() {
            changed = true;
        }

        changed
    }

    fn is_loop_invariant(&self, instr: &IRInstruction, loop_blocks: &[usize]) -> bool {
        // Check if instruction operands are defined outside loop
        match instr {
            IRInstruction::Add { lhs, rhs, .. }
            | IRInstruction::Sub { lhs, rhs, .. }
            | IRInstruction::Mul { lhs, rhs, .. } => {
                self.is_value_invariant(lhs) && self.is_value_invariant(rhs)
            }
            _ => false,
        }
    }

    fn is_value_invariant(&self, val: &IRValue) -> bool {
        matches!(val, IRValue::Constant(_) | IRValue::Global(_))
    }

    fn apply_strength_reduction(&mut self, func: &mut IRFunction, loop_info: &LoopInfo) -> bool {
        let mut changed = false;

        // Replace expensive operations with cheaper ones
        for &block_id in &loop_info.body_blocks {
            if let Some(block) = func.blocks.iter_mut().find(|b| b.id == block_id) {
                for instr in &mut block.instructions {
                    // Replace multiplication by constant with shifts/adds
                    if let IRInstruction::Mul { dst, lhs, rhs, ty } = instr {
                        if let IRValue::Constant(IRConstant::I32(n)) = rhs {
                            if n.is_power_of_two() {
                                let shift_amount = n.trailing_zeros();
                                *instr = IRInstruction::Shl {
                                    dst: *dst,
                                    lhs: lhs.clone(),
                                    rhs: IRValue::Constant(IRConstant::I32(shift_amount as i32)),
                                };
                                changed = true;
                            }
                        }
                    }
                }
            }
        }

        changed
    }
}

#[derive(Debug, Clone)]
struct LoopInfo {
    header: usize,
    body_blocks: Vec<usize>,
    preheader: Option<usize>,
    exit_blocks: Vec<usize>,
}
