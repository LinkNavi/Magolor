// Std.IO - Input/Output operations

// ============================================================================
// Console Output
// ============================================================================

pub fn print(s: string) {
    @cpp {
        std::cout << s;
    }
}

pub fn println(s: string) {
    @cpp {
        std::cout << s << std::endl;
    }
}

pub fn printlnEmpty() {
    @cpp {
        std::cout << std::endl;
    }
}

pub fn eprint(s: string) {
    @cpp {
        std::cerr << s;
    }
}

pub fn eprintln(s: string) {
    @cpp {
        std::cerr << s << std::endl;
    }
}

pub fn printInt(n: int) {
    @cpp {
        std::cout << n;
    }
}

pub fn printlnInt(n: int) {
    @cpp {
        std::cout << n << std::endl;
    }
}

pub fn printFloat(n: float) {
    @cpp {
        std::cout << n;
    }
}

pub fn printlnFloat(n: float) {
    @cpp {
        std::cout << n << std::endl;
    }
}

pub fn printBool(b: bool) {
    @cpp {
        std::cout << (b ? "true" : "false");
    }
}

pub fn printlnBool(b: bool) {
    @cpp {
        std::cout << (b ? "true" : "false") << std::endl;
    }
}

// ============================================================================
// Console Input
// ============================================================================

pub fn readLine() -> string {
    @cpp {
        std::string line;
        std::getline(std::cin, line);
        return line;
    }
}

pub fn prompt(message: string) -> string {
    @cpp {
        std::cout << message;
        std::string line;
        std::getline(std::cin, line);
        return line;
    }
}

pub fn readAll() -> string {
    @cpp {
        std::string content, line;
        while (std::getline(std::cin, line)) {
            content += line + "\n";
        }
        return content;
    }
}

pub fn readChar() -> string {
    @cpp {
        char c;
        std::cin >> c;
        return std::string(1, c);
    }
}

pub fn hasInput() -> bool {
    @cpp {
        return std::cin.peek() != EOF;
    }
}

pub fn readInt() -> Option<int> {
    @cpp {
        std::string line;
        std::getline(std::cin, line);
        try {
            return std::stoll(line);
        } catch (...) {
            return std::nullopt;
        }
    }
}

pub fn readFloat() -> Option<float> {
    @cpp {
        std::string line;
        std::getline(std::cin, line);
        try {
            return std::stod(line);
        } catch (...) {
            return std::nullopt;
        }
    }
}

// ============================================================================
// Formatting helpers
// ============================================================================

pub fn padLeft(s: string, width: int, padChar: string) -> string {
    @cpp {
        if (static_cast<int64_t>(s.length()) >= width || padChar.empty()) return s;
        std::string padding;
        int64_t needed = width - s.length();
        for (int64_t i = 0; i < needed; i++) {
            padding += padChar[0];
        }
        return padding + s;
    }
}

pub fn padRight(s: string, width: int, padChar: string) -> string {
    @cpp {
        if (static_cast<int64_t>(s.length()) >= width || padChar.empty()) return s;
        std::string result = s;
        int64_t needed = width - s.length();
        for (int64_t i = 0; i < needed; i++) {
            result += padChar[0];
        }
        return result;
    }
}

pub fn center(s: string, width: int, padChar: string) -> string {
    @cpp {
        if (static_cast<int64_t>(s.length()) >= width || padChar.empty()) return s;
        int64_t totalPad = width - s.length();
        int64_t leftPad = totalPad / 2;
        int64_t rightPad = totalPad - leftPad;
        std::string left, right;
        for (int64_t i = 0; i < leftPad; i++) left += padChar[0];
        for (int64_t i = 0; i < rightPad; i++) right += padChar[0];
        return left + s + right;
    }
}

// ============================================================================
// Output stream abstraction - FIXED: Class methods need proper @cpp blocks
// ============================================================================

pub class Writer {
    pub buffer: string;
    pub autoFlush: bool;
    
    pub fn create() {
        @cpp {
            this->buffer = "";
            this->autoFlush = false;
        }
    }
    
    pub fn write(s: string) {
        @cpp {
            this->buffer = this->buffer + s;
            if (this->autoFlush) {
                std::cout << this->buffer;
                this->buffer = "";
            }
        }
    }
    
    pub fn writeLine(s: string) {
        @cpp {
            this->buffer = this->buffer + s + "\n";
            if (this->autoFlush) {
                std::cout << this->buffer;
                this->buffer = "";
            }
        }
    }
    
    pub fn flush() {
        @cpp {
            std::cout << this->buffer;
            this->buffer = "";
        }
    }
    
    pub fn toString() -> string {
        @cpp {
            return this->buffer;
        }
    }
    
    pub fn clear() {
        @cpp {
            this->buffer = "";
        }
    }
}

pub fn newWriter() -> Writer {
    @cpp {
        Writer w;
        w.buffer = "";
        w.autoFlush = false;
        return w;
    }
}
