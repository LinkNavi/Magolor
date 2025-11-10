// examples/showcase.mg - Demonstrating Magolor's capabilities

// ============================================
// Constants and Type Aliases
// ============================================

const i32 MAX_BUFFER_SIZE = 1024;
const f64 PI = 3.14159265359;

// ============================================
// Structs with Generics
// ============================================

struct Point<T> {
    public T x;
    public T y;
}

struct Vector3 {
    public f32 x;
    public f32 y;
    public f32 z;
}

// ============================================
// Enums with Data
// ============================================

enum Result<T, E> {
    Ok(T),
    Err(E)
}

enum Option<T> {
    Some(T),
    None
}

// ============================================
// Classes with Methods
// ============================================

class Calculator {
    private i32 memory;
    
    public void fn constructor() {
        this.memory = 0;
    }
    
    public i32 fn add(i32 a, i32 b) {
        return a + b;
    }
    
    public i32 fn multiply(i32 a, i32 b) {
        return a * b;
    }
    
    public void fn store(i32 value) {
        this.memory = value;
    }
    
    public i32 fn recall() {
        return this.memory;
    }
}

// ============================================
// Traits and Implementations
// ============================================

trait Printable {
    void fn print();
}

impl Printable for Vector3 {
    void fn print() {
        print_str_nn("Vector3(");
        print_f32_nn(this.x);
        print_str_nn(", ");
        print_f32_nn(this.y);
        print_str_nn(", ");
        print_f32_nn(this.z);
        print_str_nn(")");
    }
}

// ============================================
// Functions with Generics
// ============================================

T fn max<T>(T a, T b) where T: Comparable {
    if (a > b) {
        return a;
    }
    return b;
}

// ============================================
// Higher-Order Functions and Lambdas
// ============================================

i32 fn apply_operation(i32 x, i32 y, fn(i32, i32) -> i32 operation) {
    return operation(x, y);
}

i32[] fn map(i32[] arr, fn(i32) -> i32 transform) {
    let i32 len = arr.length;
    let i32[] result = new i32[len];
    
    for (let i32 i = 0; i < len; i++) {
        result[i] = transform(arr[i]);
    }
    
    return result;
}

// ============================================
// Pattern Matching
// ============================================

i32 fn fibonacci(i32 n) {
    match n {
        0 => return 0,
        1 => return 1,
        _ => return fibonacci(n - 1) + fibonacci(n - 2)
    }
}

string fn describe_number(i32 n) {
    match n {
        0 => return "zero",
        1 => return "one", 
        2..10 => return "small",
        10..100 => return "medium",
        _ => return "large"
    }
}

// ============================================
// Error Handling with Result/Option
// ============================================

Result<i32, string> fn safe_divide(i32 a, i32 b) {
    if (b == 0) {
        return Result::Err("Division by zero");
    }
    return Result::Ok(a / b);
}

// ============================================
// Memory Management
// ============================================

Vector3* fn create_vector(f32 x, f32 y, f32 z) {
    Vector3* v = new Vector3;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

void fn vector_operations() {
    Vector3* v1 = create_vector(1.0f32, 2.0f32, 3.0f32);
    
    // Use the vector
    v1.print();
    
    // Defer cleanup to end of scope
    defer delete v1;
    
    // More operations...
}

// ============================================
// Advanced Control Flow
// ============================================

void fn demonstrate_ranges() {
    print_str("Counting 1 to 10:");
    
    for (let i32 i in 1..11) {
        print_int(i);
    }
    
    print_str("\nEven numbers 0 to 20:");
    
    for (let i32 i in 0..21) {
        if (i % 2 == 0) {
            print_int(i);
        }
    }
}

// ============================================
// String Manipulation
// ============================================

void fn string_demo() {
    let string hello = "Hello";
    let string world = "World";
    let string greeting = string_concat_cstr(hello, " ");
    greeting = string_concat_cstr(greeting, world);
    
    print_str(greeting);
    
    print_str("Length: ");
    print_int(string_length(greeting));
    
    if (string_contains(greeting, "World")) {
        print_str("Contains 'World'!");
    }
}

// ============================================
// Null Safety
// ============================================

void fn null_safety_demo(string? nullable_str) {
    // Safe navigation operator
    let i32 len = nullable_str?.length ?? 0;
    
    // Null coalescing
    let string safe_str = nullable_str ?? "default";
    
    print_str(safe_str);
}

// ============================================
// Array Operations
// ============================================

void fn array_demo() {
    let i32[] numbers = [1, 2, 3, 4, 5];
    
    print_str("Original array:");
    for (let i32 i = 0; i < 5; i++) {
        print_int(numbers[i]);
    }
    
    // Map operation
    let i32[] doubled = map(numbers, |x| => x * 2);
    
    print_str("\nDoubled:");
    for (let i32 i = 0; i < 5; i++) {
        print_int(doubled[i]);
    }
}

// ============================================
// Benchmark Function
// ============================================

void fn benchmark(string name, void fn() operation) {
    let i64 start = time_now_millis();
    
    operation();
    
    let i64 end = time_now_millis();
    let i64 elapsed = end - start;
    
    print_str_nn("Benchmark '");
    print_str_nn(name);
    print_str_nn("': ");
    print_i64(elapsed);
    print_str(" ms");
}

// ============================================
// Main Entry Point
// ============================================

i32 fn main() {
    print_str("=== Magolor Language Showcase ===\n");
    
    // Basic arithmetic
    print_str("5 + 3 = ");
    print_int(5 + 3);
    
    // Calculator class
    let Calculator calc = new Calculator();
    calc.store(42);
    print_str("Stored value: ");
    print_int(calc.recall());
    
    // Fibonacci
    print_str("\nFibonacci(10) = ");
    print_int(fibonacci(10));
    
    // String operations
    print_str("\n--- String Demo ---");
    string_demo();
    
    // Array operations
    print_str("\n--- Array Demo ---");
    array_demo();
    
    // Ranges
    print_str("\n--- Range Demo ---");
    demonstrate_ranges();
    
    // Lambda usage
    print_str("\n--- Lambda Demo ---");
    let i32 result = apply_operation(10, 5, |a, b| => a * b);
    print_str("10 * 5 = ");
    print_int(result);
    
    // Benchmark
    print_str("\n--- Benchmark ---");
    benchmark("Fibonacci 20", || => {
        let i32 result = fibonacci(20);
    });
    
    // Pattern matching
    print_str("\n--- Pattern Matching ---");
    print_str("5 is: ");
    print_str(describe_number(5));
    
    print_str("\n\n=== All tests completed! ===");
    
    return 0;
}
