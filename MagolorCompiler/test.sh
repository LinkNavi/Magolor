#!/bin/bash

echo "🚀 Testing All Features"
echo "======================="
echo ""

# Step 1: Replace ir_builder.rs
echo "Step 1: Backing up and replacing ir_builder.rs..."
cp src/modules/ir/ir_builder.rs src/modules/ir/ir_builder.rs.old
echo "   Copy the 'Complete Fix - IR Builder with All Features' artifact"
echo "   to: src/modules/ir/ir_builder.rs"
echo ""
read -p "Press Enter when ready..."

# Step 2: Rebuild
echo ""
echo "Step 2: Rebuilding compiler..."
cargo build 2>&1 | grep -E "error" && {
    echo "❌ Build failed!"
    exit 1
}
echo "   ✅ Built successfully"

# Step 3: Test with full example
echo ""
echo "Step 3: Testing with full example (including factorial)..."
cargo run --quiet hallo.mg 2>&1 | tail -5

# Step 4: Check factorial assembly
echo ""
echo "Step 4: Checking factorial assembly..."
echo "   Looking for the multiplication with saved value:"
if grep -A 35 "Calculator.MathOperations.factorial:" hallo.s | grep -E "Load.*Local|Store.*Local.*Register" > /dev/null; then
    echo "   ✅ Found local variable storage (register preservation)"
else
    echo "   ⚠️  No explicit preservation found, checking pattern..."
fi

echo ""
echo "   Factorial assembly:"
grep -A 35 "Calculator.MathOperations.factorial:" hallo.s | head -40

# Step 5: Build and run
echo ""
echo "Step 5: Building executable..."
gcc -c hallo.s -o hallo.o 2>&1 | head -3
gcc hallo.o runtime.o -o program -no-pie -lm 2>&1 | head -3

if [ $? -eq 0 ]; then
    echo "   ✅ Executable created"
else
    echo "   ❌ Linking failed"
    exit 1
fi

# Step 6: Run!
echo ""
echo "======================================"
echo "🎉 RUNNING PROGRAM"
echo "======================================"
echo ""
./program
EXIT_CODE=$?

echo ""
echo "======================================"
if [ $EXIT_CODE -eq 0 ]; then
    echo "✅ Program completed successfully!"
    echo ""
    echo "Expected output:"
    echo "  5! = 120  (should be 120, not 1)"
    echo "  Wednesday (from match)"
    echo "  Total calls: 3"
else
    echo "⚠️  Exit code: $EXIT_CODE"
fi
echo "======================================"