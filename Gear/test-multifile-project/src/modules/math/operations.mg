using Std.IO;

pub fn add(a: int, b: int) -> int {
    Std.print("math.operations: adding\n");
    return a + b;
}

pub fn multiply(a: int, b: int) -> int {
    Std.print("math.operations: multiplying\n");
    return a * b;
}

pub fn factorial(n: int) -> int {
    if (n <= 1) {
        return 1;
    }
    return multiply(n, factorial(n - 1));
}
