#!/bin/bash

echo "Fixing codegen.rs completely..."

# Fix the block labels to be unique
sed -i 's/output\.push_str(&format!("\.L{}:\\n", block\.id));/let label = format!(".L_{}_{}",  name.replace(".", "_"), block.id);\n        output.push_str(\&format!("{}:\\n", label));/g' src/modules/ir/codegen.rs

# Fix the epilogue label format string error (line 114-115)
sed -i 's/output\.push_str(&format!("{}:\n", epilogue_label));/output.push_str(\&format!("{}:\\n", epilogue_label));/g' src/modules/ir/codegen.rs

# Check if output.s exists and has Unicode issues
if [ -f output.s ]; then
    echo "Checking for Unicode characters in output.s..."
    if file output.s | grep -q "UTF-8"; then
        echo "Converting output.s to ASCII..."
        iconv -f UTF-8 -t ASCII//TRANSLIT output.s -o output.s.tmp && mv output.s.tmp output.s
    fi
fi

echo "✅ Fixed!"
echo ""
echo "Rebuilding..."
cargo build

if [ $? -eq 0 ]; then
    echo ""
    echo "✅ Build successful!"
    echo ""
    echo "Regenerating assembly..."
    cargo run hallo.mg 2>&1 | grep -v "Compiling\|Finished\|Running" > hallo_full.txt
    
    # Extract just the assembly (after the last debug output)
    tail -1 hallo_full.txt > output.s
    
    echo "Testing assembly..."
    gcc -c output.s -o output.o 2>&1 | head -20
    
    if [ $? -eq 0 ]; then
        echo ""
        echo "✅✅✅ SUCCESS! Assembly compiled!"
        nm output.o | grep " T "
    else
        echo ""
        echo "Showing first 30 lines of output.s:"
        head -30 output.s
    fi
else
    echo "❌ Build failed"
fi