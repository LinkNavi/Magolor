// test_with_print.mg - Extended print + recursion test

i32 fn factorial(i32 n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}



f32 fn divide(i32 a, i32 b) {
    let f32 af = a * 1.0;
    let f32 bf = b * 1.0;
    return af / bf;
}



i32 fn main() {
    let i32 x = 5;
    let i32 fact = factorial(x);

    print_str("=== Magolor Compiler Test ===\n");
    print_str("Calculating factorial of: ");
    print_int(x);
    print_str("\nResult: ");
    print_int(fact);
    print_str("\n");

    let f32 ratio = divide(fact, x);
    print_str("Factorial divided by input (as float): ");
    print_f32(ratio);
    print_str("\n");

    if (fact > 100) {
        print_str("That's a big number!\n");
    } else {
        print_str("That's pretty small.\n");
    }

    print_str("=== End of test ===\n");
    return 0;
}
