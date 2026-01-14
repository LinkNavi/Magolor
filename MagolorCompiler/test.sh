#!/bin/bash
# debug_magolor.sh - Debug Magolor compiler segfault

echo "=========================================="
echo "Debugging Magolor Compiler Segfault"
echo "=========================================="
echo ""

# Check if gdb is available
if ! command -v gdb &> /dev/null; then
    echo "Error: GDB is not installed"
    echo "Install with: sudo apt install gdb"
    exit 1
fi

# Check if compiler exists
if [ ! -f "./target/dev/Magolor" ]; then
    echo "Error: Compiler not found at ./target/dev/Magolor"
    exit 1
fi

# Check if test file exists
if [ ! -f "Examples/test_sockets.mg" ]; then
    echo "Error: Test file not found"
    exit 1
fi

echo "Running GDB on Magolor compiler..."
echo ""

# Run with GDB
gdb --batch \
    --command=/dev/stdin \
    --args ./target/dev/Magolor run Examples/test_sockets.mg <<EOF
# Enable pagination off for batch mode
set pagination off
set print pretty on

# Catch segfault
catch signal SIGSEGV

# Run the program
run

# When it crashes, show information
echo \n========== CRASH INFORMATION ==========\n

# Show backtrace with full details
echo \n--- Stack Trace ---\n
backtrace full

# Show the current frame
echo \n--- Current Frame ---\n
frame 0
info frame

# Show registers
echo \n--- Registers ---\n
info registers

# Show the faulting instruction
echo \n--- Faulting Instruction ---\n
x/5i \$pc-10

# Show local variables
echo \n--- Local Variables ---\n
info locals

# Show arguments
echo \n--- Function Arguments ---\n
info args

# Try to show source code
echo \n--- Source Code ---\n
list

# Show all threads
echo \n--- Threads ---\n
info threads

# Show memory at crash location
echo \n--- Memory at crash ---\n
x/10x \$rsp

quit
EOF

echo ""
echo "=========================================="
echo "GDB session complete"
echo "=========================================="
