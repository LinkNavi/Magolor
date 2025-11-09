// test_simple.mg - Simple test to isolate the parameter issue

use Console;

namespace Test {
    public class Simple {
        // Test 1: Just return a parameter
        public static i32 fn returnParam(i32 x) {
            return x;
        }
        
        // Test 2: Simple add
        public static i32 fn addTwo(i32 a, i32 b) {
            return a + b;
        }
    }
}

void fn main() {
    console.print("Test 1: Return parameter");
    let result1 = Test.Simple.returnParam(42);
    console.print("Result should be 42:");
    // result1 should be 42
    
    console.print("Test 2: Add two numbers");
    let result2 = Test.Simple.addTwo(5, 3);
    console.print("Result should be 8:");
    // result2 should be 8
    
    console.print("Done");
}