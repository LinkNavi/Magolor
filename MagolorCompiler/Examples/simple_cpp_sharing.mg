using Std.IO;

cimport <stdio.h> (printf);

fn demonstrateBasicSharing() {
    Std.print("=== Basic Variable Sharing ===\n\n");
    
    // Magolor variables
    let mut count = 0;
    let mut sum = 0;
    let multiplier = 2;
    
    Std.print($"Before C++: count={count}, sum={sum}\n");
    
    // Modify in C++
    @cpp {
        printf("Inside C++: count=%d, sum=%d, multiplier=%d\n", count, sum, multiplier);
        
        // Modify mutable variables
        count = 10;
        sum = 100;
        
        printf("After modification: count=%d, sum=%d\n", count, sum);
    }
    
    // See changes in Magolor
    Std.print($"Back in Magolor: count={count}, sum={sum}\n");
}

fn demonstrateCalculation() {
    Std.print("\n=== C++ Calculation ===\n\n");
    
    let mut result = 0;
    let limit = 10;
    
    Std.print($"Calculating sum from 1 to {limit}...\n");
    
    @cpp {
        // Calculate in C++
        result = 0;
        for (int i = 1; i <= limit; i++) {
            result += i;
        }
        printf("C++ computed result: %d\n", result);
    }
    
    Std.print($"Magolor received: {result}\n");
}

fn demonstrateStrings() {
    Std.print("\n=== String Manipulation ===\n\n");
    
    let mut message = "Hello";
    
    Std.print($"Original: '{message}'\n");
    
    @cpp {
        // Strings are std::string in C++
        std::cout << "C++ sees: '" << message << "'\n";
        message += " World";
        message += "!";
        std::cout << "C++ modified: '" << message << "'\n";
    }
    
    Std.print($"Magolor sees: '{message}'\n");
}

fn demonstrateFloats() {
    Std.print("\n=== Float Operations ===\n\n");
    
    let mut temperature_c = 25.0;
    let mut temperature_f = 0.0;
    
    Std.print($"Temperature: {temperature_c}°C\n");
    
    @cpp {
        // Convert Celsius to Fahrenheit
        temperature_f = (temperature_c * 9.0 / 5.0) + 32.0;
        printf("C++ converted: %.1f°F\n", temperature_f);
    }
    
    Std.print($"Result: {temperature_c}°C = {temperature_f}°F\n");
}

fn main() {
    Std.print("╔════════════════════════════════════╗\n");
    Std.print("║  Variable Sharing Demo             ║\n");
    Std.print("║  Magolor ↔ C++ @cpp blocks         ║\n");
    Std.print("╚════════════════════════════════════╝\n\n");
    
    demonstrateBasicSharing();
    demonstrateCalculation();
    demonstrateStrings();
    demonstrateFloats();
    
    Std.print("\n✅ Variable sharing works!\n");
    Std.print("\n💡 How it works:\n");
    Std.print("   • All Magolor variables accessible in @cpp\n");
    Std.print("   • Changes in @cpp persist to Magolor\n");
    Std.print("   • 'let mut' = modifiable, 'let' = read-only\n");
}
