using Std.IO;
using api.handlers;

fn main() {
    Std.print("Test 4: Complex Module Graph\n");
    Std.print("==============================\n");
    Std.print("Testing module dependency chain:\n");
    Std.print("main -> api -> utils -> core\n\n");
    
    let result = handleRequest(5, 3);
    Std.print($"\nFinal result: {result}\n");
    
    Std.print("\n✅ Test 4 passed!\n\n");
}
