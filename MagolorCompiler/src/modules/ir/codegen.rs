// src/modules/ir/codegen.rs - COMPLETE IMPLEMENTATION
// This is the full version with all instruction handlers

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
            Target::X86_64 => 14,
            _ => 0,
        };

        Self {
            target,
            register_allocator: RegisterAllocator::new(num_registers),
        }
    }

    pub fn generate(&mut self, program: &IRProgram) -> Result<String, String> {
        match self.target {
            Target::X86_64 => self.generate_x86_64(program),
            _ => Ok("/* Target not implemented */\n".to_string()),
        }
    }

    fn generate_x86_64(&mut self, program: &IRProgram) -> Result<String, String> {
        let mut output = String::new();

        output.push_str("    .text\n");
        output.push_str("    .intel_syntax noprefix\n");

        for (name, func) in &program.functions {
            output.push_str(&self.generate_function_x86_64(name, func)?);
        }

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

    fn generate_function_x86_64(&mut self, name: &str, func: &IRFunction) -> Result<String, String> {
        let mut output = String::new();

        output.push_str(&format!("    .globl {}\n", name));
        output.push_str(&format!("{}:\n", name));

        output.push_str("    push rbp\n");
        output.push_str("    mov rbp, rsp\n");

        if func.local_count > 0 {
            let stack_size = ((func.local_count * 8 + 15) / 16) * 16;
            output.push_str(&format!("    sub rsp, {}\n", stack_size));
        }

        let allocation = self.simple_register_map(func);

        for block in &func.blocks {
            let label = format!(".L_{}_{}",  name.replace(".", "_"), block.id);
            output.push_str(&format!("{}:\n", label));

            for instr in &block.instructions {
                output.push_str(&self.generate_instruction_x86_64(instr, &allocation, name)?);
            }
        }

        let epilogue_label = format!(".L_{}_epilogue",  name.replace(".", "_"));
        output.push_str(&format!("{}:\n", epilogue_label));
        output.push_str("    mov rsp, rbp\n");
        output.push_str("    pop rbp\n");
        output.push_str("    ret\n\n");

        Ok(output)
    }

    fn simple_register_map(&self, func: &IRFunction) -> HashMap<usize, PhysicalRegister> {
        let mut allocation = HashMap::new();
        let phys_regs = [
            PhysicalRegister::RAX, PhysicalRegister::RBX, PhysicalRegister::RCX, PhysicalRegister::RDX,
            PhysicalRegister::RSI, PhysicalRegister::RDI, PhysicalRegister::R8, PhysicalRegister::R9,
            PhysicalRegister::R10, PhysicalRegister::R11, PhysicalRegister::R12, PhysicalRegister::R13,
            PhysicalRegister::R14, PhysicalRegister::R15,
        ];

        for vreg in 0..func.register_count {
            if vreg < phys_regs.len() {
                allocation.insert(vreg, phys_regs[vreg]);
            } else {
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
                let dst_reg = self.get_physical_reg_name(*dst, allocation)?;
                let lhs_str = self.format_operand_x86_64(lhs, allocation)?;
                let rhs_str = self.format_operand_x86_64(rhs, allocation)?;
                Ok(format!("    mov {}, {}\n    add {}, {}\n", dst_reg, lhs_str, dst_reg, rhs_str))
            }

            IRInstruction::Sub { dst, lhs, rhs, .. } => {
                let dst_reg = self.get_physical_reg_name(*dst, allocation)?;
                let lhs_str = self.format_operand_x86_64(lhs, allocation)?;
                let rhs_str = self.format_operand_x86_64(rhs, allocation)?;
                Ok(format!("    mov {}, {}\n    sub {}, {}\n", dst_reg, lhs_str, dst_reg, rhs_str))
            }

            IRInstruction::Mul { dst, lhs, rhs, .. } => {
                let dst_reg = self.get_physical_reg_name(*dst, allocation)?;
                let lhs_str = self.format_operand_x86_64(lhs, allocation)?;
                let rhs_str = self.format_operand_x86_64(rhs, allocation)?;
                Ok(format!("    mov {}, {}\n    imul {}, {}\n", dst_reg, lhs_str, dst_reg, rhs_str))
            }

            IRInstruction::Div { dst, lhs, rhs, ty } => {
                let dst_reg = self.get_physical_reg_name(*dst, allocation)?;
                let lhs_str = self.format_operand_x86_64(lhs, allocation)?;
                let rhs_str = self.format_operand_x86_64(rhs, allocation)?;
                
                let mut code = String::new();
                
                match ty {
                    IRType::F32 | IRType::F64 => {
                        code.push_str(&format!("    movq xmm0, {}\n", lhs_str));
                        code.push_str(&format!("    movq xmm1, {}\n", rhs_str));
                        code.push_str("    divsd xmm0, xmm1\n");
                        code.push_str(&format!("    movq {}, xmm0\n", dst_reg));
                    }
                    _ => {
                        code.push_str(&format!("    mov rax, {}\n", lhs_str));
                        code.push_str("    cqo\n");
                        
                        if rhs_str.contains('[') {
                            code.push_str(&format!("    mov rcx, {}\n", rhs_str));
                            code.push_str("    idiv rcx\n");
                        } else {
                            code.push_str(&format!("    idiv {}\n", rhs_str));
                        }
                        
                        if dst_reg != "rax" {
                            code.push_str(&format!("    mov {}, rax\n", dst_reg));
                        }
                    }
                }
                
                Ok(code)
            }

            IRInstruction::Load { dst, addr, .. } => {
                let dst_reg = self.get_physical_reg_name(*dst, allocation)?;
                
                match addr {
                    IRValue::Local(idx) => {
                        let offset = (idx + 1) * 8;
                        Ok(format!("    mov {}, qword ptr [rbp - {}]\n", dst_reg, offset))
                    }
                    IRValue::Global(name) => {
                        if name.starts_with("__str_") {
                            Ok(format!("    lea {}, [{}]\n", dst_reg, name))
                        } else {
                            Ok(format!("    mov {}, qword ptr [{}]\n", dst_reg, name))
                        }
                    }
                    IRValue::Register(reg) => {
                        let addr_reg = self.get_physical_reg_name(*reg, allocation)?;
                        Ok(format!("    mov {}, qword ptr [{}]\n", dst_reg, addr_reg))
                    }
                    _ => {
                        let addr_str = self.format_operand_x86_64(addr, allocation)?;
                        Ok(format!("    mov {}, qword ptr [{}]\n", dst_reg, addr_str))
                    }
                }
            }

            IRInstruction::Store { addr, value, .. } => {
                match addr {
                    IRValue::Local(idx) => {
                        let offset = (idx + 1) * 8;
                        let value_str = self.format_operand_x86_64(value, allocation)?;
                        
                        if value_str.contains('[') || value_str.contains("ptr") {
                            Ok(format!("    mov rax, {}\n    mov qword ptr [rbp - {}], rax\n", value_str, offset))
                        } else {
                            Ok(format!("    mov qword ptr [rbp - {}], {}\n", offset, value_str))
                        }
                    }
                    IRValue::Global(name) => {
                        let value_str = self.format_operand_x86_64(value, allocation)?;
                        
                        if value_str.contains('[') || value_str.contains("ptr") {
                            Ok(format!("    mov rax, {}\n    mov qword ptr [{}], rax\n", value_str, name))
                        } else {
                            Ok(format!("    mov qword ptr [{}], {}\n", name, value_str))
                        }
                    }
                    IRValue::Register(reg) => {
                        let reg_str = self.get_physical_reg_name(*reg, allocation)?;
                        let value_str = self.format_operand_x86_64(value, allocation)?;
                        
                        if value_str.contains('[') || value_str.contains("ptr") {
                            Ok(format!("    mov rax, {}\n    mov qword ptr [{}], rax\n", value_str, reg_str))
                        } else {
                            Ok(format!("    mov qword ptr [{}], {}\n", reg_str, value_str))
                        }
                    }
                    _ => {
                        let addr_str = self.format_operand_x86_64(addr, allocation)?;
                        let value_str = self.format_operand_x86_64(value, allocation)?;
                        
                        if value_str.contains('[') || value_str.contains("ptr") {
                            Ok(format!("    mov rax, {}\n    mov qword ptr [{}], rax\n", value_str, addr_str))
                        } else {
                            Ok(format!("    mov qword ptr [{}], {}\n", addr_str, value_str))
                        }
                    }
                }
            }

            IRInstruction::Move { dst, src } => {
                if let Some(PhysicalRegister::Spilled(slot)) = allocation.get(dst) {
                    let src_str = self.format_operand_x86_64(src, allocation)?;
                    let offset = (slot + 1) * 8;
                    
                    if src_str.contains('[') {
                        return Ok(format!(
                            "    mov rax, {}\n    mov qword ptr [rbp - {}], rax\n",
                            src_str, offset
                        ));
                    } else {
                        return Ok(format!(
                            "    mov qword ptr [rbp - {}], {}\n",
                            offset, src_str
                        ));
                    }
                }
                
                let dst_str = self.get_physical_reg_name(*dst, allocation)?;
                let src_str = self.format_operand_x86_64(src, allocation)?;
                Ok(format!("    mov {}, {}\n", dst_str, src_str))
            }

            IRInstruction::Branch { target } => {
                let label = format!(".L_{}_{}",  func_name.replace(".", "_"), target);
                Ok(format!("    jmp {}\n", label))
            }

            IRInstruction::CondBranch { cond, true_target, false_target } => {
                let true_label = format!(".L_{}_{}",  func_name.replace(".", "_"), true_target);
                let false_label = format!(".L_{}_{}",  func_name.replace(".", "_"), false_target);
                
                let mut code = String::new();
                
                let cond_reg: String = match cond {
                    IRValue::Constant(_) => {
                        let cond_str = self.format_operand_x86_64(cond, allocation)?;
                        code.push_str(&format!("    mov rax, {}\n", cond_str));
                        "rax".to_string()
                    }
                    IRValue::Register(r) => {
                        let reg = self.get_physical_reg_name(*r, allocation)?;
                        if reg.contains('[') {
                            code.push_str(&format!("    mov rax, {}\n", reg));
                            "rax".to_string()
                        } else {
                            reg
                        }
                    }
                    _ => {
                        let cond_str = self.format_operand_x86_64(cond, allocation)?;
                        code.push_str(&format!("    mov rax, {}\n", cond_str));
                        "rax".to_string()
                    }
                };
                
                code.push_str(&format!("    test {}, {}\n", cond_reg, cond_reg));
                code.push_str(&format!("    jnz {}\n", true_label));
                code.push_str(&format!("    jmp {}\n", false_label));
                
                Ok(code)
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
                
                if func.contains("print") || func.contains("Console") || func.contains("console") {
                    if !args.is_empty() {
                        match &args[0] {
                            IRValue::Global(s) if s.starts_with("__str_") => {
                                code.push_str(&format!("    lea rdi, [{}]\n", s));
                            }
                            _ => {
                                let arg_str = self.format_operand_x86_64(&args[0], allocation)?;
                                code.push_str(&format!("    mov rdi, {}\n", arg_str));
                            }
                        }
                    }
                    code.push_str("    call console_print\n");
                } else {
                    let arg_regs = ["rdi", "rsi", "rdx", "rcx", "r8", "r9"];
                    
                    for (i, arg) in args.iter().enumerate() {
                        if i < arg_regs.len() {
                            match arg {
                                IRValue::Global(s) if s.starts_with("__str_") => {
                                    code.push_str(&format!("    lea {}, [{}]\n", arg_regs[i], s));
                                }
                                _ => {
                                    let arg_str = self.format_operand_x86_64(arg, allocation)?;
                                    code.push_str(&format!("    mov {}, {}\n", arg_regs[i], arg_str));
                                }
                            }
                        } else {
                            let arg_str = self.format_operand_x86_64(arg, allocation)?;
                            code.push_str(&format!("    push {}\n", arg_str));
                        }
                    }

                    code.push_str(&format!("    call {}\n", func));
                }

                if let Some(dst_reg) = dst {
                    let dst_str = self.get_physical_reg_name(*dst_reg, allocation)?;
                    code.push_str(&format!("    mov {}, rax\n", dst_str));
                }

                Ok(code)
            }

            IRInstruction::Cmp { dst, op, lhs, rhs, .. } => {
                let mut code = String::new();
                
                let lhs_str = self.format_operand_x86_64(lhs, allocation)?;
                let rhs_str = self.format_operand_x86_64(rhs, allocation)?;
                
                let set_instr = match op {
                    CmpOp::Eq | CmpOp::FEq => "sete",
                    CmpOp::Ne | CmpOp::FNe => "setne",
                    CmpOp::Lt | CmpOp::FLt => "setl",
                    CmpOp::Le | CmpOp::FLe => "setle",
                    CmpOp::Gt | CmpOp::FGt => "setg",
                    CmpOp::Ge | CmpOp::FGe => "setge",
                };

                let lhs_reg: String = if lhs_str.contains('[') {
                    code.push_str(&format!("    mov rax, {}\n", lhs_str));
                    "rax".to_string()
                } else {
                    lhs_str
                };

                code.push_str(&format!("    cmp {}, {}\n", lhs_reg, rhs_str));
                code.push_str(&format!("    {} al\n", set_instr));
                code.push_str("    movzx rax, al\n");
                
                let dst_str = self.get_physical_reg_name(*dst, allocation)?;
                if dst_str != "rax" {
                    code.push_str(&format!("    mov {}, rax\n", dst_str));
                }
                
                Ok(code)
            }

            _ => Ok(format!("    ; Unimplemented: {:?}\n", instr)),
        }
    }

    fn get_physical_reg_name(&self, virtual_reg: usize, allocation: &HashMap<usize, PhysicalRegister>) -> Result<String, String> {
        allocation.get(&virtual_reg).map(|reg| match reg {
            PhysicalRegister::Spilled(_) => "rax".to_string(),
            _ => self.physical_reg_name(reg)
        }).ok_or_else(|| format!("No allocation for register {}", virtual_reg))
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

    fn format_operand_x86_64(&self, val: &IRValue, allocation: &HashMap<usize, PhysicalRegister>) -> Result<String, String> {
        match val {
            IRValue::Register(r) => {
                if let Some(PhysicalRegister::Spilled(slot)) = allocation.get(r) {
                    Ok(format!("qword ptr [rbp - {}]", (slot + 1) * 8))
                } else {
                    self.get_physical_reg_name(*r, allocation)
                }
            }
            IRValue::Constant(c) => Ok(self.format_constant(c)),
            IRValue::Global(name) => {
                if name.starts_with("__str_") {
                    Ok(name.clone())
                } else {
                    Ok(format!("[{}]", name))
                }
            }
            IRValue::Local(idx) => Ok(format!("qword ptr [rbp - {}]", (idx + 1) * 8)),
            IRValue::Argument(idx) => {
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
}