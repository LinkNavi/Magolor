#!/bin/bash

# Apply Magolor Compiler Fixes

echo "Applying fixes to MagolorCompiler..."

# Fix 1: Add is_power_of_two trait to loop_optimizer.rs
echo "Fixing loop_optimizer.rs..."
cat > /tmp/loop_fix.txt << 'EOF'
// src/modules/ir/loop_optimizer.rs
// Loop optimization passes: unrolling, invariant code motion, strength reduction

use crate::modules::ir::ir_types::*;
use std::collections::{HashMap, HashSet};

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
EOF

# Insert the trait after line 5 (after the imports)
head -n 5 src/modules/ir/loop_optimizer.rs > /tmp/loop_temp.rs
cat /tmp/loop_fix.txt | tail -n +6 >> /tmp/loop_temp.rs
tail -n +6 src/modules/ir/loop_optimizer.rs >> /tmp/loop_temp.rs
mv /tmp/loop_temp.rs src/modules/ir/loop_optimizer.rs

# Fix 2: Add is_power_of_two trait to peephole.rs
echo "Fixing peephole.rs..."
cat > /tmp/peephole_fix.txt << 'EOF'
// src/modules/ir/peephole.rs
// Peephole optimization - local instruction patterns

use crate::modules::ir::ir_types::*;

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
EOF

head -n 4 src/modules/ir/peephole.rs > /tmp/peephole_temp.rs
cat /tmp/peephole_fix.txt | tail -n +5 >> /tmp/peephole_temp.rs
tail -n +5 src/modules/ir/peephole.rs >> /tmp/peephole_temp.rs
mv /tmp/peephole_temp.rs src/modules/ir/peephole.rs

# Fix 3: Fix inline_optimizer.rs type annotation
echo "Fixing inline_optimizer.rs..."
sed -i 's/let mut new_blocks = Vec::new();/let mut new_blocks: Vec<IRBasicBlock> = Vec::new();/' src/modules/ir/inline_optimizer.rs

# Fix 4: Fix ir_builder.rs return_type clone
echo "Fixing ir_builder.rs..."
sed -i 's/return_type,$/return_type: return_type.clone(),/' src/modules/ir/ir_builder.rs
sed -i 's/if return_type == IRType::Void {/if ir_func.return_type == IRType::Void {/' src/modules/ir/ir_builder.rs

echo ""
echo "Automated fixes applied!"
echo ""
echo "MANUAL FIXES STILL NEEDED:"
echo "1. Copy the constant_folding.rs artifact to: src/modules/ir/constant_folding.rs"
echo "2. Replace src/modules/ir/ir_types.rs with the 'ir_types.rs (Fixed)' artifact"
echo "3. Replace src/modules/tokenizer.rs with the 'tokenizer.rs (Fixed)' artifact"
echo "4. Update src/modules/IR.rs to fix error conversions (see below)"
echo ""
echo "For IR.rs error conversion, change lines 43, 46, 50, 55, 59, 63, 68, 72, 76:"
echo "  FROM: .map_err(|e| anyhow::anyhow!(...))"
echo "  TO:   .map_err(|e| anyhow::Error::msg(format!(...)))"
echo ""
echo "OR alternatively, change all builder.build() and optimizer.optimize() to return"
echo "anyhow::Result instead of Result<T, String>"