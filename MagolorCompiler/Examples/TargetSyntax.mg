using Std.IO;
using Std.Parse;
using Std.Option;

// --- Helper: robust iterative integer input ------------------
// avoids recursion / stack growth, friendly prompt loop
fn readInt(prompt: string) -> int {
    while (true) {
        Std.print(prompt);
        let line = Std.readLine();

        match Std.parseInt(line) {
            Some(v) => return v,
            None => Std.print("That wasn't a number. Try again.\n")
        }
    }
}

// --- Example: returns a closure (multi-line form) -----------
fn makeAdder(x: int) -> fn(int) -> int {
    // multi-line closure syntax (captures `x`)
    return fn(y: int) -> int {
        let sum = x + y;
        return sum;
    };
}

// --- Optional: one-line arrow closure equivalent -------------
// fn makeAdder(x: int) -> fn(int) -> int => (y) => x + y;

fn main() {
    let base = readInt("Enter base number: ");
    let add = makeAdder(base);         // add is fn(int)->int

    let toAdd = readInt("Enter another: ");
    let result = add(toAdd);           // call the returned function

    // Interpolated string automatically formats values
    Std.print($"Result: {result}\n");
}
