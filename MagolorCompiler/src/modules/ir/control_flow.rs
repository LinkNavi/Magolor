// src/modules/ir/control_flow.rs
// Control flow graph construction and analysis

use crate::modules::ir::ir_types::*;
use std::collections::{HashMap, HashSet, VecDeque};

pub struct ControlFlowAnalyzer;

impl ControlFlowAnalyzer {
    pub fn analyze(func: &mut IRFunction) {
        Self::build_cfg(func);
        Self::compute_dominators(func);
        Self::compute_postdominators(func);
        Self::identify_natural_loops(func);
    }

    fn build_cfg(func: &mut IRFunction) {
        // Clear existing edges
        for block in &mut func.blocks {
            block.predecessors.clear();
            block.successors.clear();
        }

        // Build edges from terminator instructions
        for i in 0..func.blocks.len() {
            let successors = Self::get_successors(&func.blocks[i]);
            
            for &succ in &successors {
                if succ < func.blocks.len() {
                    func.blocks[i].successors.push(succ);
                    func.blocks[succ].predecessors.push(i);
                }
            }
        }
    }

    fn get_successors(block: &IRBasicBlock) -> Vec<usize> {
        if let Some(last) = block.instructions.last() {
            match last {
                IRInstruction::Branch { target } => vec![*target],
                IRInstruction::CondBranch { true_target, false_target, .. } => {
                    vec![*true_target, *false_target]
                }
                IRInstruction::Return { .. } => vec![],
                _ => vec![],
            }
        } else {
            vec![]
        }
    }

    fn compute_dominators(func: &mut IRFunction) {
        let n = func.blocks.len();
        if n == 0 {
            return;
        }

        // Initialize
        func.blocks[0].dominators = vec![0];
        for i in 1..n {
            func.blocks[i].dominators = (0..n).collect();
        }

        // Iterative dataflow
        let mut changed = true;
        while changed {
            changed = false;
            
            for i in 1..n {
                let preds: Vec<_> = func.blocks[i].predecessors.clone();
                
                if preds.is_empty() {
                    continue;
                }

                // Start with first predecessor's dominators
                let mut new_doms: HashSet<usize> = func.blocks[preds[0]]
                    .dominators
                    .iter()
                    .cloned()
                    .collect();

                // Intersect with other predecessors
                for &pred in &preds[1..] {
                    let pred_doms: HashSet<usize> = func.blocks[pred]
                        .dominators
                        .iter()
                        .cloned()
                        .collect();
                    new_doms = new_doms.intersection(&pred_doms).cloned().collect();
                }

                // Add self
                new_doms.insert(i);

                let mut sorted: Vec<_> = new_doms.into_iter().collect();
                sorted.sort_unstable();

                if sorted != func.blocks[i].dominators {
                    func.blocks[i].dominators = sorted;
                    changed = true;
                }
            }
        }
    }

    fn compute_postdominators(_func: &mut IRFunction) {
        // Similar to dominators but computed backwards from exits
        // Implementation would mirror compute_dominators with reversed CFG
    }

    fn identify_natural_loops(func: &mut IRFunction) {
        // Find back edges using dominance information
        let mut back_edges = Vec::new();

        for i in 0..func.blocks.len() {
            for &succ in &func.blocks[i].successors {
                // Back edge if successor dominates current
                if func.blocks[i].dominators.contains(&succ) {
                    back_edges.push((i, succ));
                }
            }
        }

        // Each back edge defines a natural loop
        for (tail, header) in back_edges {
            let _loop_blocks = Self::find_loop_blocks(func, header, tail);
            // Store loop information for later optimization
        }
    }

    fn find_loop_blocks(func: &IRFunction, header: usize, tail: usize) -> Vec<usize> {
        let mut loop_blocks = HashSet::new();
        let mut work_list = VecDeque::new();

        loop_blocks.insert(header);
        work_list.push_back(tail);

        while let Some(node) = work_list.pop_front() {
            if loop_blocks.insert(node) {
                for &pred in &func.blocks[node].predecessors {
                    if !loop_blocks.contains(&pred) {
                        work_list.push_back(pred);
                    }
                }
            }
        }

        loop_blocks.into_iter().collect()
    }

    pub fn compute_reaching_definitions(func: &IRFunction) -> HashMap<usize, HashSet<usize>> {
        let mut reaching: HashMap<usize, HashSet<usize>> = HashMap::new();

        // Initialize
        for block in &func.blocks {
            reaching.insert(block.id, HashSet::new());
        }

        // Iterative dataflow
        let mut changed = true;
        while changed {
            changed = false;

            for block in &func.blocks {
                let mut new_reaching = HashSet::new();

                // Union of predecessors' out sets
                for &pred in &block.predecessors {
                    if let Some(pred_defs) = reaching.get(&pred) {
                        new_reaching.extend(pred_defs.iter().cloned());
                    }
                }

                // Add definitions in this block
                for instr in &block.instructions {
                    if let Some(def) = Self::get_defined_reg(instr) {
                        new_reaching.insert(def);
                    }
                }

                if new_reaching != *reaching.get(&block.id).unwrap() {
                    reaching.insert(block.id, new_reaching);
                    changed = true;
                }
            }
        }

        reaching
    }

    fn get_defined_reg(instr: &IRInstruction) -> Option<usize> {
        match instr {
            IRInstruction::Add { dst, .. }
            | IRInstruction::Sub { dst, .. }
            | IRInstruction::Mul { dst, .. }
            | IRInstruction::Load { dst, .. }
            | IRInstruction::Move { dst, .. } => Some(*dst),
            _ => None,
        }
    }

    pub fn compute_live_ranges(func: &IRFunction) -> HashMap<usize, LiveRange> {
        let mut live_ranges = HashMap::new();

        for (block_idx, block) in func.blocks.iter().enumerate() {
            for (instr_idx, instr) in block.instructions.iter().enumerate() {
                // Track register uses and defs
                let uses = Self::get_used_registers(instr);
                let defs = Self::get_defined_reg(instr);

                for &reg in &uses {
                    live_ranges
                        .entry(reg)
                        .or_insert_with(|| LiveRange::new(reg))
                        .add_use(block_idx, instr_idx);
                }

                if let Some(reg) = defs {
                    live_ranges
                        .entry(reg)
                        .or_insert_with(|| LiveRange::new(reg))
                        .add_def(block_idx, instr_idx);
                }
            }
        }

        live_ranges
    }

    fn get_used_registers(instr: &IRInstruction) -> Vec<usize> {
        let mut regs = Vec::new();

        match instr {
            IRInstruction::Add { lhs, rhs, .. }
            | IRInstruction::Sub { lhs, rhs, .. }
            | IRInstruction::Mul { lhs, rhs, .. } => {
                if let IRValue::Register(r) = lhs {
                    regs.push(*r);
                }
                if let IRValue::Register(r) = rhs {
                    regs.push(*r);
                }
            }
            IRInstruction::Move { src, .. } => {
                if let IRValue::Register(r) = src {
                    regs.push(*r);
                }
            }
            _ => {}
        }

        regs
    }
}

#[derive(Debug, Clone)]
pub struct LiveRange {
    pub register: usize,
    pub start: (usize, usize), // (block, instruction)
    pub end: (usize, usize),
    pub uses: Vec<(usize, usize)>,
}

impl LiveRange {
    fn new(register: usize) -> Self {
        Self {
            register,
            start: (usize::MAX, usize::MAX),
            end: (0, 0),
            uses: Vec::new(),
        }
    }

    fn add_def(&mut self, block: usize, instr: usize) {
        if (block, instr) < self.start {
            self.start = (block, instr);
        }
        if (block, instr) > self.end {
            self.end = (block, instr);
        }
    }

    fn add_use(&mut self, block: usize, instr: usize) {
        self.uses.push((block, instr));
        if (block, instr) < self.start {
            self.start = (block, instr);
        }
        if (block, instr) > self.end {
            self.end = (block, instr);
        }
    }

    pub fn overlaps(&self, other: &LiveRange) -> bool {
        !(self.end < other.start || other.end < self.start)
    }
}
