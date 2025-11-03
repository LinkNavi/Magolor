#!/usr/bin/env python3
"""Fix the variable allocation bug in ir_builder.rs"""

import re

filepath = "src/modules/ir/ir_builder.rs"

print("Reading ir_builder.rs...")
with open(filepath, 'r') as f:
    content = f.read()

# Find and replace the VarDecl handling
old_pattern = r'''Statement::VarDecl \{ name, var_type, value, is_mutable: _ \} => \{
\s+let ty = self\.convert_type\(&var_type\);
\s+let _local_idx = self\.local_counter;
\s+self\.local_counter \+= 1;

\s+let reg = self\.next_register\(\);
\s+self\.emit_instruction\(func, IRInstruction::Alloca \{
\s+dst: reg,
\s+ty: ty\.clone\(\),
\s+count: None,
\s+\}\);

\s+if let Some\(val\) = value \{
\s+let val_reg = self\.build_expression\(func, val\)\?;
\s+self\.emit_instruction\(func, IRInstruction::Store \{
\s+addr: IRValue::Register\(reg\),
\s+value: val_reg,
\s+ty: ty\.clone\(\),
\s+\}\);
\s+\}

\s+self\.symbol_table\.last_mut\(\)\.unwrap\(\)
\s+\.insert\(name, \(IRValue::Register\(reg\), ty\)\);
\s+Ok\(\(\)\)
\s+\}'''

new_code = '''Statement::VarDecl { name, var_type, value, is_mutable: _ } => {
                let ty = self.convert_type(&var_type);
                let local_idx = self.local_counter;
                self.local_counter += 1;

                // Use Local instead of a register for the address
                let addr = IRValue::Local(local_idx);
                
                if let Some(val) = value {
                    let val_reg = self.build_expression(func, val)?;
                    self.emit_instruction(func, IRInstruction::Store {
                        addr: addr.clone(),
                        value: val_reg,
                        ty: ty.clone(),
                    });
                }

                // Register the variable as a Local slot
                self.symbol_table.last_mut().unwrap()
                    .insert(name, (addr, ty));
                Ok(())
            }'''

print("Replacing VarDecl handling...")
content = re.sub(old_pattern, new_code, content, flags=re.DOTALL)

# Also need to fix the VarRef loading to handle Local properly
print("Checking VarRef handling...")
# It should already handle IRValue::Local, but let's make sure

print("Writing fixed file...")
with open(filepath, 'w') as f:
    f.write(content)

print("✓ Fixed!")
print("\nRebuild and test:")
print("  cargo build")
print("  cargo run test.mg")
print("  gcc -c test.s -o test.o && gcc test.o runtime.o -o program && ./program")