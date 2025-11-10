// test.mg - Sample Magolor program

// Simple factorial function
i32 fn factorial(i32 n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

// Fibonacci function
i32 fn fibonacci(i32 n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

// Sum function with a loop
i32 fn sum(i32 n) {
    let mut i32 result = 0;
    let mut i32 i = 1;
    
    while (i <= n) {
        result = result + i;
        i = i + 1;
    }
    
    return result;
}

// Main entry point
i32 fn main() {
    let i32 x = 5;
    let i32 fact = factorial(x);
    let i32 fib = fibonacci(10);
    let i32 total = sum(100);
    
    return fact;
}

// Class example
class Calculator {
    private static i32 lastResult = 0;
    
    public static i32 fn add(i32 a, i32 b) {
        return a + b;
    }
    
    public static i32 fn multiply(i32 a, i32 b) {
        let mut i32 result = 0;
        let mut i32 i = 0;
        
        while (i < b) {
            result = result + a;
            i = i + 1;
        }
        
        return result;
    }
}
