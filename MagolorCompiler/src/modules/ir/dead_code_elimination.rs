// src/modules/ir/dead_code_elimination.rs
// Dead code elimination pass

use crate::modules::ir::ir_types::*;
use std::collections::HashSet;

pub struct DeadCodeEliminator {
    live_registers: HashSet<usize>,
    live_blocks: HashSet<usize>,
}

impl DeadCodeEliminator {
    pub fn new() -> Self {
        Self {
            live_registers: HashSet::new(),
            live_blocks: HashSet::new(),
        }
    }

    pub fn run(&mut self, program: &mut IRProgram) -> bool {
        let mut changed = false;

        for func in program.functions.values_mut() {
            changed |= self.eliminate_dead_code(func);
        }

        changed
    }

    fn eliminate_dead_code(&mut self, func: &mut IRFunction) -> bool {
        // Mark live blocks
        self.live_blocks.clear();
        self.mark_reachable_blocks(func, 0);

        // Remove unreachable blocks
        let old_block_count = func.blocks.len();
        func.blocks.retain(|b| self.live_blocks.contains(&b.id));
        let blocks_removed = old_block_count != func.blocks.len();

        // Mark live registers
        self.live_registers.clear();
        self.mark_live_registers(func);

        // Remove dead instructions
        let mut instrs_removed = false;
        for block in &mut func.blocks {
            let old_len = block.instructions.len();
            block.instructions.retain(|instr| self.is_instruction_live(instr));
            instrs_removed |= old_len != block.instructions.len();
        }

        blocks_removed || instrs_removed
    }

    fn mark_reachable_blocks(&mut self, func: &IRFunction, block_id: usize) {
        if self.live_blocks.contains(&block_id) || block_id >= func.blocks.len() {
            return;
        }

        self.live_blocks.insert(block_id);

        if let Some(block) = func.blocks.iter().find(|b| b.id == block_id) {
            for successor in &block.successors {
                self.mark_reachable_blocks(func, *successor);
            }

            // Also check last instruction for branches
            if let Some(last) = block.instructions.last() {
                match last {
                    IRInstruction::Branch { target } => {
                        self.mark_reachable_blocks(func, *target);
                    }
                    IRInstruction::CondBranch { true_target, false_target, .. } => {
                        self.mark_reachable_blocks(func, *true_target);
                        self.mark_reachable_blocks(func, *false_target);
                    }
                    _ => {}
                }
            }
        }
    }

    fn mark_live_registers(&mut self, func: &IRFunction) {
        // Start from instructions with side effects
        for block in &func.blocks {
            for instr in &block.instructions {
                self.mark_instruction_operands(instr);
            }
        }
    }

    fn mark_instruction_operands(&mut self, instr: &IRInstruction) {
        match instr {
            IRInstruction::Return { value: Some(val) } => {
                self.mark_value_live(val);
            }
            IRInstruction::Store { value, .. } => {
                self.mark_value_live(value);
            }
            IRInstruction::Call { args, .. } | IRInstruction::IndirectCall { args, .. } => {
                for arg in args {
                    self.mark_value_live(arg);
                }
            }
            IRInstruction::CondBranch { cond, .. } => {
                self.mark_value_live(cond);
            }
            _ => {}
        }
    }

    fn mark_value_live(&mut self, val: &IRValue) {
        if let IRValue::Register(r) = val {
            self.live_registers.insert(*r);
        }
    }

    fn is_instruction_live(&self, instr: &IRInstruction) -> bool {
        match instr {
            // Always keep these
            IRInstruction::Return { .. }
            | IRInstruction::Branch { .. }
            | IRInstruction::CondBranch { .. }
            | IRInstruction::Store { .. }
            | IRInstruction::Call { .. }
            | IRInstruction::IndirectCall { .. } => true,

            // Keep if destination is live
            IRInstruction::Add { dst, .. }
            | IRInstruction::Sub { dst, .. }
            | IRInstruction::Mul { dst, .. }
            | IRInstruction::Div { dst, .. }
            | IRInstruction::Mod { dst, .. }
            | IRInstruction::And { dst, .. }
            | IRInstruction::Or { dst, .. }
            | IRInstruction::Xor { dst, .. }
            | IRInstruction::Shl { dst, .. }
            | IRInstruction::Shr { dst, .. }
            | IRInstruction::Not { dst, .. }
            | IRInstruction::Cmp { dst, .. }
            | IRInstruction::Load { dst, .. }
            | IRInstruction::Alloca { dst, .. }
            | IRInstruction::Cast { dst, .. }
            | IRInstruction::Bitcast { dst, .. }
            | IRInstruction::GetElementPtr { dst, .. }
            | IRInstruction::ExtractValue { dst, .. }
            | IRInstruction::InsertValue { dst, .. }
            | IRInstruction::Phi { dst, .. }
            | IRInstruction::Select { dst, .. }
            | IRInstruction::Move { dst, .. } => self.live_registers.contains(dst),

            IRInstruction::Intrinsic { dst, .. } => {
                if let Some(d) = dst {
                    self.live_registers.contains(d)
                } else {
                    true // Side effects
                }
            }
        }
    }
}
