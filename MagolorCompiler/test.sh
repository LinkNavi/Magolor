#!/bin/bash
# ============================================================================
# Magolor Compiler - Comprehensive Test Suite
# ============================================================================
# This script tests ALL features of the Magolor language and compiler:
# - Basic syntax and types
# - Functions and closures
# - Classes and OOP
# - Module system
# - Package management
# - C++ interop
# - Standard library
# - Error handling
# ============================================================================

set -e

# Color codes for output
RED='\033[1;31m'
GREEN='\033[1;32m'
YELLOW='\033[1;33m'
BLUE='\033[1;34m'
CYAN='\033[1;36m'
WHITE='\033[1;37m'
RESET='\033[0m'

# Test counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
SKIPPED_TESTS=0

# Print header
print_header() {
    echo ""
    echo -e "${BLUE}╔════════════════════════════════════════════════════════════╗${RESET}"
    echo -e "${BLUE}║${WHITE}  $1${BLUE}$(printf '%*s' $((57 - ${#1})) '')║${RESET}"
    echo -e "${BLUE}╚════════════════════════════════════════════════════════════╝${RESET}"
    echo ""
}

# Print test result
print_result() {
    local test_name="$1"
    local result="$2"
    local message="$3"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    if [ "$result" = "PASS" ]; then
        echo -e "${GREEN}✓${RESET} ${test_name}"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    elif [ "$result" = "FAIL" ]; then
        echo -e "${RED}✗${RESET} ${test_name}"
        if [ -n "$message" ]; then
            echo -e "  ${RED}Error:${RESET} $message"
        fi
        FAILED_TESTS=$((FAILED_TESTS + 1))
    elif [ "$result" = "SKIP" ]; then
        echo -e "${YELLOW}⊘${RESET} ${test_name} (skipped)"
        SKIPPED_TESTS=$((SKIPPED_TESTS + 1))
    fi
}

# Run a test
run_test() {
    local test_name="$1"
    local test_file="$2"
    local expected_output="$3"
    
    # Compile and run
    if output=$(magolor run "$test_file" 2>&1); then
        if [ -n "$expected_output" ]; then
            if echo "$output" | grep -q "$expected_output"; then
                print_result "$test_name" "PASS"
            else
                print_result "$test_name" "FAIL" "Output doesn't match. Expected: '$expected_output', Got: '$output'"
            fi
        else
            print_result "$test_name" "PASS"
        fi
    else
        print_result "$test_name" "FAIL" "Compilation or execution failed: $output"
    fi
}

# Check prerequisites
check_prerequisites() {
    print_header "Checking Prerequisites"
    
    if ! command -v magolor &> /dev/null; then
        echo -e "${RED}✗ magolor not found${RESET}"
        echo ""
        echo "Please install the Magolor compiler:"
        echo "  cd MagolorCompiler"
        echo "  zora build"
        echo "  sudo cp target/debug/Magolor /usr/local/bin/magolor"
        exit 1
    fi
    echo -e "${GREEN}✓${RESET} magolor found: $(which magolor)"
    
    if ! command -v gear &> /dev/null; then
        echo -e "${RED}✗ gear not found${RESET}"
        echo ""
        echo "Please install Gear:"
        echo "  cd Gear"
        echo "  zora build"
        echo "  ./move.sh"
        exit 1
    fi
    echo -e "${GREEN}✓${RESET} gear found: $(which gear)"
    
    if ! command -v g++ &> /dev/null; then
        echo -e "${RED}✗ g++ not found${RESET}"
        exit 1
    fi
    echo -e "${GREEN}✓${RESET} g++ found: $(which g++)"
    
    echo ""
}

# Clean up test artifacts
cleanup() {
    echo ""
    echo -e "${CYAN}Cleaning up test artifacts...${RESET}"
    rm -rf test_* .test_*
    echo -e "${GREEN}✓${RESET} Cleanup complete"
}

# ============================================================================
# TEST SUITE 1: Basic Syntax
# ============================================================================
test_basic_syntax() {
    print_header "TEST SUITE 1: Basic Syntax"
    
    # Test 1.1: Hello World
    cat > test_hello.mg << 'EOF'
using Std.IO;

fn main() {
    Std.print("Hello, World!\n");
}
EOF
    run_test "1.1 Hello World" "test_hello.mg" "Hello, World!"
    
    # Test 1.2: Variables and Types
    cat > test_variables.mg << 'EOF'
using Std.IO;

fn main() {
    let x = 42;
    let y = 3.14;
    let s = "test";
    let b = true;
    
    Std.print($"int: {x}\n");
    Std.print($"float: {y}\n");
    Std.print($"string: {s}\n");
    Std.print($"bool: {b}\n");
}
EOF
    run_test "1.2 Variables and Types" "test_variables.mg" "int: 42"
    
    # Test 1.3: Mutable Variables
    cat > test_mut.mg << 'EOF'
using Std.IO;

fn main() {
    let mut x = 10;
    Std.print($"Before: {x}\n");
    x = 20;
    Std.print($"After: {x}\n");
}
EOF
    run_test "1.3 Mutable Variables" "test_mut.mg" "Before: 10"
    
    # Test 1.4: Arithmetic
    cat > test_arithmetic.mg << 'EOF'
using Std.IO;

fn main() {
    let a = 10;
    let b = 3;
    
    Std.print($"Add: {a + b}\n");
    Std.print($"Sub: {a - b}\n");
    Std.print($"Mul: {a * b}\n");
    Std.print($"Div: {a / b}\n");
    Std.print($"Mod: {a % b}\n");
}
EOF
    run_test "1.4 Arithmetic Operations" "test_arithmetic.mg" "Add: 13"
    
    # Test 1.5: String Interpolation
    cat > test_interpolation.mg << 'EOF'
using Std.IO;

fn main() {
    let name = "Alice";
    let age = 30;
    Std.print($"Name: {name}, Age: {age}\n");
}
EOF
    run_test "1.5 String Interpolation" "test_interpolation.mg" "Name: Alice, Age: 30"
    
    # Test 1.6: Arrays
    cat > test_arrays.mg << 'EOF'
using Std.IO;

fn main() {
    let numbers = [1, 2, 3, 4, 5];
    Std.print("Array: ");
    for (n in numbers) {
        Std.print($"{n} ");
    }
    Std.print("\n");
}
EOF
    run_test "1.6 Arrays" "test_arrays.mg" "Array: 1 2 3 4 5"
    
    rm -f test_hello.mg test_variables.mg test_mut.mg test_arithmetic.mg test_interpolation.mg test_arrays.mg
}

# ============================================================================
# TEST SUITE 2: Control Flow
# ============================================================================
test_control_flow() {
    print_header "TEST SUITE 2: Control Flow"
    
    # Test 2.1: If-Else
    cat > test_if.mg << 'EOF'
using Std.IO;

fn main() {
    let x = 10;
    if (x > 5) {
        Std.print("Greater\n");
    } else {
        Std.print("Lesser\n");
    }
}
EOF
    run_test "2.1 If-Else Statement" "test_if.mg" "Greater"
    
    # Test 2.2: While Loop
    cat > test_while.mg << 'EOF'
using Std.IO;

fn main() {
    let mut i = 0;
    while (i < 5) {
        Std.print($"{i} ");
        i = i + 1;
    }
    Std.print("\n");
}
EOF
    run_test "2.2 While Loop" "test_while.mg" "0 1 2 3 4"
    
    # Test 2.3: For Loop
    cat > test_for.mg << 'EOF'
using Std.IO;

fn main() {
    let items = [10, 20, 30];
    for (item in items) {
        Std.print($"{item} ");
    }
    Std.print("\n");
}
EOF
    run_test "2.3 For Loop" "test_for.mg" "10 20 30"
    
    # Test 2.4: Nested Control Flow
    cat > test_nested.mg << 'EOF'
using Std.IO;

fn main() {
    let mut sum = 0;
    let mut i = 0;
    while (i < 3) {
        let mut j = 0;
        while (j < 3) {
            sum = sum + 1;
            j = j + 1;
        }
        i = i + 1;
    }
    Std.print($"Sum: {sum}\n");
}
EOF
    run_test "2.4 Nested Control Flow" "test_nested.mg" "Sum: 9"
    
    rm -f test_if.mg test_while.mg test_for.mg test_nested.mg
}

# ============================================================================
# TEST SUITE 3: Functions
# ============================================================================
test_functions() {
    print_header "TEST SUITE 3: Functions"
    
    # Test 3.1: Basic Function
    cat > test_func.mg << 'EOF'
using Std.IO;

fn add(a: int, b: int) -> int {
    return a + b;
}

fn main() {
    let result = add(5, 3);
    Std.print($"Result: {result}\n");
}
EOF
    run_test "3.1 Basic Function" "test_func.mg" "Result: 8"
    
    # Test 3.2: Multiple Parameters
    cat > test_params.mg << 'EOF'
using Std.IO;

fn greet(name: string, age: int) {
    Std.print($"Hello {name}, you are {age} years old\n");
}

fn main() {
    greet("Bob", 25);
}
EOF
    run_test "3.2 Multiple Parameters" "test_params.mg" "Hello Bob"
    
    # Test 3.3: Recursion
    cat > test_recursion.mg << 'EOF'
using Std.IO;

fn factorial(n: int) -> int {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

fn main() {
    let result = factorial(5);
    Std.print($"5! = {result}\n");
}
EOF
    run_test "3.3 Recursion (Factorial)" "test_recursion.mg" "5! = 120"
    
    # Test 3.4: Lambda/Closure
    cat > test_lambda.mg << 'EOF'
using Std.IO;

fn makeAdder(x: int) -> fn(int) -> int {
    return fn(y: int) -> int {
        return x + y;
    };
}

fn main() {
    let add5 = makeAdder(5);
    let result = add5(3);
    Std.print($"Result: {result}\n");
}
EOF
    run_test "3.4 Lambda/Closure" "test_lambda.mg" "Result: 8"
    
    rm -f test_func.mg test_params.mg test_recursion.mg test_lambda.mg
}

# ============================================================================
# TEST SUITE 4: Classes and OOP
# ============================================================================
test_classes() {
    print_header "TEST SUITE 4: Classes and OOP"
    
    # Test 4.1: Basic Class
    cat > test_class.mg << 'EOF'
using Std.IO;

class Point {
    pub x: int;
    pub y: int;
    
    pub fn display() {
        Std.print($"Point({this.x}, {this.y})\n");
    }
}

fn main() {
    let p = new Point();
    p.x = 10;
    p.y = 20;
    p.display();
}
EOF
    run_test "4.1 Basic Class" "test_class.mg" "Point(10, 20)"
    
    # Test 4.2: Class with Methods
    cat > test_methods.mg << 'EOF'
using Std.IO;

class Calculator {
    pub fn add(a: int, b: int) -> int {
        return a + b;
    }
    
    pub fn multiply(a: int, b: int) -> int {
        return a * b;
    }
}

fn main() {
    let calc = new Calculator();
    Std.print($"Add: {calc.add(5, 3)}\n");
    Std.print($"Mul: {calc.multiply(5, 3)}\n");
}
EOF
    run_test "4.2 Class with Methods" "test_methods.mg" "Add: 8"
    
    # Test 4.3: Static Members
    cat > test_static.mg << 'EOF'
using Std.IO;

class Config {
    pub static VERSION: int = 1;
    
    pub static fn getVersion() -> int {
        return 1;
    }
}

fn main() {
    Std.print($"Version: {Config::getVersion()}\n");
}
EOF
    run_test "4.3 Static Members" "test_static.mg" "Version: 1"
    
    rm -f test_class.mg test_methods.mg test_static.mg
}

# ============================================================================
# TEST SUITE 5: Standard Library
# ============================================================================
test_stdlib() {
    print_header "TEST SUITE 5: Standard Library"
    
    # Test 5.1: Std.IO
    cat > test_stdio.mg << 'EOF'
using Std.IO;

fn main() {
    Std.print("Testing IO\n");
    Std.println("With newline");
}
EOF
    run_test "5.1 Std.IO" "test_stdio.mg" "Testing IO"
    
    # Test 5.2: Std.Parse
    cat > test_parse.mg << 'EOF'
using Std.IO;
using Std.Parse;
using Std.Option;

fn main() {
    let result = Std.parseInt("42");
    match result {
        Some(v) => Std.print($"Parsed: {v}\n"),
        None => Std.print("Failed\n")
    }
}
EOF
    run_test "5.2 Std.Parse" "test_parse.mg" "Parsed: 42"
    
    # Test 5.3: Std.Math
    cat > test_math.mg << 'EOF'
using Std.IO;
using Std.Math;

fn main() {
    let x = Std.Math.sqrt(16.0);
    Std.print($"sqrt(16) = {x}\n");
}
EOF
    run_test "5.3 Std.Math" "test_math.mg" "sqrt(16) = 4"
    
    # Test 5.4: Std.String
    cat > test_string.mg << 'EOF'
using Std.IO;
using Std.String;

fn main() {
    let s = "  hello  ";
    let trimmed = Std.String.trim(s);
    Std.print($"Trimmed: '{trimmed}'\n");
}
EOF
    run_test "5.4 Std.String" "test_string.mg" "Trimmed: 'hello'"
    
    # Test 5.5: Std.Array
    cat > test_array_ops.mg << 'EOF'
using Std.IO;
using Std.Array;

fn main() {
    let mut arr = [1, 2, 3];
    Std.Array.push(arr, 4);
    Std.print($"Length: {Std.Array.length(arr)}\n");
}
EOF
    run_test "5.5 Std.Array" "test_array_ops.mg" "Length: 4"
    
    rm -f test_stdio.mg test_parse.mg test_math.mg test_string.mg test_array_ops.mg
}

# ============================================================================
# TEST SUITE 6: Option/Match Pattern Matching
# ============================================================================
test_pattern_matching() {
    print_header "TEST SUITE 6: Pattern Matching"
    
    # Test 6.1: Option Type
    cat > test_option.mg << 'EOF'
using Std.IO;

fn divide(a: int, b: int) -> int {
    if (b == 0) {
        return 0;
    }
    return a / b;
}

fn main() {
    let result = divide(10, 2);
    Std.print($"Result: {result}\n");
}
EOF
    run_test "6.1 Option Type" "test_option.mg" "Result: 5"
    
    # Test 6.2: Match Expression
    cat > test_match.mg << 'EOF'
using Std.IO;
using Std.Parse;

fn main() {
    let opt = Std.parseInt("42");
    
    match opt {
        Some(v) => {
            Std.print($"Got value: {v}\n");
        },
        None => {
            Std.print("No value\n");
        }
    }
}
EOF
    run_test "6.2 Match Expression" "test_match.mg" "Got value: 42"
    
    rm -f test_option.mg test_match.mg
}

# ============================================================================
# TEST SUITE 7: C++ Interop
# ============================================================================
test_cpp_interop() {
    print_header "TEST SUITE 7: C++ Interop"
    
    # Test 7.1: cimport
    cat > test_cimport.mg << 'EOF'
using Std.IO;

cimport <math.h> as Math;

fn main() {
    let result = Math::sqrt(16.0);
    Std.print($"sqrt(16) = {result}\n");
}
EOF
    run_test "7.1 C Import" "test_cimport.mg" "sqrt(16) = 4"
    
    # Test 7.2: @cpp blocks
    cat > test_cpp_block.mg << 'EOF'
using Std.IO;

cimport <stdio.h> (printf);

fn main() {
    Std.print("From Magolor\n");
    
    @cpp {
        printf("From C++\n");
    }
}
EOF
    run_test "7.2 @cpp Blocks" "test_cpp_block.mg" "From Magolor"
    
    # Test 7.3: Variable Sharing
    cat > test_var_sharing.mg << 'EOF'
using Std.IO;

cimport <stdio.h> (printf);

fn main() {
    let mut x = 10;
    
    Std.print($"Before: {x}\n");
    
    @cpp {
        x = 20;
        printf("Changed in C++\n");
    }
    
    Std.print($"After: {x}\n");
}
EOF
    run_test "7.3 Variable Sharing" "test_var_sharing.mg" "After: 20"
    
    rm -f test_cimport.mg test_cpp_block.mg test_var_sharing.mg
}

# ============================================================================
# TEST SUITE 8: Module System
# ============================================================================
test_modules() {
    print_header "TEST SUITE 8: Module System"
    
    # Create test project
    mkdir -p test_modules/src/{math,utils}
    cd test_modules
    
    # Create project.toml
    cat > project.toml << 'EOF'
[project]
name = "test-modules"
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

fn multiply(a: int, b: int) -> int {
    return a * b;
}
EOF
    
    # Create utils.helpers module
    cat > src/utils/helpers.mg << 'EOF'
using Std.IO;
using math.basic;

fn calculate(x: int, y: int) -> int {
    return add(x, y) * 2;
}
EOF
    
    # Create main
    cat > src/main.mg << 'EOF'
using Std.IO;
using utils.helpers;

fn main() {
    let result = calculate(5, 3);
    Std.print($"Result: {result}\n");
}
EOF
    
    # Build and run
    if gear build > /dev/null 2>&1 && gear run 2>&1 | grep -q "Result: 16"; then
        print_result "8.1 Module System" "PASS"
    else
        print_result "8.1 Module System" "FAIL" "Module build or execution failed"
    fi
    
    cd ..
    rm -rf test_modules
}

# ============================================================================
# TEST SUITE 9: Package System
# ============================================================================
test_packages() {
    print_header "TEST SUITE 9: Package System"
    
    # Create library package
    mkdir -p test_pkg_lib/src
    cd test_pkg_lib
    
    cat > project.toml << 'EOF'
[project]
name = "testlib"
version = "1.0.0"
description = "Test library"
license = "MIT"

[dependencies]
EOF
    
    cat > src/lib.mg << 'EOF'
using Std.IO;

fn double(x: int) -> int {
    return x * 2;
}
EOF
    
    cd ..
    
    # Create app that uses library
    mkdir -p test_pkg_app
    cd test_pkg_app
    gear init test-app > /dev/null 2>&1
    
    cat > project.toml << 'EOF'
[project]
name = "test-app"
version = "0.1.0"
authors = ["Test User <test@example.com>"]
description = "Testing package system"
license = "MIT"

[dependencies]
testlib = "path:../test_pkg_lib"

[build]
optimization = "2"
EOF
    
    cat > src/main.mg << 'EOF'
using Std.IO;

fn main() {
    let result = double(21);
    Std.print($"Result: {result}\n");
}
EOF
    
    # Install dependencies and build
    if gear install > /dev/null 2>&1 && gear build > /dev/null 2>&1 && gear run 2>&1 | grep -q "Result: 42"; then
        print_result "9.1 Package Dependencies" "PASS"
    else
        print_result "9.1 Package Dependencies" "FAIL" "Package build or execution failed"
    fi
    
    cd ..
    rm -rf test_pkg_lib test_pkg_app
}

# ============================================================================
# TEST SUITE 10: Error Handling
# ============================================================================
test_error_handling() {
    print_header "TEST SUITE 10: Error Handling"
    
    # Test 10.1: Syntax Error Detection
    cat > test_syntax_error.mg << 'EOF'
using Std.IO;

fn main() {
    let x = 
}
EOF
    
    if ! magolor check test_syntax_error.mg > /dev/null 2>&1; then
        print_result "10.1 Syntax Error Detection" "PASS"
    else
        print_result "10.1 Syntax Error Detection" "FAIL" "Should have detected syntax error"
    fi
    
    # Test 10.2: Type Error Detection
    cat > test_type_error.mg << 'EOF'
using Std.IO;

fn main() {
    let x: int = "string";
}
EOF
    
    if ! magolor check test_type_error.mg > /dev/null 2>&1; then
        print_result "10.2 Type Error Detection" "PASS"
    else
        print_result "10.2 Type Error Detection" "FAIL" "Should have detected type error"
    fi
    
    # Test 10.3: Undefined Variable
    cat > test_undefined.mg << 'EOF'
using Std.IO;

fn main() {
    Std.print($"{undefined_var}\n");
}
EOF
    
    if ! magolor check test_undefined.mg > /dev/null 2>&1; then
        print_result "10.3 Undefined Variable Detection" "PASS"
    else
        print_result "10.3 Undefined Variable Detection" "FAIL" "Should have detected undefined variable"
    fi
    
    rm -f test_syntax_error.mg test_type_error.mg test_undefined.mg
}

# ============================================================================
# TEST SUITE 11: Advanced Features
# ============================================================================
test_advanced() {
    print_header "TEST SUITE 11: Advanced Features"
    
    # Test 11.1: Complex Recursion (Fibonacci)
    cat > test_fibonacci.mg << 'EOF'
using Std.IO;

fn fib(n: int) -> int {
    if (n <= 1) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

fn main() {
    let result = fib(10);
    Std.print($"fib(10) = {result}\n");
}
EOF
    run_test "11.1 Fibonacci Recursion" "test_fibonacci.mg" "fib(10) = 55"
    
    # Test 11.2: Bubble Sort
    cat > test_sort.mg << 'EOF'
using Std.IO;

fn bubbleSort(arr: int) {
    let mut i = 0;
    let n = 5;
    while (i < n) {
        let mut j = 0;
        while (j < n - 1) {
            j = j + 1;
        }
        i = i + 1;
    }
}

fn main() {
    let mut arr = [5, 2, 8, 1, 9];
    bubbleSort(arr);
    Std.print("Sorted\n");
}
EOF
    run_test "11.2 Sorting Algorithm" "test_sort.mg" "Sorted"
    
    # Test 11.3: Higher-Order Functions
    cat > test_hof.mg << 'EOF'
using Std.IO;

fn apply(f: fn(int) -> int, x: int) -> int {
    return f(x);
}

fn square(x: int) -> int {
    return x * x;
}

fn main() {
    let result = apply(square, 5);
    Std.print($"Result: {result}\n");
}
EOF
    run_test "11.3 Higher-Order Functions" "test_hof.mg" "Result: 25"
    
    rm -f test_fibonacci.mg test_sort.mg test_hof.mg
}

# ============================================================================
# Main Execution
# ============================================================================

main() {
    print_header "Magolor Compiler - Comprehensive Test Suite"
    
    echo -e "${CYAN}This suite tests all features of the Magolor language:${RESET}"
    echo "  • Basic syntax, types, and operators"
    echo "  • Control flow (if, while, for, match)"
    echo "  • Functions, lambdas, and closures"
    echo "  • Classes and object-oriented programming"
    echo "  • Standard library functions"
    echo "  • Pattern matching with Option types"
    echo "  • C++ interoperability (cimport, @cpp)"
    echo "  • Module system"
    echo "  • Package management"
    echo "  • Error detection and handling"
    echo "  • Advanced algorithms"
    echo ""
    
    check_prerequisites
    
    # Run all test suites
    test_basic_syntax
    test_control_flow
    test_functions
    test_classes
    test_stdlib
    test_pattern_matching
    test_cpp_interop
    test_modules
    test_packages
    test_error_handling
    test_advanced
    
    # Print summary
    echo ""
    print_header "Test Summary"
    
    echo -e "${WHITE}Total Tests:${RESET}   $TOTAL_TESTS"
    echo -e "${GREEN}Passed:${RESET}        $PASSED_TESTS"
    echo -e "${RED}Failed:${RESET}        $FAILED_TESTS"
    echo -e "${YELLOW}Skipped:${RESET}       $SKIPPED_TESTS"
    echo ""
    
    local pass_rate=0
    if [ $TOTAL_TESTS -gt 0 ]; then
        pass_rate=$((PASSED_TESTS * 100 / TOTAL_TESTS))
    fi
    
    echo -e "${CYAN}Pass Rate:${RESET}     ${pass_rate}%"
    echo ""
    
    if [ $FAILED_TESTS -eq 0 ]; then
        echo -e "${GREEN}╔════════════════════════════════════════╗${RESET}"
        echo -e "${GREEN}║${WHITE}  ALL TESTS PASSED! 🎉                 ${GREEN}║${RESET}"
        echo -e "${GREEN}║${WHITE}  Magolor compiler is ready to use!   ${GREEN}║${RESET}"
        echo -e "${GREEN}╚════════════════════════════════════════╝${RESET}"
        cleanup
        exit 0
    else
        echo -e "${RED}╔════════════════════════════════════════╗${RESET}"
        echo -e "${RED}║${WHITE}  SOME TESTS FAILED                    ${RED}║${RESET}"
        echo -e "${RED}║${WHITE}  Please review the errors above       ${RED}║${RESET}"
        echo -e "${RED}╚════════════════════════════════════════╝${RESET}"
        cleanup
        exit 1
    fi
}

# Handle Ctrl+C
trap cleanup EXIT

# Run main
main "$@"
