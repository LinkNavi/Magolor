// src/modules/ir/codegen.rs
// Final code generation to target assembly/machine code

use crate::modules::ir::ir_types::*;
use crate::modules::ir::register_allocator::{RegisterAllocator, PhysicalRegister};
use std::collections::HashMap;

pub struct CodeGenerator {
    target: Target,
    register_allocator: RegisterAllocator,
}

#[derive(Debug, Clone, Copy)]
pub enum Target {
    X86_64,
    ARM64,
    WASM,
    LLVM,
}

impl CodeGenerator {
    pub fn new(target: Target) -> Self {
        let num_registers = match target {
            Target::X86_64 => 14, // General purpose registers
            Target::ARM64 => 28,
            Target::WASM => 0, // Stack-based
            Target::LLVM => 0, // LLVM handles register allocation
        };

        Self {
            target,
            register_allocator: RegisterAllocator::new(num_registers),
        }
    }

    pub fn generate(&mut self, program: &IRProgram) -> Result<String, String> {
        match self.target {
            Target::X86_64 => self.generate_x86_64(program),
            Target::ARM64 => self.generate_arm64(program),
            Target::WASM => self.generate_wasm(program),
            Target::LLVM => self.generate_llvm_ir(program),
        }
    }

    fn generate_x86_64(&mut self, program: &IRProgram) -> Result<String, String> {
        let mut output = String::new();

        // Assembly header
        output.push_str("    .text\n");
        output.push_str("    .intel_syntax noprefix\n\n");

        // Generate code for each function
        for (name, func) in &program.functions {
            output.push_str(&self.generate_function_x86_64(name, func)?);
        }

        // Data section for globals and strings
        if !program.globals.is_empty() || !program.string_literals.is_empty() {
            output.push_str("\n    .data\n");
            
            for (str_val, id) in &program.string_literals {
                output.push_str(&format!("__str_{}:\n", id));
                output.push_str(&format!("    .asciz \"{}\"\n", str_val));
            }

            for (name, global) in &program.globals {
                output.push_str(&format!("{}:\n", name));
                if let Some(init) = &global.init {
                    output.push_str(&format!("    .quad {}\n", self.format_constant(init)));
                } else {
                    output.push_str(&format!("    .zero {}\n", global.ty.size_bytes()));
                }
            }
        }

        Ok(output)
    }

   // Fix in src/modules/ir/codegen.rs
// The issue is that we're calling register allocator but it may fail or not allocate all registers
// For now, let's use a simpler approach: direct virtual-to-physical mapping

fn generate_function_x86_64(&mut self, name: &str, func: &IRFunction) -> Result<String, String> {
    let mut output = String::new();

    // Function label
    output.push_str(&format!("    .globl {}\n", name));
    output.push_str(&format!("{}:\n", name));

    // Prologue
    output.push_str("    push rbp\n");
    output.push_str("    mov rbp, rsp\n");

    // Allocate stack space for locals
    if func.local_count > 0 {
        let stack_size = func.local_count * 8;
        output.push_str(&format!("    sub rsp, {}\n", stack_size));
    }

    // SIMPLIFIED: Use a simple virtual-to-physical register mapping
    // Instead of complex graph coloring, map virtual regs to a rotating set of physical regs
    let allocation = self.simple_register_map(func);

    // Generate code for each block
    for block in &func.blocks {
        let label = format!(".L_{}_{}",  name.replace(".", "_"), block.id);
        output.push_str(&format!("{}:\n", label));

        for instr in &block.instructions {
            output.push_str(&self.generate_instruction_x86_64(instr, &allocation, name)?);
        }
    }

    // Default epilogue (if no explicit return)
    let epilogue_label = format!(".L_{}_epilogue",  name.replace(".", "_"));
        output.push_str(&format!("{}:
", epilogue_label));
    output.push_str("    mov rsp, rbp\n");
    output.push_str("    pop rbp\n");
    output.push_str("    ret\n\n");

    Ok(output)
}

// Add this helper method to create a simple register allocation
fn simple_register_map(&self, func: &IRFunction) -> HashMap<usize, PhysicalRegister> {
    let mut allocation = HashMap::new();
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

    // Simple round-robin allocation
    for vreg in 0..func.register_count {
        if vreg < phys_regs.len() {
            allocation.insert(vreg, phys_regs[vreg]);
        } else {
            // Spill to stack for registers beyond available physical regs
            allocation.insert(vreg, PhysicalRegister::Spilled(vreg - phys_regs.len()));
        }
    }

    allocation
}

    fn generate_instruction_x86_64(
        &self,
        instr: &IRInstruction,
        allocation: &HashMap<usize, PhysicalRegister>,
        func_name: &str,
    ) -> Result<String, String> {
        match instr {
            IRInstruction::Add { dst, lhs, rhs, .. } => {
                let dst_reg = self.get_physical_reg(*dst, allocation)?;
                let lhs_str = self.format_operand_x86_64(lhs, allocation)?;
                let rhs_str = self.format_operand_x86_64(rhs, allocation)?;
                Ok(format!("    mov {}, {}\n    add {}, {}\n", dst_reg, lhs_str, dst_reg, rhs_str))
            }

            IRInstruction::Sub { dst, lhs, rhs, .. } => {
                let dst_reg = self.get_physical_reg(*dst, allocation)?;
                let lhs_str = self.format_operand_x86_64(lhs, allocation)?;
                let rhs_str = self.format_operand_x86_64(rhs, allocation)?;
                Ok(format!("    mov {}, {}\n    sub {}, {}\n", dst_reg, lhs_str, dst_reg, rhs_str))
            }

            IRInstruction::Mul { dst, lhs, rhs, .. } => {
                let dst_reg = self.get_physical_reg(*dst, allocation)?;
                let lhs_str = self.format_operand_x86_64(lhs, allocation)?;
                let rhs_str = self.format_operand_x86_64(rhs, allocation)?;
                Ok(format!("    mov {}, {}\n    imul {}, {}\n", dst_reg, lhs_str, dst_reg, rhs_str))
            }

            IRInstruction::Load { dst, addr, .. } => {
                let dst_reg = self.get_physical_reg(*dst, allocation)?;
                let addr_str = self.format_operand_x86_64(addr, allocation)?;
                Ok(format!("    mov {}, qword ptr [{}]\n", dst_reg, addr_str))
            }

            IRInstruction::Store { addr, value, .. } => {
                let addr_str = self.format_operand_x86_64(addr, allocation)?;
                let value_str = self.format_operand_x86_64(value, allocation)?;
                Ok(format!("    mov qword ptr [{}], {}\n", addr_str, value_str))
            }

            IRInstruction::Move { dst, src } => {
                let dst_reg = self.get_physical_reg(*dst, allocation)?;
                let src_str = self.format_operand_x86_64(src, allocation)?;
                Ok(format!("    mov {}, {}\n", dst_reg, src_str))
            }

            IRInstruction::Branch { target } => {
                let label = format!(".L_{}_{}",  func_name.replace(".", "_"), target);
                Ok(format!("    jmp {}\n", label))
            }

            IRInstruction::CondBranch { cond, true_target, false_target } => {
                let cond_str = self.format_operand_x86_64(cond, allocation)?;
                let true_label = format!(".L_{}_{}",  func_name.replace(".", "_"), true_target);
                let false_label = format!(".L_{}_{}",  func_name.replace(".", "_"), false_target);
                Ok(format!(
                    "    test {}, {}\n    jnz {}\n    jmp {}\n",
                    cond_str, cond_str, true_label, false_label
                ))
            }

            IRInstruction::Return { value } => {
                let mut code = String::new();
                if let Some(val) = value {
                    let val_str = self.format_operand_x86_64(val, allocation)?;
                    code.push_str(&format!("    mov rax, {}\n", val_str));
                }
                let epilogue_label = format!(".L_{}_epilogue",  func_name.replace(".", "_"));
                code.push_str(&format!("    jmp {}\n", epilogue_label));
                Ok(code)
            }

            IRInstruction::Call { dst, func, args, .. } => {
                let mut code = String::new();
                
                // Pass arguments (x86-64 calling convention)
                let arg_regs = ["rdi", "rsi", "rdx", "rcx", "r8", "r9"];
                for (i, arg) in args.iter().enumerate() {
                    if i < arg_regs.len() {
                        let arg_str = self.format_operand_x86_64(arg, allocation)?;
                        code.push_str(&format!("    mov {}, {}\n", arg_regs[i], arg_str));
                    } else {
                        // Push to stack for additional arguments
                        let arg_str = self.format_operand_x86_64(arg, allocation)?;
                        code.push_str(&format!("    push {}\n", arg_str));
                    }
                }

                code.push_str(&format!("    call {}\n", func));

                if let Some(dst_reg) = dst {
                    let dst_str = self.get_physical_reg(*dst_reg, allocation)?;
                    code.push_str(&format!("    mov {}, rax\n", dst_str));
                }

                Ok(code)
            }

            IRInstruction::Cmp { dst, op, lhs, rhs, .. } => {
                let lhs_str = self.format_operand_x86_64(lhs, allocation)?;
                let rhs_str = self.format_operand_x86_64(rhs, allocation)?;
                let dst_reg = self.get_physical_reg(*dst, allocation)?;
                
                let set_instr = match op {
                    CmpOp::Eq | CmpOp::FEq => "sete",
                    CmpOp::Ne | CmpOp::FNe => "setne",
                    CmpOp::Lt | CmpOp::FLt => "setl",
                    CmpOp::Le | CmpOp::FLe => "setle",
                    CmpOp::Gt | CmpOp::FGt => "setg",
                    CmpOp::Ge | CmpOp::FGe => "setge",
                };

                Ok(format!(
                    "    cmp {}, {}\n    {} {}\n    movzx {}, {}\n",
                    lhs_str, rhs_str, set_instr, self.to_8bit_reg(&dst_reg), dst_reg, self.to_8bit_reg(&dst_reg)
                ))
            }

            _ => Ok(format!("    ; Unimplemented: {:?}\n", instr)),
        }
    }

    fn get_physical_reg(
        &self,
        virtual_reg: usize,
        allocation: &HashMap<usize, PhysicalRegister>,
    ) -> Result<String, String> {
        allocation
            .get(&virtual_reg)
            .map(|reg| self.physical_reg_name(reg))
            .ok_or_else(|| format!("No allocation for register {}", virtual_reg))
    }

    fn physical_reg_name(&self, reg: &PhysicalRegister) -> String {
        match reg {
            PhysicalRegister::RAX => "rax".to_string(),
            PhysicalRegister::RBX => "rbx".to_string(),
            PhysicalRegister::RCX => "rcx".to_string(),
            PhysicalRegister::RDX => "rdx".to_string(),
            PhysicalRegister::RSI => "rsi".to_string(),
            PhysicalRegister::RDI => "rdi".to_string(),
            PhysicalRegister::R8 => "r8".to_string(),
            PhysicalRegister::R9 => "r9".to_string(),
            PhysicalRegister::R10 => "r10".to_string(),
            PhysicalRegister::R11 => "r11".to_string(),
            PhysicalRegister::R12 => "r12".to_string(),
            PhysicalRegister::R13 => "r13".to_string(),
            PhysicalRegister::R14 => "r14".to_string(),
            PhysicalRegister::R15 => "r15".to_string(),
            PhysicalRegister::Spilled(slot) => format!("qword ptr [rbp - {}]", (slot + 1) * 8),
        }
    }

    fn to_8bit_reg(&self, reg: &str) -> String {
        match reg {
            "rax" => "al",
            "rbx" => "bl",
            "rcx" => "cl",
            "rdx" => "dl",
            _ => "al",
        }
        .to_string()
    }

    fn format_operand_x86_64(
        &self,
        val: &IRValue,
        allocation: &HashMap<usize, PhysicalRegister>,
    ) -> Result<String, String> {
        match val {
            IRValue::Register(r) => self.get_physical_reg(*r, allocation),
            IRValue::Constant(c) => Ok(self.format_constant(c)),
            IRValue::Global(name) => Ok(format!("[{}]", name)),
            IRValue::Local(idx) => Ok(format!("qword ptr [rbp - {}]", (idx + 1) * 8)),
            IRValue::Argument(idx) => {
                // Arguments are in registers or stack
                let arg_regs = ["rdi", "rsi", "rdx", "rcx", "r8", "r9"];
                if *idx < arg_regs.len() {
                    Ok(arg_regs[*idx].to_string())
                } else {
                    Ok(format!("qword ptr [rbp + {}]", (idx - arg_regs.len() + 2) * 8))
                }
            }
            IRValue::Undef => Ok("0".to_string()),
        }
    }

    fn format_constant(&self, c: &IRConstant) -> String {
        match c {
            IRConstant::I8(n) => n.to_string(),
            IRConstant::I16(n) => n.to_string(),
            IRConstant::I32(n) => n.to_string(),
            IRConstant::I64(n) => n.to_string(),
            IRConstant::F32(f) => format!("{}", f.as_f32()),
            IRConstant::F64(f) => format!("{}", f.as_f64()),
            IRConstant::Bool(b) => if *b { "1" } else { "0" }.to_string(),
            IRConstant::String(s) => format!("\"{}\"", s),
            IRConstant::Null => "0".to_string(),
        }
    }

    fn generate_arm64(&mut self, _program: &IRProgram) -> Result<String, String> {
        // TODO: Implement ARM64 code generation
        Ok("/* ARM64 codegen not yet implemented */\n".to_string())
    }

    fn generate_wasm(&mut self, _program: &IRProgram) -> Result<String, String> {
        // TODO: Implement WebAssembly code generation
        Ok(";; WASM codegen not yet implemented\n".to_string())
    }

    fn generate_llvm_ir(&mut self, program: &IRProgram) -> Result<String, String> {
        let mut output = String::new();

        // LLVM IR header
        output.push_str("; ModuleID = 'magolor'\n");
        output.push_str("target datalayout = \"e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128\"\n");
        output.push_str("target triple = \"x86_64-pc-linux-gnu\"\n\n");

        // Generate LLVM IR for each function
        for (name, func) in &program.functions {
            output.push_str(&self.generate_function_llvm(name, func)?);
        }

        Ok(output)
    }

    fn generate_function_llvm(&self, name: &str, func: &IRFunction) -> Result<String, String> {
        let mut output = String::new();

        // Function signature
        let ret_ty = self.type_to_llvm(&func.return_type);
        let params: Vec<String> = func
            .params
            .iter()
            .map(|(_, ty)| self.type_to_llvm(ty))
            .collect();
        
        output.push_str(&format!(
            "define {} @{}({}) {{\n",
            ret_ty,
            name,
            params.join(", ")
        ));

        // Function body (simplified)
        for block in &func.blocks {
            output.push_str(&format!("{}:\n", block.id));
            for instr in &block.instructions {
                output.push_str("  ; ");
                output.push_str(&format!("{:?}\n", instr));
            }
        }

        output.push_str("}\n\n");
        Ok(output)
    }

    fn type_to_llvm(&self, ty: &IRType) -> String {
        match ty {
            IRType::I8 => "i8".to_string(),
            IRType::I16 => "i16".to_string(),
            IRType::I32 => "i32".to_string(),
            IRType::I64 => "i64".to_string(),
            IRType::F32 => "float".to_string(),
            IRType::F64 => "double".to_string(),
            IRType::Bool => "i1".to_string(),
            IRType::Ptr(inner) => format!("{}*", self.type_to_llvm(inner)),
            IRType::Void => "void".to_string(),
            _ => "i32".to_string(),
        }
    }
}
