#!/bin/bash
# Quick test script for Magolor package system

set -e

echo "╔════════════════════════════════════════╗"
echo "║  Magolor Package System Quick Test    ║"
echo "╚════════════════════════════════════════╝"
echo ""

# Clean up any previous tests
echo "🧹 Cleaning up previous tests..."
rm -rf test-pkg-math test-pkg-app

# Step 1: Create a simple math library
echo ""
echo "📦 Step 1: Creating math library package..."
mkdir -p test-pkg-math/src
cd test-pkg-math

cat > project.toml << 'EOF'
[project]
name = "math-utils"
version = "1.0.0"
description = "Simple math utilities"
license = "MIT"

[dependencies]
# No dependencies for this simple library
EOF

cat > src/lib.mg << 'EOF'
using Std.IO;

fn add(a: int, b: int) -> int {
    Std.print("math-utils: adding numbers\n");
    return a + b;
}

fn multiply(a: int, b: int) -> int {
    Std.print("math-utils: multiplying numbers\n");
    return a * b;
}

fn square(x: int) -> int {
    return multiply(x, x);
}

fn factorial(n: int) -> int {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}
EOF

cat > README.md << 'EOF'
# Math Utils

Simple math utility functions for Magolor.

## Functions

- `add(a, b)` - Add two integers
- `multiply(a, b)` - Multiply two integers  
- `square(x)` - Square a number
- `factorial(n)` - Calculate factorial
EOF

echo "✅ Math library created at test-pkg-math/"
cd ..

# Step 2: Create an app that uses the library
echo ""
echo "📦 Step 2: Creating app that uses the library..."
mkdir -p test-pkg-app
cd test-pkg-app
gear init test-app

cat > project.toml << 'EOF'
[project]
name = "test-app"
version = "0.1.0"
authors = ["Test User <test@example.com>"]
description = "Testing package system with local dependency"
license = "MIT"

[dependencies]
math-utils = "path:../test-pkg-math"

[build]
optimization = "2"
EOF

cat > src/main.mg << 'EOF'
using Std.IO;

fn main() {
    Std.print("╔═══════════════════════════════════╗\n");
    Std.print("║  Testing Magolor Package System  ║\n");
    Std.print("╚═══════════════════════════════════╝\n");
    Std.print("\n");
    
    Std.print("Testing functions from math-utils package:\n");
    Std.print("\n");
    
    // Test addition
    let sum = add(10, 5);
    Std.print($"10 + 5 = {sum}\n");
    
    // Test multiplication
    let product = multiply(7, 6);
    Std.print($"7 * 6 = {product}\n");
    
    // Test square
    let sq = square(8);
    Std.print($"8 squared = {sq}\n");
    
    // Test factorial
    let fact = factorial(5);
    Std.print($"5! = {fact}\n");
    
    Std.print("\n");
    Std.print("✅ All package functions work!\n");
}
EOF

echo "✅ Test app created at test-pkg-app/"

# Step 3: Install dependencies
echo ""
echo "📥 Step 3: Installing dependencies..."
gear install

# Step 4: Check what was installed
echo ""
echo "📋 Step 4: Checking installation..."
if [ -d ".magolor/packages/math-utils" ]; then
    echo "✅ math-utils package installed to .magolor/packages/"
    ls -lh .magolor/packages/math-utils
else
    echo "❌ Package installation failed"
    exit 1
fi

if [ -f ".magolor/lock.toml" ]; then
    echo "✅ Lock file created"
    echo ""
    echo "Lock file contents:"
    cat .magolor/lock.toml
else
    echo "⚠️  Lock file not created"
fi

# Step 5: Build the project
echo ""
echo "🔨 Step 5: Building project with dependencies..."
gear build

# Step 6: Run the project
echo ""
echo "🚀 Step 6: Running the application..."
echo "─────────────────────────────────────────"
gear run
echo "─────────────────────────────────────────"

# Summary
echo ""
echo "╔════════════════════════════════════════╗"
echo "║           TEST SUMMARY                 ║"
echo "╚════════════════════════════════════════╝"
echo ""
echo "✅ Created library package: test-pkg-math"
echo "✅ Created app package: test-pkg-app"
echo "✅ Installed local dependency"
echo "✅ Built project with dependency sources"
echo "✅ Ran application successfully"
echo ""
echo "Package system is working! 🎉"
echo ""
echo "To explore:"
echo "  - Check .magolor/packages/ for installed packages"
echo "  - Check .magolor/lock.toml for dependency versions"
echo "  - Try modifying test-pkg-math/src/lib.mg and rebuild"
echo ""
echo "Clean up with: rm -rf test-pkg-math test-pkg-app"
