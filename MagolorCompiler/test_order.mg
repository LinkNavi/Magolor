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
