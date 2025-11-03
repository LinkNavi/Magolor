// src/modules/ir/constant_folding.rs
// Constant folding optimization pass - FIXED VERSION

use crate::modules::ir::ir_types::*;

pub struct ConstantFolder;

impl ConstantFolder {
    pub fn new() -> Self {
        Self
    }

    pub fn run(&mut self, program: &mut IRProgram) -> bool {
        let mut changed = false;

        for func in program.functions.values_mut() {
            changed |= self.fold_function(func);
        }

        changed
    }

    fn fold_function(&mut self, func: &mut IRFunction) -> bool {
        let mut changed = false;

        for block in &mut func.blocks {
            changed |= self.fold_block(block);
        }

        changed
    }

    fn fold_block(&mut self, block: &mut IRBasicBlock) -> bool {
        let mut changed = false;
        let mut new_instructions = Vec::new();

        for instr in &block.instructions {
            if let Some(folded) = self.fold_instruction(instr) {
                new_instructions.push(folded);
                changed = true;
            } else {
                new_instructions.push(instr.clone());
            }
        }

        if changed {
            block.instructions = new_instructions;
        }

        changed
    }

    fn fold_instruction(&self, instr: &IRInstruction) -> Option<IRInstruction> {
        match instr {
            IRInstruction::Add { dst, lhs, rhs, .. } => {
                if let (IRValue::Constant(c1), IRValue::Constant(c2)) = (lhs, rhs) {
                    if let Some(result) = self.fold_add(c1, c2) {
                        return Some(IRInstruction::Move {
                            dst: *dst,
                            src: IRValue::Constant(result),
                        });
                    }
                }
            }
            IRInstruction::Sub { dst, lhs, rhs, .. } => {
                if let (IRValue::Constant(c1), IRValue::Constant(c2)) = (lhs, rhs) {
                    if let Some(result) = self.fold_sub(c1, c2) {
                        return Some(IRInstruction::Move {
                            dst: *dst,
                            src: IRValue::Constant(result),
                        });
                    }
                }
            }
            IRInstruction::Mul { dst, lhs, rhs, .. } => {
                if let (IRValue::Constant(c1), IRValue::Constant(c2)) = (lhs, rhs) {
                    if let Some(result) = self.fold_mul(c1, c2) {
                        return Some(IRInstruction::Move {
                            dst: *dst,
                            src: IRValue::Constant(result),
                        });
                    }
                }
            }
            IRInstruction::Div { dst, lhs, rhs, .. } => {
                if let (IRValue::Constant(c1), IRValue::Constant(c2)) = (lhs, rhs) {
                    if let Some(result) = self.fold_div(c1, c2) {
                        return Some(IRInstruction::Move {
                            dst: *dst,
                            src: IRValue::Constant(result),
                        });
                    }
                }
            }
            _ => {}
        }
        None
    }

    fn fold_add(&self, c1: &IRConstant, c2: &IRConstant) -> Option<IRConstant> {
        match (c1, c2) {
            (IRConstant::I32(a), IRConstant::I32(b)) => Some(IRConstant::I32(a.wrapping_add(*b))),
            (IRConstant::I64(a), IRConstant::I64(b)) => Some(IRConstant::I64(a.wrapping_add(*b))),
            (IRConstant::F32(a), IRConstant::F32(b)) => {
                let result = a.as_f32() + b.as_f32();
                Some(IRConstant::F32(result.into()))
            }
            (IRConstant::F64(a), IRConstant::F64(b)) => {
                let result = a.as_f64() + b.as_f64();
                Some(IRConstant::F64(result.into()))
            }
            _ => None,
        }
    }

    fn fold_sub(&self, c1: &IRConstant, c2: &IRConstant) -> Option<IRConstant> {
        match (c1, c2) {
            (IRConstant::I32(a), IRConstant::I32(b)) => Some(IRConstant::I32(a.wrapping_sub(*b))),
            (IRConstant::I64(a), IRConstant::I64(b)) => Some(IRConstant::I64(a.wrapping_sub(*b))),
            (IRConstant::F32(a), IRConstant::F32(b)) => {
                let result = a.as_f32() - b.as_f32();
                Some(IRConstant::F32(result.into()))
            }
            (IRConstant::F64(a), IRConstant::F64(b)) => {
                let result = a.as_f64() - b.as_f64();
                Some(IRConstant::F64(result.into()))
            }
            _ => None,
        }
    }

    fn fold_mul(&self, c1: &IRConstant, c2: &IRConstant) -> Option<IRConstant> {
        match (c1, c2) {
            (IRConstant::I32(a), IRConstant::I32(b)) => Some(IRConstant::I32(a.wrapping_mul(*b))),
            (IRConstant::I64(a), IRConstant::I64(b)) => Some(IRConstant::I64(a.wrapping_mul(*b))),
            (IRConstant::F32(a), IRConstant::F32(b)) => {
                let result = a.as_f32() * b.as_f32();
                Some(IRConstant::F32(result.into()))
            }
            (IRConstant::F64(a), IRConstant::F64(b)) => {
                let result = a.as_f64() * b.as_f64();
                Some(IRConstant::F64(result.into()))
            }
            _ => None,
        }
    }

    fn fold_div(&self, c1: &IRConstant, c2: &IRConstant) -> Option<IRConstant> {
        match (c1, c2) {
            (IRConstant::I32(a), IRConstant::I32(b)) if *b != 0 => {
                Some(IRConstant::I32(a.wrapping_div(*b)))
            }
            (IRConstant::I64(a), IRConstant::I64(b)) if *b != 0 => {
                Some(IRConstant::I64(a.wrapping_div(*b)))
            }
            (IRConstant::F32(a), IRConstant::F32(b)) if b.as_f32() != 0.0 => {
                let result = a.as_f32() / b.as_f32();
                Some(IRConstant::F32(result.into()))
            }
            (IRConstant::F64(a), IRConstant::F64(b)) if b.as_f64() != 0.0 => {
                let result = a.as_f64() / b.as_f64();
                Some(IRConstant::F64(result.into()))
            }
            _ => None,
        }
    }
}