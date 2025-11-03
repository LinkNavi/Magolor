// src/modules/ir/register_allocator.rs
// Register allocation using graph coloring

use crate::modules::ir::ir_types::*;
use crate::modules::ir::control_flow::{ControlFlowAnalyzer, LiveRange};
use std::collections::{HashMap, HashSet};

pub struct RegisterAllocator {
    available_registers: usize,
    allocation: HashMap<usize, PhysicalRegister>,
    spilled: HashSet<usize>,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum PhysicalRegister {
    // x86-64 registers
    RAX,
    RBX,
    RCX,
    RDX,
    RSI,
    RDI,
    R8,
    R9,
    R10,
    R11,
    R12,
    R13,
    R14,
    R15,
    Spilled(usize), // Stack slot
}

impl RegisterAllocator {
    pub fn new(num_registers: usize) -> Self {
        Self {
            available_registers: num_registers,
            allocation: HashMap::new(),
            spilled: HashSet::new(),
        }
    }

    pub fn allocate(&mut self, func: &mut IRFunction) -> Result<(), String> {
        // Compute live ranges
        let live_ranges = ControlFlowAnalyzer::compute_live_ranges(func);

        // Build interference graph
        let interference = self.build_interference_graph(&live_ranges);

        // Color the graph
        let coloring = self.graph_coloring(&interference, &live_ranges)?;

        // Assign physical registers
        self.assign_registers(coloring);

        // Insert spill code if needed
        if !self.spilled.is_empty() {
            self.insert_spill_code(func)?;
        }

        Ok(())
    }

    fn build_interference_graph(
        &self,
        live_ranges: &HashMap<usize, LiveRange>,
    ) -> HashMap<usize, HashSet<usize>> {
        let mut graph: HashMap<usize, HashSet<usize>> = HashMap::new();

        let registers: Vec<_> = live_ranges.keys().cloned().collect();

        for i in 0..registers.len() {
            for j in (i + 1)..registers.len() {
                let r1 = registers[i];
                let r2 = registers[j];

                if live_ranges[&r1].overlaps(&live_ranges[&r2]) {
                    graph.entry(r1).or_insert_with(HashSet::new).insert(r2);
                    graph.entry(r2).or_insert_with(HashSet::new).insert(r1);
                }
            }
        }

        graph
    }

    fn graph_coloring(
        &mut self,
        interference: &HashMap<usize, HashSet<usize>>,
        live_ranges: &HashMap<usize, LiveRange>,
    ) -> Result<HashMap<usize, usize>, String> {
        let mut coloring: HashMap<usize, usize> = HashMap::new();
        let mut stack = Vec::new();
        let mut remaining: HashSet<_> = interference.keys().cloned().collect();

        // Simplification: remove nodes with degree < k
        while !remaining.is_empty() {
            // Find node with minimum degree
            let min_node = remaining
                .iter()
                .min_by_key(|&&node| {
                    interference
                        .get(&node)
                        .map(|neighbors| {
                            neighbors.intersection(&remaining).count()
                        })
                        .unwrap_or(0)
                })
                .cloned();

            if let Some(node) = min_node {
                let degree = interference
                    .get(&node)
                    .map(|n| n.intersection(&remaining).count())
                    .unwrap_or(0);

                if degree < self.available_registers {
                    // Can potentially color this node
                    stack.push(node);
                    remaining.remove(&node);
                } else {
                    // Need to spill - choose node with lowest priority
                    let spill_node = self.choose_spill_candidate(&remaining, live_ranges);
                    self.spilled.insert(spill_node);
                    stack.push(spill_node);
                    remaining.remove(&spill_node);
                }
            } else {
                break;
            }
        }

        // Selection: assign colors
        while let Some(node) = stack.pop() {
            if self.spilled.contains(&node) {
                continue;
            }

            // Find available color
            let mut used_colors = HashSet::new();
            if let Some(neighbors) = interference.get(&node) {
                for &neighbor in neighbors {
                    if let Some(&color) = coloring.get(&neighbor) {
                        used_colors.insert(color);
                    }
                }
            }

            // Assign lowest available color
            let mut color = 0;
            while used_colors.contains(&color) {
                color += 1;
            }

            if color >= self.available_registers {
                self.spilled.insert(node);
            } else {
                coloring.insert(node, color);
            }
        }

        Ok(coloring)
    }

    fn choose_spill_candidate(
        &self,
        candidates: &HashSet<usize>,
        live_ranges: &HashMap<usize, LiveRange>,
    ) -> usize {
        // Spill the register with the longest live range (simple heuristic)
        candidates
            .iter()
            .max_by_key(|&&reg| {
                live_ranges
                    .get(&reg)
                    .map(|lr| lr.uses.len())
                    .unwrap_or(0)
            })
            .cloned()
            .unwrap()
    }

    fn assign_registers(&mut self, coloring: HashMap<usize, usize>) {
        let phys_regs = [
            PhysicalRegister::RAX,
            PhysicalRegister::RBX,
            PhysicalRegister::RCX,
            PhysicalRegister::RDX,
            PhysicalRegister::RSI,
            PhysicalRegister::RDI,
            PhysicalRegister::R8,
            PhysicalRegister::R9,
            PhysicalRegister::R10,
            PhysicalRegister::R11,
            PhysicalRegister::R12,
            PhysicalRegister::R13,
            PhysicalRegister::R14,
            PhysicalRegister::R15,
        ];

        for (virtual_reg, color) in coloring {
            if color < phys_regs.len() {
                self.allocation.insert(virtual_reg, phys_regs[color]);
            }
        }

        // Assign stack slots to spilled registers
        for (i, &spilled_reg) in self.spilled.iter().enumerate() {
            self.allocation.insert(spilled_reg, PhysicalRegister::Spilled(i));
        }
    }

    fn insert_spill_code(&mut self, func: &mut IRFunction) -> Result<(), String> {
        for block in &mut func.blocks {
            let mut new_instructions = Vec::new();

            for instr in &block.instructions {
                // Insert loads before uses of spilled registers
                let uses = self.get_used_virtual_regs(instr);
                for &reg in &uses {
                    if self.spilled.contains(&reg) {
                        if let Some(PhysicalRegister::Spilled(slot)) = self.allocation.get(&reg) {
                            // Insert load from stack
                            let temp_reg = func.register_count;
                            func.register_count += 1;
                            
                            new_instructions.push(IRInstruction::Load {
                                dst: temp_reg,
                                addr: IRValue::Local(*slot),
                                ty: IRType::I64,
                            });
                        }
                    }
                }

                new_instructions.push(instr.clone());

                // Insert stores after defs of spilled registers
                if let Some(def_reg) = self.get_defined_virtual_reg(instr) {
                    if self.spilled.contains(&def_reg) {
                        if let Some(PhysicalRegister::Spilled(slot)) = self.allocation.get(&def_reg) {
                            new_instructions.push(IRInstruction::Store {
                                addr: IRValue::Local(*slot),
                                value: IRValue::Register(def_reg),
                                ty: IRType::I64,
                            });
                        }
                    }
                }
            }

            block.instructions = new_instructions;
        }

        Ok(())
    }

    fn get_used_virtual_regs(&self, instr: &IRInstruction) -> Vec<usize> {
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

    fn get_defined_virtual_reg(&self, instr: &IRInstruction) -> Option<usize> {
        match instr {
            IRInstruction::Add { dst, .. }
            | IRInstruction::Sub { dst, .. }
            | IRInstruction::Mul { dst, .. }
            | IRInstruction::Move { dst, .. } => Some(*dst),
            _ => None,
        }
    }

    pub fn get_allocation(&self) -> &HashMap<usize, PhysicalRegister> {
        &self.allocation
    }
}
