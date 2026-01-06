// Std.IO - Input/Output operations
// Provides console I/O and basic file operations

using Std.Core.Prelude;

// ============================================================================
// Console Output
// ============================================================================

// Print without newline
pub fn print(s: string) {
    @cpp {
        std::cout << s;
    }
}

// Print with newline
pub fn println(s: string) {
    @cpp {
        std::cout << s << std::endl;
    }
}

// Print to stderr
pub fn eprint(s: string) {
    @cpp {
        std::cerr << s;
    }
}

// Print to stderr with newline
pub fn eprintln(s: string) {
    @cpp {
        std::cerr << s << std::endl;
    }
}

// Formatted print (uses string interpolation internally)
pub fn printf(format: string, args: Array<string>) {
    let mut result = format;
    for (i in 0..args.length()) {
        let placeholder = "{" + toString(i) + "}";
        result = result.replace(placeholder, args[i]);
    }
    print(result);
}

// Debug print - includes type info
pub fn dbg(value: any, label: string) {
    @cpp {
        std::cerr << "[DEBUG " << label << "] " << value << std::endl;
    }
}

// ============================================================================
// Console Input
// ============================================================================

// Read a line from stdin
pub fn readLine() -> string {
    @cpp {
        std::string line;
        std::getline(std::cin, line);
        return line;
    }
}

// Read a line with prompt
pub fn prompt(message: string) -> string {
    print(message);
    return readLine();
}

// Read all input until EOF
pub fn readAll() -> string {
    @cpp {
        std::string content, line;
        while (std::getline(std::cin, line)) {
            content += line + "\n";
        }
        return content;
    }
}

// Read a single character
pub fn readChar() -> string {
    @cpp {
        char c;
        std::cin >> c;
        return std::string(1, c);
    }
}

// Check if stdin has data available
pub fn hasInput() -> bool {
    @cpp {
        return std::cin.peek() != EOF;
    }
}

// ============================================================================
// Formatting helpers
// ============================================================================

// Pad string on left
pub fn padLeft(s: string, width: int, padChar: string) -> string {
    let len = s.length();
    if (len >= width) { return s; }
    let padding = "";
    for (i in 0..(width - len)) {
        padding = padding + padChar;
    }
    return padding + s;
}

// Pad string on right
pub fn padRight(s: string, width: int, padChar: string) -> string {
    let len = s.length();
    if (len >= width) { return s; }
    let padding = "";
    for (i in 0..(width - len)) {
        padding = padding + padChar;
    }
    return s + padding;
}

// Center string
pub fn center(s: string, width: int, padChar: string) -> string {
    let len = s.length();
    if (len >= width) { return s; }
    let totalPad = width - len;
    let leftPad = totalPad / 2;
    let rightPad = totalPad - leftPad;
    let left = "";
    let right = "";
    for (i in 0..leftPad) { left = left + padChar; }
    for (i in 0..rightPad) { right = right + padChar; }
    return left + s + right;
}

// ============================================================================
// Output stream abstraction
// ============================================================================

pub class Writer {
    pub buffer: string;
    pub autoFlush: bool;
    
    pub fn create() {
        this.buffer = "";
        this.autoFlush = false;
    }
    
    pub fn write(s: string) {
        this.buffer = this.buffer + s;
        if (this.autoFlush) {
            this.flush();
        }
    }
    
    pub fn writeLine(s: string) {
        this.write(s + "\n");
    }
    
    pub fn flush() {
        print(this.buffer);
        this.buffer = "";
    }
    
    pub fn toString() -> string {
        return this.buffer;
    }
}

// Create a buffered writer
pub fn newWriter() -> Writer {
    return new Writer();
}

// Create an auto-flushing writer
pub fn newAutoWriter() -> Writer {
    let mut w = new Writer();
    w.autoFlush = true;
    return w;
}
