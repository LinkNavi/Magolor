#!/bin/bash

set -e  # Exit on error

echo "🚀 Building and Running Magolor Program"
echo "========================================"
echo ""

# Step 1: Compile Magolor to assembly
echo "Step 1: Compiling hallo.mg to assembly..."
cargo run --quiet hallo.mg 2>&1 | grep -v "Compiling\|Finished\|Running" >&2

if [ ! -f hallo.s ]; then
    echo "❌ hallo.s not created"
    exit 1
fi

echo "✅ Generated hallo.s ($(wc -l < hallo.s) lines)"
echo ""

# Step 2: Assemble the Magolor code
echo "Step 2: Assembling hallo.s..."
gcc -c hallo.s -o hallo.o

if [ $? -ne 0 ]; then
    echo "❌ Assembly failed"
    exit 1
fi

echo "✅ Created hallo.o"
echo ""

# Step 3: Compile runtime
echo "Step 3: Compiling runtime library..."
if [ ! -f runtime.c ]; then
    echo "❌ runtime.c not found! Create it first."
    echo "See the 'runtime.c - Complete Runtime Library' artifact"
    exit 1
fi

gcc -c runtime.c -o runtime.o -lm

if [ $? -ne 0 ]; then
    echo "❌ Runtime compilation failed"
    exit 1
fi

echo "✅ Created runtime.o"
echo ""

# Step 4: Link everything together
echo "Step 4: Linking..."

# Try different linking approaches
# Approach 1: Standard linking
gcc hallo.o runtime.o -o program -no-pie -lm 2>/dev/null

if [ $? -eq 0 ]; then
    echo "✅ Created executable: ./program"
    echo ""
    echo "========================================"
    echo "🎉 SUCCESS! Running program..."
    echo "========================================"
    echo ""
    ./program
    echo ""
    echo "========================================"
    echo "✅ Program executed successfully!"
else
    # Approach 2: Link with verbose to see issues
    echo "⚠️  Standard linking failed, trying verbose..."
    gcc hallo.o runtime.o -o program -no-pie -lm -v
    
    if [ $? -eq 0 ]; then
        echo ""
        echo "✅ Created executable: ./program"
        echo ""
        echo "Running..."
        ./program
    else
        echo ""
        echo "❌ Linking failed. This is likely because:"
        echo "   1. Your functions need mangling/name adjustment"
        echo "   2. Missing runtime functions"
        echo "   3. ABI mismatch"
        echo ""
        echo "Checking what's in hallo.o:"
        nm hallo.o || readelf -s hallo.o
        echo ""
        echo "Checking for undefined symbols:"
        nm -u hallo.o || readelf -s hallo.o | grep UND
    fi
fi

echo ""
echo "========================================"
echo "Build process complete!"