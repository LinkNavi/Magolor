#!/bin/bash
# Quick fix script - restore original ir_builder.rs

echo "Restoring original ir_builder.rs from git..."

# Check if we have the original
if git show HEAD:src/modules/ir/ir_builder.rs > /tmp/ir_builder_backup.rs 2>/dev/null; then
    echo "✓ Found original in git"
    cp /tmp/ir_builder_backup.rs src/modules/ir/ir_builder.rs
    echo "✓ Restored original ir_builder.rs"
else
    echo "❌ Could not find original in git"
    echo ""
    echo "Manual fix needed:"
    echo "The ir_builder.rs file needs to have a complete IRBuilder implementation."
    echo "The artifact I provided was incomplete."
    echo ""
    echo "Solution: Keep your ORIGINAL ir_builder.rs file from document index 24"
    echo "Only replace codegen.rs (document index 19) with the fixed version"
fi

echo ""
echo "Now update codegen.rs with the lifetime fixes..."
echo "Replace src/modules/ir/codegen.rs with the 'codegen_fixed' artifact"