// src/modules/ir/ssa_transform.rs
// Static Single Assignment form transformation

use crate::modules::ir::ir_types::*;
use std::collections::{HashMap, HashSet};

pub struct SSATransform {
    dominance_computed: bool,
}

impl SSATransform {
    pub fn new() -> Self {
        Self {
            dominance_computed: false,
        }
    }

    pub fn run(&mut self, program: &mut IRProgram) -> bool {
        let mut changed = false;

        for func in program.functions.values_mut() {
            changed |= self.transform_to_ssa(func);
        }

        changed
    }

    fn transform_to_ssa(&mut self, func: &mut IRFunction) -> bool {
        if func.blocks.is_empty() {
            return false;
        }

        // Compute dominance information
        self.compute_dominance(func);

        // Compute dominance frontiers
        self.compute_dominance_frontiers(func);

        // Insert phi nodes
        self.insert_phi_nodes(func);

        // Rename variables
        self.rename_variables(func);

        true
    }

    fn compute_dominance(&mut self, func: &mut IRFunction) {
        let n = func.blocks.len();
        
        // Initialize: entry dominates itself, others dominated by all
        for i in 0..n {
            if i == 0 {
                func.blocks[i].dominators = vec![0];
            } else {
                func.blocks[i].dominators = (0..n).collect();
            }
        }

        // Iterative computation
        let mut changed = true;
        while changed {
            changed = false;
            
            for i in 1..n {
                let mut new_dom = HashSet::new();
                new_dom.insert(i);

                // Intersection of predecessors' dominators
                let preds: Vec<_> = func.blocks[i].predecessors.clone();
                
                if !preds.is_empty() {
                    let mut intersect: HashSet<usize> = func.blocks[preds[0]]
                        .dominators
                        .iter()
                        .cloned()
                        .collect();

                    for &pred in &preds[1..] {
                        let pred_doms: HashSet<usize> = func.blocks[pred]
                            .dominators
                            .iter()
                            .cloned()
                            .collect();
                        intersect = intersect.intersection(&pred_doms).cloned().collect();
                    }

                    new_dom.extend(intersect);
                }

                let mut sorted_dom: Vec<_> = new_dom.into_iter().collect();
                sorted_dom.sort_unstable();

                if sorted_dom != func.blocks[i].dominators {
                    func.blocks[i].dominators = sorted_dom;
                    changed = true;
                }
            }
        }
    }

    fn compute_dominance_frontiers(&mut self, func: &mut IRFunction) {
        let n = func.blocks.len();
        
        for i in 0..n {
            func.blocks[i].dom_frontier.clear();
        }

        for b in 0..n {
            let preds: Vec<_> = func.blocks[b].predecessors.clone();
            
            if preds.len() >= 2 {
                for &p in &preds {
                    let mut runner = p;
                    
                    while !self.strictly_dominates(func, runner, b) {
                        if !func.blocks[runner].dom_frontier.contains(&b) {
                            func.blocks[runner].dom_frontier.push(b);
                        }
                        
                        // Move to immediate dominator
                        if let Some(&idom) = func.blocks[runner]
                            .dominators
                            .iter()
                            .rev()
                            .find(|&&d| d != runner)
                        {
                            runner = idom;
                        } else {
                            break;
                        }
                    }
                }
            }
        }
    }

    fn strictly_dominates(&self, func: &IRFunction, dom: usize, node: usize) -> bool {
        dom != node && func.blocks[node].dominators.contains(&dom)
    }

    fn insert_phi_nodes(&mut self, func: &mut IRFunction) {
        let mut defs: HashMap<usize, Vec<usize>> = HashMap::new();

        // Collect all register definitions
        for (block_idx, block) in func.blocks.iter().enumerate() {
            for instr in &block.instructions {
                if let Some(def_reg) = self.get_defined_register(instr) {
                    defs.entry(def_reg).or_insert_with(Vec::new).push(block_idx);
                }
            }
        }

        // Insert phi nodes at dominance frontiers
        for (reg, def_blocks) in defs {
            let mut work_list = def_blocks.clone();
            let mut visited = HashSet::new();

            while let Some(block_id) = work_list.pop() {
                if visited.contains(&block_id) {
                    continue;
                }
                visited.insert(block_id);

                let df: Vec<_> = func.blocks[block_id].dom_frontier.clone();
                
                for &frontier_block in &df {
                    // Check if phi already exists
                    let has_phi = func.blocks[frontier_block]
                        .instructions
                        .iter()
                        .any(|i| matches!(i, IRInstruction::Phi { dst, .. } if *dst == reg));

                    if !has_phi {
                        // Insert phi node
                        let phi = IRInstruction::Phi {
                            dst: reg,
                            incoming: Vec::new(),
                            ty: IRType::I32, // Would need proper type tracking
                        };
                        func.blocks[frontier_block].instructions.insert(0, phi);
                        
                        if !work_list.contains(&frontier_block) {
                            work_list.push(frontier_block);
                        }
                    }
                }
            }
        }
    }

    fn rename_variables(&mut self, func: &mut IRFunction) {
        let mut stacks: HashMap<usize, Vec<usize>> = HashMap::new();
        let mut counters: HashMap<usize, usize> = HashMap::new();

        self.rename_block(func, 0, &mut stacks, &mut counters);
    }

    fn rename_block(
        &self,
        func: &mut IRFunction,
        block_id: usize,
        stacks: &mut HashMap<usize, Vec<usize>>,
        counters: &mut HashMap<usize, usize>,
    ) {
        // Process phi nodes first
        let mut phi_defs = Vec::new();
        
        for instr in &mut func.blocks[block_id].instructions {
            if let IRInstruction::Phi { dst, .. } = instr {
                let new_reg = self.new_name(*dst, counters);
                phi_defs.push((*dst, new_reg));
                stacks.entry(*dst).or_insert_with(Vec::new).push(new_reg);
            }
        }

        // Rename uses and defs in regular instructions
        // (Simplified - would need full implementation)

        // Process successors
        let successors: Vec<_> = func.blocks[block_id].successors.clone();
        for &_succ in &successors {
            // Update phi nodes in successor
            // (Implementation would add entries to phi incoming lists)
        }

        // Recursively process dominated blocks
        // (Would need to implement dominator tree traversal)

        // Pop names from stacks
        for (old_reg, _) in phi_defs {
            if let Some(stack) = stacks.get_mut(&old_reg) {
                stack.pop();
            }
        }
    }

    fn new_name(&self, old_reg: usize, counters: &mut HashMap<usize, usize>) -> usize {
        let counter = counters.entry(old_reg).or_insert(0);
        let new_reg = old_reg * 1000 + *counter; // Simple naming scheme
        *counter += 1;
        new_reg
    }

    fn get_defined_register(&self, instr: &IRInstruction) -> Option<usize> {
        match instr {
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
            | IRInstruction::Move { dst, .. } => Some(*dst),
            
            IRInstruction::Call { dst: Some(dst), .. }
            | IRInstruction::IndirectCall { dst: Some(dst), .. }
            | IRInstruction::Intrinsic { dst: Some(dst), .. } => Some(*dst),
            
            _ => None,
        }
    }
}
