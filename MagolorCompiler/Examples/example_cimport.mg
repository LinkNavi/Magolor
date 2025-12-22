using Std.IO;

// Import C standard library headers
cimport <stdio.h> (printf);
cimport <math.h> as Math;
cimport <string.h> (strlen, strcmp);

fn calculateCircleArea(radius: float) -> float {
    // Use math functions through the Math namespace
    let pi = Math::acos(-1.0);
    return pi * Math::pow(radius, 2.0);
}

fn demonstrateCStrings() {
    Std.print("\n=== C String Functions ===\n");
    
    // Note: In real code, you'd use proper string handling
    // This is just to demonstrate C function calls work
    let msg = "Hello from Magolor!";
    Std.print($"Message: {msg}\n");
}

fn main() {
    Std.print("╔════════════════════════════════════╗\n");
    Std.print("║  C/C++ Interop Demo                ║\n");
    Std.print("╚════════════════════════════════════╝\n\n");
    
    // Math functions through namespace
    Std.print("=== Math Functions ===\n");
    let area = calculateCircleArea(5.0);
    Std.print($"Circle area (r=5): {area}\n");
    
    let sqrtVal = Math::sqrt(16.0);
    Std.print($"Square root of 16: {sqrtVal}\n");
    
    let sinVal = Math::sin(3.14159 / 2.0);
    Std.print($"sin(π/2): {sinVal}\n");
    
    // Direct C function call
    Std.print("\n=== Direct C Calls ===\n");
    printf("Hello from printf!\n");
    printf("Formatted output: %d + %d = %d\n", 5, 3, 8);
    
    // String functions
    demonstrateCStrings();
    
    Std.print("\n✅ C/C++ interop working!\n");
}
