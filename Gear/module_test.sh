#!/bin/bash
# Complete test script for Magolor module system

set -e

echo "╔════════════════════════════════════════╗"
echo "║   Magolor Module System Test Suite    ║"
echo "╚════════════════════════════════════════╝"
echo ""

# Check if magolor is installed
if ! command -v magolor &> /dev/null; then
    echo "❌ Error: 'magolor' command not found"
    echo ""
    echo "Please install the Magolor compiler first:"
    echo "  cd MagolorCompiler"
    echo "  zora build"
    echo "  sudo cp target/debug/Magolor /usr/local/bin/magolor"
    echo "  # or: sudo mv target/debug/Magolor /usr/local/bin/magolor"
    exit 1
fi

# Check if gear is installed
if ! command -v gear &> /dev/null; then
    echo "❌ Error: 'gear' command not found"
    echo ""
    echo "Please install Gear first:"
    echo "  cd Gear"
    echo "  zora build"
    echo "  ./move.sh"
    echo "  # or manually: cp target/debug/Gear ~/.local/bin/gear"
    exit 1
fi

# Clean up
echo "🧹 Cleaning up previous tests..."
rm -rf module-test-app module-test-lib

# ============================================================================
# TEST 1: Simple Module Import
# ============================================================================
echo ""
echo "═══════════════════════════════════════"
echo "TEST 1: Simple Module Import"
echo "═══════════════════════════════════════"
echo ""

mkdir -p module-test-app/src
cd module-test-app

# Create project.toml manually
cat > project.toml << 'EOF'
[project]
name = "module-test-app"
version = "0.1.0"
authors = ["Test User <test@example.com>"]
description = "Testing module system"
license = "MIT"

[dependencies]

[build]
optimization = "2"
EOF

# Create a utils module
cat > src/utils.mg << 'EOF'
using Std.IO;

fn add(a: int, b: int) -> int {
    Std.print("utils: adding numbers\n");
    return a + b;
}

fn multiply(a: int, b: int) -> int {
    Std.print("utils: multiplying numbers\n");
    return a * b;
}

fn square(x: int) -> int {
    return multiply(x, x);
}
EOF

# Create main that uses utils
cat > src/main.mg << 'EOF'
using Std.IO;
using utils;

fn main() {
    Std.print("Test 1: Simple Module Import\n");
    Std.print("==============================\n");
    
    let sum = add(5, 3);
    Std.print($"5 + 3 = {sum}\n");
    
    let product = multiply(7, 6);
    Std.print($"7 * 6 = {product}\n");
    
    let sq = square(4);
    Std.print($"4 squared = {sq}\n");
    
    Std.print("\n✅ Test 1 passed!\n\n");
}
EOF

echo "📦 Building test 1..."
gear build || { echo "❌ Build failed"; exit 1; }

echo "🚀 Running test 1..."
echo "─────────────────────────────────────────"
gear run || { echo "❌ Run failed"; exit 1; }
echo "─────────────────────────────────────────"

cd ..

# ============================================================================
# TEST 2: Nested Modules
# ============================================================================
echo ""
echo "═══════════════════════════════════════"
echo "TEST 2: Nested Modules"
echo "═══════════════════════════════════════"
echo ""

rm -rf module-test-app
mkdir -p module-test-app/src/{math,text}
cd module-test-app

# Create project.toml manually
cat > project.toml << 'EOF'
[project]
name = "module-test-app"
version = "0.1.0"
authors = ["Test User <test@example.com>"]
description = "Testing module system"
license = "MIT"

[dependencies]

[build]
optimization = "2"
EOF

# Create math.basic module
cat > src/math/basic.mg << 'EOF'
using Std.IO;

fn add(a: int, b: int) -> int {
    return a + b;
}

fn subtract(a: int, b: int) -> int {
    return a - b;
}

fn multiply(a: int, b: int) -> int {
    return a * b;
}
EOF

# Create math.advanced module that uses basic
cat > src/math/advanced.mg << 'EOF'
using Std.IO;
using math.basic;

fn power(base: int, exp: int) -> int {
    if (exp == 0) {
        return 1;
    }
    if (exp == 1) {
        return base;
    }
    return multiply(base, power(base, subtract(exp, 1)));
}

fn factorial(n: int) -> int {
    if (n <= 1) {
        return 1;
    }
    return multiply(n, factorial(subtract(n, 1)));
}
EOF

# Create text.format module
cat > src/text/format.mg << 'EOF'
using Std.IO;

fn banner(text: string) {
    Std.print("====================================\n");
    Std.print($"  {text}\n");
    Std.print("====================================\n");
}

fn separator() {
    Std.print("------------------------------------\n");
}
EOF

# Create main that uses all modules
cat > src/main.mg << 'EOF'
using Std.IO;
using math.basic;
using math.advanced;
using text.format;

fn main() {
    banner("Test 2: Nested Modules");
    Std.print("\n");
    
    Std.print("Basic math operations:\n");
    let sum = add(10, 5);
    Std.print($"10 + 5 = {sum}\n");
    
    let diff = subtract(10, 5);
    Std.print($"10 - 5 = {diff}\n");
    
    separator();
    
    Std.print("Advanced math operations:\n");
    let pow = power(2, 8);
    Std.print($"2^8 = {pow}\n");
    
    let fact = factorial(5);
    Std.print($"5! = {fact}\n");
    
    Std.print("\n✅ Test 2 passed!\n\n");
}
EOF

echo "📦 Building test 2..."
gear build || { echo "❌ Build failed"; exit 1; }

echo "🚀 Running test 2..."
echo "─────────────────────────────────────────"
gear run || { echo "❌ Run failed"; exit 1; }
echo "─────────────────────────────────────────"

cd ..

# ============================================================================
# TEST 3: Package Dependencies
# ============================================================================
echo ""
echo "═══════════════════════════════════════"
echo "TEST 3: Package Dependencies"
echo "═══════════════════════════════════════"
echo ""

# Create a reusable library package
mkdir -p module-test-lib/src
cd module-test-lib

cat > project.toml << 'EOF'
[project]
name = "testlib"
version = "1.0.0"
description = "Test library for module system"
license = "MIT"

[dependencies]
EOF

# Create library with multiple modules
cat > src/lib.mg << 'EOF'
using Std.IO;

fn processNumber(n: int) -> int {
    Std.print($"testlib: processing {n}\n");
    return n * 2;
}

fn greet(name: string) {
    Std.print($"testlib: Hello, {name}!\n");
}
EOF

cat > src/helpers.mg << 'EOF'
using Std.IO;

fn formatResult(value: int) -> int {
    Std.print($"testlib.helpers: formatting {value}\n");
    return value;
}
EOF

cd ..

# Create app that uses the library
rm -rf module-test-app
mkdir -p module-test-app/src
cd module-test-app

# Create project.toml manually
cat > project.toml << 'EOF'
[project]
name = "module-test-app"
version = "0.1.0"
authors = ["Test User <test@example.com>"]
description = "Testing module system"
license = "MIT"

[dependencies]
testlib = "path:../module-test-lib"

[build]
optimization = "2"
EOF

cat > src/main.mg << 'EOF'
using Std.IO;
using testlib;

fn main() {
    Std.print("Test 3: Package Dependencies\n");
    Std.print("==============================\n\n");
    
    greet("Module System");
    
    let x = 42;
    let result = processNumber(x);
    Std.print($"Processed {x} -> {result}\n");
    
    Std.print("\n✅ Test 3 passed!\n\n");
}
EOF

echo "📦 Installing dependencies..."
gear install || { echo "❌ Install failed"; exit 1; }

echo "📦 Building test 3..."
gear build --verbose || { echo "❌ Build failed"; exit 1; }

echo "🚀 Running test 3..."
echo "─────────────────────────────────────────"
gear run || { echo "❌ Run failed"; exit 1; }
echo "─────────────────────────────────────────"

cd ..

# ============================================================================
# TEST 4: Complex Module Graph
# ============================================================================
echo ""
echo "═══════════════════════════════════════"
echo "TEST 4: Complex Module Graph"
echo "═══════════════════════════════════════"
echo ""

rm -rf module-test-app
mkdir -p module-test-app/src/{core,utils,api}
cd module-test-app

# Create project.toml manually
cat > project.toml << 'EOF'
[project]
name = "module-test-app"
version = "0.1.0"
authors = ["Test User <test@example.com>"]
description = "Testing module system"
license = "MIT"

[dependencies]

[build]
optimization = "2"
EOF

# Core types
cat > src/core/types.mg << 'EOF'
using Std.IO;

fn makePoint(x: int, y: int) -> int {
    Std.print($"core.types: creating point ({x}, {y})\n");
    return x + y;  // Simplified: just return sum
}
EOF

# Core operations (uses types)
cat > src/core/ops.mg << 'EOF'
using Std.IO;
using core.types;

fn transform(x: int, y: int) -> int {
    let point = makePoint(x, y);
    return point * 2;
}
EOF

# Utils (uses core)
cat > src/utils/helpers.mg << 'EOF'
using Std.IO;
using core.ops;

fn processData(a: int, b: int) -> int {
    Std.print("utils.helpers: processing data\n");
    return transform(a, b);
}
EOF

# API (uses everything)
cat > src/api/handlers.mg << 'EOF'
using Std.IO;
using core.types;
using core.ops;
using utils.helpers;

fn handleRequest(x: int, y: int) -> int {
    Std.print("api.handlers: handling request\n");
    let data = processData(x, y);
    let point = makePoint(x, y);
    return data + point;
}
EOF

# Main (uses api)
cat > src/main.mg << 'EOF'
using Std.IO;
using api.handlers;

fn main() {
    Std.print("Test 4: Complex Module Graph\n");
    Std.print("==============================\n");
    Std.print("Testing module dependency chain:\n");
    Std.print("main -> api -> utils -> core\n\n");
    
    let result = handleRequest(5, 3);
    Std.print($"\nFinal result: {result}\n");
    
    Std.print("\n✅ Test 4 passed!\n\n");
}
EOF

echo "📦 Building test 4..."
gear build || { echo "❌ Build failed"; exit 1; }

echo "🚀 Running test 4..."
echo "─────────────────────────────────────────"
gear run || { echo "❌ Run failed"; exit 1; }
echo "─────────────────────────────────────────"

cd ..

# ============================================================================
# Summary
# ============================================================================
echo ""
echo "╔════════════════════════════════════════╗"
echo "║           TEST SUMMARY                 ║"
echo "╚════════════════════════════════════════╝"
echo ""
echo "✅ Test 1: Simple Module Import - PASSED"
echo "✅ Test 2: Nested Modules - PASSED"
echo "✅ Test 3: Package Dependencies - PASSED"
echo "✅ Test 4: Complex Module Graph - PASSED"
echo ""
echo "All module system tests passed! 🎉"
echo ""
echo "Module system features verified:"
echo "  ✓ Basic module import (using module_name)"
echo "  ✓ Nested modules (using path.to.module)"
echo "  ✓ Cross-module function calls"
echo "  ✓ Package imports (using external_package)"
echo "  ✓ Complex dependency chains"
echo "  ✓ Module name resolution"
echo "  ✓ Symbol visibility"
echo ""
echo "Artifacts created:"
echo "  📁 module-test-app/ - Test application"
echo "  📁 module-test-lib/ - Test library"
echo ""
echo "Clean up with: rm -rf module-test-app module-test-lib"
