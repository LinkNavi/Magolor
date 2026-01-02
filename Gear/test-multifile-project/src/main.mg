using Std.IO;
using modules.utils;
using modules.math.operations;
using modules.data.processor;
using modules.advanced.algorithms.sorting;

fn main() {
    Std.print("╔═══════════════════════════════════╗\n");
    Std.print("║  Multi-File Project Test          ║\n");
    Std.print("╚═══════════════════════════════════╝\n");
    Std.print("\n");
    
    greet("Multi-File System");
    Std.print("\n");
    
    // Test nested modules
    Std.print("Testing deeply nested modules:\n");
    let sorted = bubbleSort(42);
    Std.print($"Sorted result: {sorted}\n");
    
    Std.print("\n");
    
    // Test other modules
    let sum = add(10, 5);
    Std.print($"10 + 5 = {sum}\n");
    
    let result = processData(8, 3);
    Std.print($"Process result: {result}\n");
    
    Std.print("\n");
    Std.print("✅ All multi-file tests passed!\n");
}
