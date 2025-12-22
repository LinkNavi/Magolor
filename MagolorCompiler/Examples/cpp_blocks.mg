using Std.IO;

cimport <stdio.h> (printf);
cimport <math.h> as Math;

fn demonstrateVariableSharing() {
    Std.print("=== Variable Sharing Demo ===\n\n");
    
    // Define variables in Magolor
    let mut count = 0;
    let mut sum = 0;
    let multiplier = 2;
    
    Std.print($"Initial count: {count}\n");
    Std.print($"Initial sum: {sum}\n");
    Std.print($"Multiplier: {multiplier}\n\n");
    
    // Access and modify in C++
    @cpp {
        // All Magolor variables are accessible here!
        printf("Inside C++:\n");
        printf("  count = %d\n", count);
        printf("  sum = %d\n", sum);
        printf("  multiplier = %d\n", multiplier);
        
        // Modify mutable variables
        count = 5;
        sum = 100;
        // multiplier is immutable, can't modify
        
        printf("\nAfter C++ modification:\n");
        printf("  count = %d\n", count);
        printf("  sum = %d\n", sum);
    }
    
    // Back in Magolor - see the changes!
    Std.print("\nBack in Magolor:\n");
    Std.print($"  count = {count}\n");
    Std.print($"  sum = {sum}\n");
    Std.print($"  multiplier = {multiplier}\n");
}

fn demonstrateComplexSharing() {
    Std.print("\n=== Complex Data Sharing ===\n\n");
    
    let mut result = 0.0;
    let base = 10.0;
    
    Std.print($"Computing factorial-like operations on {base}\n");
    
    @cpp {
        // Use Magolor variables in C++ calculations
        result = 1.0;
        for (int i = 1; i <= base; i++) {
            result *= i;
        }
        printf("C++ computed: %f\n", result);
    }
    
    Std.print($"Magolor received: {result}\n");
    
    // Use the result in more calculations
    let sqrt_result = Math::sqrt(result);
    Std.print($"Square root: {sqrt_result}\n");
}

fn demonstrateStringSharing() {
    Std.print("\n=== String Sharing ===\n\n");
    
    // Note: Magolor strings are std::string in C++
    let mut message = "Hello";
    
    Std.print($"Magolor: {message}\n");
    
    @cpp {
        // Access and modify the string
        std::cout << "C++: " << message << "\n";
        message += " from C++!";
        std::cout << "C++ modified: " << message << "\n";
    }
    
    Std.print($"Magolor sees: {message}\n");
}

fn demonstrateArraySharing() {
    Std.print("\n=== Array Sharing ===\n\n");
    
    // Magolor arrays are std::vector in C++
    let mut numbers = [1, 2, 3, 4, 5];
    
    Std.print("Original array: ");
    for (n in numbers) {
        Std.print($"{n} ");
    }
    Std.print("\n");
    
    @cpp {
        // Access the vector in C++
        std::cout << "C++ sees " << numbers.size() << " elements\n";
        
        // Add more elements
        numbers.push_back(6);
        numbers.push_back(7);
        
        // Sort them
        std::sort(numbers.begin(), numbers.end(), std::greater<int>());
        
        std::cout << "C++ sorted (descending): ";
        for (int n : numbers) {
            std::cout << n << " ";
        }
        std::cout << "\n";
    }
    
    Std.print("Back in Magolor: ");
    for (n in numbers) {
        Std.print($"{n} ");
    }
    Std.print("\n");
}

fn demonstrateMixedCalculation() {
    Std.print("\n=== Mixed Magolor/C++ Calculation ===\n\n");
    
    let mut temperature_celsius = 25.0;
    let mut temperature_fahrenheit = 0.0;
    
    Std.print($"Temperature: {temperature_celsius}°C\n");
    
    // Convert using C++
    @cpp {
        temperature_fahrenheit = (temperature_celsius * 9.0 / 5.0) + 32.0;
        printf("C++ converted: %.1f°F\n", temperature_fahrenheit);
    }
    
    // Use in Magolor
    Std.print($"Magolor: {temperature_celsius}°C = {temperature_fahrenheit}°F\n");
    
    // Check if it's hot (in Magolor)
    if (temperature_fahrenheit > 80.0) {
        Std.print("It's hot!\n");
    } else {
        Std.print("It's pleasant!\n");
    }
}

fn main() {
    Std.print("╔════════════════════════════════════╗\n");
    Std.print("║  Variable Sharing Demo             ║\n");
    Std.print("║  Magolor ↔ C++ @cpp blocks         ║\n");
    Std.print("╚════════════════════════════════════╝\n\n");
    
    demonstrateVariableSharing();
    demonstrateComplexSharing();
    demonstrateStringSharing();
    demonstrateArraySharing();
    demonstrateMixedCalculation();
    
    Std.print("\n✅ Variable sharing working perfectly!\n");
    Std.print("\n💡 Key Points:\n");
    Std.print("   • Magolor variables are accessible in @cpp blocks\n");
    Std.print("   • Changes in @cpp persist back to Magolor\n");
    Std.print("   • `mut` variables can be modified\n");
    Std.print("   • Immutable variables are read-only\n");
    Std.print("   • Arrays → std::vector, Strings → std::string\n");
}
