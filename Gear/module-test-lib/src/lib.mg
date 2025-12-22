using Std.IO;

fn processNumber(n: int) -> int {
    Std.print($"testlib: processing {n}\n");
    return n * 2;
}

fn greet(name: string) {
    Std.print($"testlib: Hello, {name}!\n");
}
