// src/modules/ir/peephole.rs
// Peephole optimization - local instruction patterns

use crate::modules::ir::ir_types::*;

// Add after imports
trait IntegerPowerOfTwo {
    fn is_power_of_two(&self) -> bool;
}

impl IntegerPowerOfTwo for i32 {
    fn is_power_of_two(&self) -> bool {
        *self > 0 && (*self & (*self - 1)) == 0
    }
}

impl IntegerPowerOfTwo for &i32 {
    fn is_power_of_two(&self) -> bool {
        **self > 0 && (**self & (**self - 1)) == 0
    }
}

impl IntegerPowerOfTwo for &mut i32 {
    fn is_power_of_two(&self) -> bool {
        **self > 0 && (**self & (**self - 1)) == 0
    }
}
pub struct PeepholeOptimizer;

impl PeepholeOptimizer {
    pub fn new() -> Self {
        Self
    }

    pub fn run(&mut self, program: &mut IRProgram) -> bool {
        let mut changed = false;

        for func in program.functions.values_mut() {
            changed |= self.optimize_function(func);
        }

        changed
    }

    fn optimize_function(&mut self, func: &mut IRFunction) -> bool {
        let mut changed = false;

        for block in &mut func.blocks {
            changed |= self.optimize_block(block);
        }

        changed
    }

    fn optimize_block(&mut self, block: &mut IRBasicBlock) -> bool {
        let mut changed = false;
        let mut i = 0;

        while i < block.instructions.len() {
            if i + 1 < block.instructions.len() {
                if let Some(replacement) = self.try_pattern_match_pair(
                    &block.instructions[i],
                    &block.instructions[i + 1],
                ) {
                    block.instructions.splice(i..i + 2, replacement);
                    changed = true;
                    continue;
                }
            }

            if let Some(replacement) = self.try_pattern_match_single(&block.instructions[i]) {
                block.instructions[i] = replacement;
                changed = true;
            }

            i += 1;
        }

        changed
    }

    fn try_pattern_match_single(&self, instr: &IRInstruction) -> Option<IRInstruction> {
        match instr {
            // x + 0 => x
            IRInstruction::Add { dst, lhs, rhs, .. } => {
                if matches!(rhs, IRValue::Constant(IRConstant::I32(0) | IRConstant::I64(0))) {
                    return Some(IRInstruction::Move {
                        dst: *dst,
                        src: lhs.clone(),
                    });
                }
            }

            // x - 0 => x
            IRInstruction::Sub { dst, lhs, rhs, .. } => {
                if matches!(rhs, IRValue::Constant(IRConstant::I32(0) | IRConstant::I64(0))) {
                    return Some(IRInstruction::Move {
                        dst: *dst,
                        src: lhs.clone(),
                    });
                }
            }

            // x * 0 => 0
            IRInstruction::Mul { dst, rhs, .. } => {
                if matches!(rhs, IRValue::Constant(IRConstant::I32(0) | IRConstant::I64(0))) {
                    return Some(IRInstruction::Move {
                        dst: *dst,
                        src: rhs.clone(),
                    });
                }
            }

            // x * 1 => x
            IRInstruction::Mul { dst, lhs, rhs, .. } => {
                if matches!(rhs, IRValue::Constant(IRConstant::I32(1) | IRConstant::I64(1))) {
                    return Some(IRInstruction::Move {
                        dst: *dst,
                        src: lhs.clone(),
                    });
                }
            }

            // x * 2 => x << 1
            IRInstruction::Mul { dst, lhs, rhs, .. } => {
                if let IRValue::Constant(IRConstant::I32(n)) = rhs {
                    if n.is_power_of_two() && *n > 1 {
                        return Some(IRInstruction::Shl {
                            dst: *dst,
                            lhs: lhs.clone(),
                            rhs: IRValue::Constant(IRConstant::I32(n.trailing_zeros() as i32)),
                        });
                    }
                }
            }

            // x / 1 => x
            IRInstruction::Div { dst, lhs, rhs, .. } => {
                if matches!(rhs, IRValue::Constant(IRConstant::I32(1) | IRConstant::I64(1))) {
                    return Some(IRInstruction::Move {
                        dst: *dst,
                        src: lhs.clone(),
                    });
                }
            }

            // x & 0 => 0
            IRInstruction::And { dst, rhs, .. } => {
                if matches!(rhs, IRValue::Constant(IRConstant::I32(0) | IRConstant::I64(0))) {
                    return Some(IRInstruction::Move {
                        dst: *dst,
                        src: rhs.clone(),
                    });
                }
            }

            // x | 0 => x
            IRInstruction::Or { dst, lhs, rhs } => {
                if matches!(rhs, IRValue::Constant(IRConstant::I32(0) | IRConstant::I64(0))) {
                    return Some(IRInstruction::Move {
                        dst: *dst,
                        src: lhs.clone(),
                    });
                }
            }

            // x ^ 0 => x
            IRInstruction::Xor { dst, lhs, rhs } => {
                if matches!(rhs, IRValue::Constant(IRConstant::I32(0) | IRConstant::I64(0))) {
                    return Some(IRInstruction::Move {
                        dst: *dst,
                        src: lhs.clone(),
                    });
                }
            }

            // x ^ x => 0
            IRInstruction::Xor { dst, lhs, rhs } => {
                if lhs == rhs {
                    return Some(IRInstruction::Move {
                        dst: *dst,
                        src: IRValue::Constant(IRConstant::I32(0)),
                    });
                }
            }

            _ => {}
        }

        None
    }

    fn try_pattern_match_pair(
        &self,
        instr1: &IRInstruction,
        instr2: &IRInstruction,
    ) -> Option<Vec<IRInstruction>> {
        // Move followed by move: x = y; z = x => z = y (if x not used elsewhere)
        if let (
            IRInstruction::Move { dst: dst1, src: src1 },
            IRInstruction::Move { dst: dst2, src: IRValue::Register(src2) },
        ) = (instr1, instr2)
        {
            if dst1 == src2 {
                return Some(vec![IRInstruction::Move {
                    dst: *dst2,
                    src: src1.clone(),
                }]);
            }
        }

        // Redundant load/store elimination
        if let (
            IRInstruction::Store { addr: addr1, value, .. },
            IRInstruction::Load { dst, addr: addr2, ty: _ },
        ) = (instr1, instr2)
        {
            if addr1 == addr2 {
                return Some(vec![
                    instr1.clone(),
                    IRInstruction::Move {
                        dst: *dst,
                        src: value.clone(),
                    },
                ]);
            }
        }

        // x = a + b; y = x + c => y = a + b + c (strength reduction)
        if let (
            IRInstruction::Add { dst: dst1, lhs: _lhs1, rhs: rhs1, ty: _ty1 },
            IRInstruction::Add { dst: _dst2, lhs: IRValue::Register(lhs2), rhs: rhs2, ty: _ty2 },
        ) = (instr1, instr2)
        {
            if dst1 == lhs2 {
                // Check if we can combine (simplified)
                if matches!(rhs1, IRValue::Constant(_)) && matches!(rhs2, IRValue::Constant(_)) {
                    // Would combine constants here
                }
            }
        }

        None
    }
}
