using Std.IO;

fn demonstrateBasicSharing() {
    Std.IO.println("=== Basic Variable Sharing ===\n");
    
    // Magolor variables
    let mut count = 0;
    let mut sum = 0;
    let multiplier = 2;
    
    Std.IO.print("Before: count=");
    Std.IO.print(count);
    Std.IO.print(", sum=");
    Std.IO.print(sum);
    Std.IO.println("");
    
    // Modify variables
    count = 10;
    sum = 100;
    
    Std.IO.print("After: count=");
    Std.IO.print(count);
    Std.IO.print(", sum=");
    Std.IO.print(sum);
    Std.IO.println("");
}

fn demonstrateCalculation() {
    Std.IO.println("\n=== Calculation ===\n");
    
    let mut result = 0;
    let limit = 10;
    
    Std.IO.print("Calculating sum from 1 to ");
    Std.IO.print(limit);
    Std.IO.println("...");
    
    // Calculate in pure Magolor
    let mut i = 1;
    while (i <= limit) {
        result = result + i;
        i = i + 1;
    }
    
    Std.IO.print("Result: ");
    Std.IO.print(result);
    Std.IO.println("");
}

fn demonstrateStrings() {
    Std.IO.println("\n=== String Operations ===\n");
    
    let mut message = "Hello";
    
    Std.IO.print("Original: '");
    Std.IO.print(message);
    Std.IO.println("'");
    
    message = message + " World";
    message = message + "!";
    
    Std.IO.print("Modified: '");
    Std.IO.print(message);
    Std.IO.println("'");
}

fn demonstrateFloats() {
    Std.IO.println("\n=== Float Operations ===\n");
    
    let temperature_c = 25.0;
    let temperature_f = (temperature_c * 9.0 / 5.0) + 32.0;
    
    Std.IO.print("Temperature: ");
    Std.IO.print(temperature_c);
    Std.IO.println("°C");
    
    Std.IO.print("Result: ");
    Std.IO.print(temperature_c);
    Std.IO.print("°C = ");
    Std.IO.print(temperature_f);
    Std.IO.println("°F");
}

fn main() {
    Std.IO.println("╔════════════════════════════════════╗");
    Std.IO.println("║  Pure Magolor Demo                ║");
    Std.IO.println("║  No C++ needed!                   ║");
    Std.IO.println("╚════════════════════════════════════╝\n");
    
    demonstrateBasicSharing();
    demonstrateCalculation();
    demonstrateStrings();
    demonstrateFloats();
    
    Std.IO.println("\n✅ Pure Magolor works!");
}
