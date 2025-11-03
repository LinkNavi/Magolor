#!/bin/bash

echo "🔍 Debugging Segmentation Fault"
echo "================================"
echo ""

# Check what the assembly looks like
echo "1. Checking generated main function:"
echo "---"
sed -n '/^main:/,/^[a-zA-Z]/p' hallo.s | head -30
echo "---"
echo ""

# Run with gdb to see where it crashes
echo "2. Running with GDB to find crash location:"
echo ""

if command -v gdb &> /dev/null; then
    # Create GDB commands
    cat > gdb_commands.txt << 'EOF'
run
backtrace
info registers
quit
EOF
    
    gdb -batch -x gdb_commands.txt ./program 2>&1 | head -50
    rm gdb_commands.txt
else
    echo "GDB not available, using objdump..."
    objdump -d ./program | grep -A 20 "<main>:"
fi

echo ""
echo "3. Checking for common issues in generated code:"
echo ""

# Check for uninitialized register usage
if grep -q "mov qword ptr \[rax\], 0" hallo.s; then
    echo "⚠️  Found: mov qword ptr [rax], 0"
    echo "   Problem: Using uninitialized RAX as pointer!"
    echo "   This will definitely segfault."
fi

# Check for bad memory access patterns
if grep -q "mov .*, \[0x" hallo.s; then
    echo "⚠️  Found: Suspicious memory access"
fi

echo ""
echo "4. Disassembling main to see actual instructions:"
objdump -d program -M intel | sed -n '/<main>:/,/<[^>]*>:/p' | head -40

echo ""
echo "================================"
echo "Analysis complete"