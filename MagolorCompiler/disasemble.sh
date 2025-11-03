#!/bin/bash

echo "🔍 Debugging Assembly Output"
echo "============================"
echo ""

# Check the output.s file
echo "1. Checking output.s file:"
if [ -f output.s ]; then
    echo "   Size: $(wc -c < output.s) bytes"
    echo "   Lines: $(wc -l < output.s) lines"
    echo ""
    echo "2. First 50 lines of output.s:"
    echo "---"
    head -50 output.s
    echo "---"
    echo ""
    echo "3. Looking for .globl directives (exported functions):"
    grep "\.globl" output.s || echo "   ⚠️  No .globl directives found!"
    echo ""
    echo "4. Looking for function labels:"
    grep "^[a-zA-Z_][a-zA-Z0-9._]*:" output.s | head -10 || echo "   ⚠️  No function labels found!"
    echo ""
else
    echo "   ❌ output.s not found!"
    echo ""
    echo "   Regenerating..."
    cargo run hallo.mg 2>&1 | tail -100 > full_output.txt
    
    # Try to extract just the assembly
    if grep -q "\.text" full_output.txt; then
        grep -A 1000 "\.text" full_output.txt > output.s
        echo "   ✓ Regenerated output.s"
        echo ""
        echo "   First 30 lines:"
        head -30 output.s
    else
        echo "   ❌ No assembly found in output"
        echo "   Showing last 50 lines of compiler output:"
        tail -50 full_output.txt
    fi
fi

echo ""
echo "5. Checking object file:"
file output.o
hexdump -C output.o | head -20

echo ""
echo "6. Trying to disassemble:"
objdump -d output.o 2>&1 | head -20

echo ""
echo "============================"
echo "Diagnosis complete"