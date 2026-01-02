#!/bin/bash
# Test script for Gear multi-file project support

set -e

echo "╔════════════════════════════════════════╗"
echo "║   Gear Multi-File Project Test        ║"
echo "╚════════════════════════════════════════╝"
echo ""

# Check if gear and magolor are installed
if ! command -v gear &> /dev/null; then
    echo "❌ Error: 'gear' command not found"
    echo "Please install Gear first"
    exit 1
fi

if ! command -v magolor &> /dev/null; then
    echo "❌ Error: 'magolor' command not found"
    echo "Please install the Magolor compiler first"
    exit 1
fi

# Clean up any existing test project
rm -rf test-multifile-project
echo "🧹 Cleaned up previous test"

# ============================================================================
# TEST 1: Create a new multi-file project
# ============================================================================
echo ""
echo "═══════════════════════════════════════"
echo "TEST 1: Multi-File Project Creation"
echo "═══════════════════════════════════════"
echo ""

gear new test-multifile-project
cd test-multifile-project

echo "✅ Project created with structure:"
tree -L 3 || find . -type f -o -type d | sed 's|[^/]*/|  |g'

# ============================================================================
# TEST 2: Add more modules
# ============================================================================
echo ""
echo "═══════════════════════════════════════"
echo "TEST 2: Adding Custom Modules"
echo "═══════════════════════════════════════"
echo ""

# Create a math module
mkdir -p src/modules/math
cat > src/modules/math/operations.mg << 'EOF'
using Std.IO;

pub fn add(a: int, b: int) -> int {
    Std.print("math.operations: adding\n");
    return a + b;
}

pub fn multiply(a: int, b: int) -> int {
    Std.print("math.operations: multiplying\n");
    return a * b;
}

pub fn factorial(n: int) -> int {
    if (n <= 1) {
        return 1;
    }
    return multiply(n, factorial(n - 1));
}
EOF

# Create a data module
mkdir -p src/modules/data
cat > src/modules/data/processor.mg << 'EOF'
using Std.IO;
using modules.math.operations;

pub fn processData(x: int, y: int) -> int {
    Std.print("data.processor: processing\n");
    let sum = add(x, y);
    let product = multiply(x, y);
    return sum + product;
}

pub fn analyze(values: int) -> int {
    Std.print($"data.processor: analyzing {values}\n");
    return factorial(values);
}
EOF

# Update main.mg to use the new modules
cat > src/main.mg << 'EOF'
using Std.IO;
using modules.utils;
using modules.math.operations;
using modules.data.processor;

fn main() {
    Std.print("╔═══════════════════════════════════╗\n");
    Std.print("║  Multi-File Project Test          ║\n");
    Std.print("╚═══════════════════════════════════╝\n");
    Std.print("\n");
    
    greet("Multi-File System");
    Std.print("\n");
    
    // Test basic math operations
    Std.print("Testing math operations:\n");
    let sum = add(10, 5);
    Std.print($"10 + 5 = {sum}\n");
    
    let product = multiply(7, 6);
    Std.print($"7 * 6 = {product}\n");
    
    let fact = factorial(5);
    Std.print($"5! = {fact}\n");
    
    Std.print("\n");
    
    // Test data processing
    Std.print("Testing data processing:\n");
    let result = processData(8, 3);
    Std.print($"Process result: {result}\n");
    
    let analysis = analyze(4);
    Std.print($"Analysis result: {analysis}\n");
    
    Std.print("\n");
    Std.print("✅ All multi-file tests passed!\n");
}
EOF

echo "✅ Created additional modules:"
echo "   - src/modules/math/operations.mg"
echo "   - src/modules/data/processor.mg"

# ============================================================================
# TEST 3: Build the multi-file project
# ============================================================================
echo ""
echo "═══════════════════════════════════════"
echo "TEST 3: Building Multi-File Project"
echo "═══════════════════════════════════════"
echo ""

echo "📦 Building project with multiple files..."
gear build --verbose

if [ $? -eq 0 ]; then
    echo "✅ Build successful!"
else
    echo "❌ Build failed"
    exit 1
fi

# ============================================================================
# TEST 4: Run the multi-file project
# ============================================================================
echo ""
echo "═══════════════════════════════════════"
echo "TEST 4: Running Multi-File Project"
echo "═══════════════════════════════════════"
echo ""

gear run

if [ $? -eq 0 ]; then
    echo ""
    echo "✅ Run successful!"
else
    echo "❌ Run failed"
    exit 1
fi

# ============================================================================
# TEST 5: Test deeply nested modules
# ============================================================================
echo ""
echo "═══════════════════════════════════════"
echo "TEST 5: Deeply Nested Modules"
echo "═══════════════════════════════════════"
echo ""

mkdir -p src/modules/advanced/algorithms
cat > src/modules/advanced/algorithms/sorting.mg << 'EOF'
using Std.IO;

pub fn bubbleSort(arr: int) -> int {
    Std.print("algorithms.sorting: bubble sort called\n");
    return arr;
}
EOF

# Update main to import deeply nested module
cat > src/main.mg << 'EOF'
using Std.IO;
using modules.utils;
using modules.math.operations;
using modules.data.processor;
using modules.advanced.algorithms.sorting;

fn main() {
    Std.print("╔═══════════════════════════════════╗\n");
    Std.print("║  Multi-File Project Test          ║\n");
    Std.print("╚═══════════════════════════════════╝\n");
    Std.print("\n");
    
    greet("Multi-File System");
    Std.print("\n");
    
    // Test nested modules
    Std.print("Testing deeply nested modules:\n");
    let sorted = bubbleSort(42);
    Std.print($"Sorted result: {sorted}\n");
    
    Std.print("\n");
    
    // Test other modules
    let sum = add(10, 5);
    Std.print($"10 + 5 = {sum}\n");
    
    let result = processData(8, 3);
    Std.print($"Process result: {result}\n");
    
    Std.print("\n");
    Std.print("✅ All multi-file tests passed!\n");
}
EOF

echo "📦 Rebuilding with deeply nested modules..."
gear build --verbose

if [ $? -eq 0 ]; then
    echo "✅ Build with nested modules successful!"
else
    echo "❌ Build failed"
    exit 1
fi

echo ""
echo "🚀 Running with nested modules..."
gear run

cd ..

# ============================================================================
# Summary
# ============================================================================
echo ""
echo "╔════════════════════════════════════════╗"
echo "║           TEST SUMMARY                 ║"
echo "╚════════════════════════════════════════╝"
echo ""
echo "✅ Test 1: Multi-file project creation - PASSED"
echo "✅ Test 2: Adding custom modules - PASSED"
echo "✅ Test 3: Building multi-file project - PASSED"
echo "✅ Test 4: Running multi-file project - PASSED"
echo "✅ Test 5: Deeply nested modules - PASSED"
echo ""
echo "All multi-file project tests passed! 🎉"
echo ""
echo "Project structure created:"
echo "  test-multifile-project/"
echo "  ├── src/"
echo "  │   ├── main.mg"
echo "  │   └── modules/"
echo "  │       ├── utils.mg"
echo "  │       ├── math/"
echo "  │       │   └── operations.mg"
echo "  │       ├── data/"
echo "  │       │   └── processor.mg"
echo "  │       └── advanced/"
echo "  │           └── algorithms/"
echo "  │               └── sorting.mg"
echo "  └── target/"
echo "      └── test-multifile-project (executable)"
echo ""
echo "Clean up with: rm -rf test-multifile-project"
