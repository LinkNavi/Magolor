#!/bin/bash

echo "🔧 Applying Complete Fix to Magolor Compiler"
echo "============================================="
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check we're in the right directory
if [ ! -f "Cargo.toml" ]; then
    echo -e "${RED}❌ Error: Must run from MagolorCompiler directory${NC}"
    exit 1
fi

# Step 1: Backup files
echo -e "${YELLOW}Step 1: Creating backups...${NC}"
cp src/modules/ir/ir_builder.rs src/modules/ir/ir_builder.rs.backup.$(date +%s)
cp src/modules/ir/codegen.rs src/modules/ir/codegen.rs.backup.$(date +%s)
echo -e "${GREEN}   ✅ Backups created${NC}"
echo ""

# Step 2: Instructions
echo -e "${YELLOW}Step 2: YOU NEED TO UPDATE TWO FILES:${NC}"
echo ""
echo "1. Replace src/modules/ir/ir_builder.rs with:"
echo "   'Fixed IR Builder - Register Preservation' artifact"
echo ""
echo "2. Replace src/modules/ir/codegen.rs with:"
echo "   'Fixed Codegen - Proper Local Handling' artifact"
echo ""
read -p "Press Enter when BOTH files are updated..."

# Step 3: Build
echo ""
echo -e "${YELLOW}Step 3: Building compiler...${NC}"
cargo build 2>&1 | tee /tmp/build.log

#if grep -q "error" /tmp/build.log; then
 #   echo -e "${RED}❌ Build failed! Check errors above.${NC}"
  #  exit 1
#fi

echo -e "${GREEN}   ✅ Build successful${NC}"

# Step 4: Compile test
echo ""
echo -e "${YELLOW}Step 4: Compiling hallo.mg...${NC}"
cargo run hallo.mg 2>&1 | grep -E "(Generated|error)" | tail -5

if [ ! -f hallo.s ]; then
    echo -e "${RED}❌ hallo.s not generated${NC}"
    exit 1
fi

echo -e "${GREEN}   ✅ Assembly generated ($(wc -l < hallo.s) lines)${NC}"

# Step 5: Check assembly
echo ""
echo -e "${YELLOW}Step 5: Verifying assembly quality...${NC}"

# Check add function
echo "   Checking add function..."
if grep -A 10 "Calculator.MathOperations.add:" hallo.s | grep -q "add.*rdi.*rsi"; then
    echo -e "${GREEN}   ✅ Add uses parameters correctly${NC}"
else
    echo -e "${YELLOW}   ⚠️  Add might have issues${NC}"
fi

# Check divide - should load from locals
echo "   Checking divide function..."
DIVIDE_SECTION=$(grep -A 20 "Calculator.MathOperations.divide:" hallo.s)
if echo "$DIVIDE_SECTION" | grep -q "mov.*\[rbp"; then
    echo -e "${GREEN}   ✅ Divide loads from locals${NC}"
else
    echo -e "${YELLOW}   ⚠️  Divide may not load parameters${NC}"
fi

# Check factorial - should have proper recursion
echo "   Checking factorial function..."
FACTORIAL_SECTION=$(grep -A 30 "Calculator.MathOperations.factorial:" hallo.s)
if echo "$FACTORIAL_SECTION" | grep -q "imul" && echo "$FACTORIAL_SECTION" | grep -q "call.*factorial"; then
    echo -e "${GREEN}   ✅ Factorial has recursion and multiply${NC}"
else
    echo -e "${YELLOW}   ⚠️  Factorial may be incomplete${NC}"
fi

# Step 6: Build executable
echo ""
echo -e "${YELLOW}Step 6: Building executable...${NC}"

if [ ! -f runtime.c ]; then
    echo -e "${RED}❌ runtime.c not found!${NC}"
    exit 1
fi

gcc -c hallo.s -o hallo.o 2>&1
if [ $? -ne 0 ]; then
    echo -e "${RED}❌ Assembly failed${NC}"
    exit 1
fi

gcc -c runtime.c -o runtime.o -lm 2>&1
if [ $? -ne 0 ]; then
    echo -e "${RED}❌ Runtime compilation failed${NC}"
    exit 1
fi

gcc hallo.o runtime.o -o program -no-pie -lm 2>&1
if [ $? -ne 0 ]; then
    echo -e "${RED}❌ Linking failed${NC}"
    exit 1
fi

echo -e "${GREEN}   ✅ Executable created${NC}"

# Step 7: Run with gdb to catch segfaults
echo ""
echo "========================================"
echo -e "${GREEN}🎉 RUNNING PROGRAM${NC}"
echo "========================================"
echo ""

# Run normally first
timeout 2 ./program
EXIT_CODE=$?

echo ""
echo "========================================"

if [ $EXIT_CODE -eq 0 ]; then
    echo -e "${GREEN}✅ SUCCESS! Program completed${NC}"
    echo ""
    echo "Expected output:"
    echo "  5 + 3 = 8"
    echo "  10 / 2 = 5"
    echo "  5! = 120"
    echo "  Sum of array: 15"
elif [ $EXIT_CODE -eq 139 ]; then
    echo -e "${RED}❌ SEGMENTATION FAULT${NC}"
    echo ""
    echo "Running with GDB to find the issue..."
    echo ""
    gdb -batch -ex 'run' -ex 'bt' -ex 'info registers' ./program 2>&1 | tail -40
elif [ $EXIT_CODE -eq 124 ]; then
    echo -e "${RED}❌ TIMEOUT (infinite loop?)${NC}"
else
    echo -e "${YELLOW}⚠️  Exit code: $EXIT_CODE${NC}"
fi

echo ""
echo "========================================"
echo "Diagnosis:"
echo ""

# Show the first few instructions of main
echo "Main function start:"
grep -A 15 "^main:" hallo.s | head -20

echo ""
echo "If there are still issues, check:"
echo "1. Are parameters being stored to locals in function prologue?"
echo "2. Are Load instructions properly reading from [rbp - offset]?"
echo "3. Are function arguments using the right registers (rdi, rsi)?"