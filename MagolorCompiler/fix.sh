#!/bin/bash

echo "🔍 Checking runtime and execution"
echo "================================="
echo ""

# Check if runtime has the correct functions
echo "1. Checking runtime.o symbols:"
nm runtime.o | grep -E "console_print|string_concat"
echo ""

# Check what strings are in the program
echo "2. Checking string literals in program:"
strings program | head -20
echo ""

# Look at the actual assembly for the factorial call site
echo "3. Looking at main() disassembly around factorial:"
objdump -d program -M intel | sed -n '/<main>/,/<[^>]*>:/p' | tail -50
echo ""

# The issue might be statement ordering
echo "4. Checking the order of operations in hallo.mg:"
echo "---"
cat hallo.mg | grep -A 2 -B 2 "factorial"
echo "---"
echo ""

# Create a test with explicit ordering
echo "5. Creating test with explicit order:"
cat > test_order.mg << 'EOF'
use Console;

namespace Calculator {
    public class MathOperations {
        private static i32 callCount = 0;
        
        public static i32 fn add(i32 a, i32 b) {
            callCount++;
            return a + b;
        }
        
        public static i32 fn factorial(i32 n) {
            if (n <= 1) {
                return 1;
            }
            return n * factorial(n - 1);
        }
        
        public static i32 fn getCallCount() {
            return callCount;
        }
    }
}

void fn main() {
    console.print("=== Calculator Demo ===");
    
    let result1 = Calculator.MathOperations.add(5, 3);
    console.print("5 + 3 = " + result1);
    
    console.print("About to call factorial...");
    let fact = Calculator.MathOperations.factorial(5);
    console.print("Factorial computed");
    console.print("5! = " + fact);
    
    console.print("Done!");
}
EOF

echo "6. Compiling test_order.mg:"
cargo run --quiet test_order.mg 2>&1 | tail -1

if [ -f test_order.s ]; then
    gcc -c test_order.s -o test_order.o && \
    gcc test_order.o runtime.o -o test_order -no-pie -lm && \
    echo "" && \
    echo "7. Running test_order:" && \
    echo "---" && \
    ./test_order && \
    echo "---"
else
    echo "❌ Compilation failed"
fi

echo ""
echo "8. If factorial still shows wrong value, check the assembly:"
if [ -f test_order.s ]; then
    echo "Looking for imul in factorial:"
    grep -A 30 "Calculator.MathOperations.factorial:" test_order.s | grep -E "(imul|mul|call.*factorial)"
fi