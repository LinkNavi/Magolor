use Console;
use Math;

namespace Calculator {
    public class MathOperations {
        private static i32 callCount = 0;
        
        public static i32 fn add(i32 a, i32 b) {
            callCount++;
            return a + b;
        }
        
        public static f64 fn divide(f64 a, f64 b) {
            if (b == 0.0) {
                console.print("Error: Division by zero");
                return 0.0;
            }
            return a / b;
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
    
    // Basic operations
    let result1 = Calculator.MathOperations.add(5, 3);
    console.print("5 + 3 = " + result1);
    
    let result2 = Calculator.MathOperations.divide(10.0, 2.0);
    console.print("10 / 2 = " + result2);
    
    // Factorial
    let fact = Calculator.MathOperations.factorial(5);
    console.print("5! = " + fact);
    
    // Array processing
    let i32[] numbers = [1, 2, 3, 4, 5];
    let mut sum = 0;
    
    foreach (i32 num in numbers) {
        sum += num;
    }
    
    console.print("Sum of array: " + sum);
    
    // Control flow
    for (let i = 0; i < 3; i++) {
        console.print("Loop iteration: " + i);
    }
    
    // Match expression
    let day = 3;
    match day {
        1 => console.print("Monday"),
        2 => console.print("Tuesday"),
        3 => console.print("Wednesday"),
        _ => console.print("Other day")
    }
    
    // Show call count
    let count = Calculator.MathOperations.getCallCount();
    console.print("Total math operations: " + count);
}
