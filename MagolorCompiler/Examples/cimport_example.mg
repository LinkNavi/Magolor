// cimport_example.mg - Demonstrating C header imports
// This shows how to use cimport for C interop with LSP support

// Import C headers - LSP will provide completions for these
@cimport { stdio.h stdlib.h string.h }

// Or import individual headers
cimport "math.h"
cimport <time.h>

// Link flags for the compiler
@link { -lm }

// C++ includes for codegen
@include { <cstdio> <cstdlib> <cstring> <cmath> <ctime> }

using Std.IO;

// Example: Using C's printf
pub fn printFormatted(fmt: string, value: int) {
    @cpp {
        printf(fmt.c_str(), value);
    }
}

// Example: Using C's math functions
pub fn calculateSqrt(x: float) -> float {
    @cpp {
        return sqrt(x);
    }
}

pub fn calculatePow(base: float, exp: float) -> float {
    @cpp {
        return pow(base, exp);
    }
}

pub fn calculateSin(x: float) -> float {
    @cpp {
        return sin(x);
    }
}

// Example: Using C's time functions
pub fn getCurrentTime() -> int {
    @cpp {
        return static_cast<int64_t>(time(nullptr));
    }
}

pub fn getRandomNumber(max: int) -> int {
    @cpp {
        static bool seeded = false;
        if (!seeded) {
            srand(time(nullptr));
            seeded = true;
        }
        return rand() % max;
    }
}

// Example: Using C's string functions
pub fn stringLength(s: string) -> int {
    @cpp {
        return strlen(s.c_str());
    }
}

pub fn compareStrings(a: string, b: string) -> int {
    @cpp {
        return strcmp(a.c_str(), b.c_str());
    }
}

// Example: Memory allocation (be careful with this!)
pub class CBuffer {
    pub ptr: int;  // Actually a void* stored as int
    pub size: int;
    
    pub fn create() {
        this.ptr = 0;
        this.size = 0;
    }
    
    pub static fn allocate(bytes: int) -> CBuffer {
        let buf = new CBuffer();
        buf.size = bytes;
        @cpp {
            buf.ptr = reinterpret_cast<int64_t>(malloc(bytes));
        }
        return buf;
    }
    
    pub fn free() {
        @cpp {
            if (this->ptr != 0) {
                ::free(reinterpret_cast<void*>(this->ptr));
                this->ptr = 0;
            }
        }
    }
    
    pub fn isValid() -> bool {
        return this.ptr != 0;
    }
}

// Example: File operations using C's stdio
pub class CFile {
    pub handle: int;  // FILE* stored as int
    pub path: string;
    
    pub fn create() {
        this.handle = 0;
        this.path = "";
    }
    
    pub static fn open(path: string, mode: string) -> Option<CFile> {
        @cpp {
            FILE* f = fopen(path.c_str(), mode.c_str());
            if (f == nullptr) {
                return std::nullopt;
            }
            
            CFile file;
            file.handle = reinterpret_cast<int64_t>(f);
            file.path = path;
            return std::make_optional(file);
        }
    }
    
    pub fn close() {
        @cpp {
            if (this->handle != 0) {
                fclose(reinterpret_cast<FILE*>(this->handle));
                this->handle = 0;
            }
        }
    }
    
    pub fn write(data: string) -> int {
        @cpp {
            if (this->handle == 0) return 0;
            FILE* f = reinterpret_cast<FILE*>(this->handle);
            return fputs(data.c_str(), f);
        }
    }
    
    pub fn readLine() -> string {
        @cpp {
            if (this->handle == 0) return "";
            FILE* f = reinterpret_cast<FILE*>(this->handle);
            char buffer[4096];
            if (fgets(buffer, sizeof(buffer), f) != nullptr) {
                return std::string(buffer);
            }
            return "";
        }
    }
    
    pub fn isEOF() -> bool {
        @cpp {
            if (this->handle == 0) return true;
            FILE* f = reinterpret_cast<FILE*>(this->handle);
            return feof(f) != 0;
        }
    }
}

// Demo main
fn main() {
    println("=== CImport Demo ===");
    
    // Math functions
    let sqrtVal = calculateSqrt(16.0);
    println($"sqrt(16) = {sqrtVal}");
    
    let powVal = calculatePow(2.0, 10.0);
    println($"2^10 = {powVal}");
    
    // Time functions
    let now = getCurrentTime();
    println($"Current timestamp: {now}");
    
    let random = getRandomNumber(100);
    println($"Random number (0-99): {random}");
    
    // String functions
    let len = stringLength("Hello, World!");
    println($"String length: {len}");
    
    // Printf style
    printFormatted("Formatted: %d\n", 42);
    
    // File operations
    let fileOpt = CFile.open("test.txt", "w");
    if (isSome(fileOpt)) {
        let file = unwrap(fileOpt);
        file.write("Hello from Magolor!\n");
        file.write("Using C's stdio.\n");
        file.close();
        println("File written successfully!");
    }
    
    // Buffer allocation
    let buf = CBuffer.allocate(1024);
    if (buf.isValid()) {
        println($"Allocated {buf.size} bytes");
        buf.free();
        println("Buffer freed");
    }
    
    println("=== Demo Complete ===");
}
