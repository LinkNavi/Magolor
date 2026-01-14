// Std.IO - Pure Magolor I/O Module
// Comprehensive input/output functionality
// Functions are implemented in LLVM runtime with polymorphic dispatch

// ============================================================================
// Console Output Functions
// ============================================================================

// Print a value to stdout without newline
// Polymorphic - handles String, Int, Float, Bool automatically
pub fn print(value: String) {
    // Implementation provided by LLVM runtime
}

// Print a value to stdout with newline
// Polymorphic - handles String, Int, Float, Bool automatically
pub fn println(value: String) {
    // Implementation provided by LLVM runtime
}

// Print an error message to stderr
pub fn eprint(value: String) {
    // Implementation provided by LLVM runtime
}

// Print an error message to stderr with newline
pub fn eprintln(value: String) {
    // Implementation provided by LLVM runtime
}

// ============================================================================
// Console Input Functions
// ============================================================================

// Read a line from stdin (removes trailing newline)
pub fn readLine() -> String {
    // Implementation provided by LLVM runtime
    return "";
}

// Read a single character from stdin
pub fn readChar() -> String {
    // Implementation provided by LLVM runtime
    return "";
}

// Read until EOF and return as string
pub fn readAll() -> String {
    // Implementation provided by LLVM runtime
    return "";
}

// ============================================================================
// Formatted Output Functions
// ============================================================================

// Print with format string (similar to printf)
pub fn printf(format: String, args: Array<String>) {
    // Implementation provided by LLVM runtime
}

// Print formatted string with newline
pub fn printfln(format: String, args: Array<String>) {
    // Implementation provided by LLVM runtime
}

// ============================================================================
// Prompt Functions
// ============================================================================

// Display a prompt and read user input
pub fn prompt(message: String) -> String {
    print(message);
    return readLine();
}

// Display a prompt with options and read input
pub fn promptChoice(message: String, options: Array<String>) -> Int {
    println(message);
    let mut i = 0;
    while i < length(options) {
        print("  ");
        print(toString(i + 1));
        print(". ");
        println(options[i]);
        i = i + 1;
    }
    print("Choice: ");
    return toInt(readLine());
}

// Confirm yes/no prompt
pub fn confirm(message: String) -> Bool {
    print(message);
    print(" (y/n): ");
    let response = readLine();
    return response == "y" || response == "Y" || response == "yes" || response == "YES";
}

// ============================================================================
// Utility Functions
// ============================================================================

// Clear the console screen
pub fn clear() {
    // Implementation provided by LLVM runtime
}

// Flush stdout buffer
pub fn flush() {
    // Implementation provided by LLVM runtime
}

// Check if stdin has data available
pub fn hasInput() -> Bool {
    // Implementation provided by LLVM runtime
    return false;
}

// ============================================================================
// Color/Style Output (ANSI Escape Codes)
// ============================================================================

// Print with color
pub fn printColor(value: String, color: String) {
    // Implementation provided by LLVM runtime
}

// Print bold text
pub fn printBold(value: String) {
    // Implementation provided by LLVM runtime
}

// Print underlined text
pub fn printUnderline(value: String) {
    // Implementation provided by LLVM runtime
}

// Reset terminal formatting
pub fn resetFormat() {
    // Implementation provided by LLVM runtime
}
