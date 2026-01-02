using Std.IO;
using modules.math.operations;

pub fn processData(x: int, y: int) -> int {
    Std.print("data.processor: processing\n");
    let sum = add(x, y);
    let product = multiply(x, y);
    return sum + product;
}

pub fn analyze(values: int) -> int {
    Std.print($"data.processor: analyzing {values}\n");
    return factorial(values);
}
