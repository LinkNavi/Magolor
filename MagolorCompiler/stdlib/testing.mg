// Std.Testing - Unit testing framework
// Simple but powerful testing utilities

using Std.Core.Prelude;
using Std.IO;
using Std.Time;

// ============================================================================
// Test result types
// ============================================================================

pub class TestResult {
    pub name: string;
    pub passed: bool;
    pub message: string;
    pub durationMs: int;
}

pub class TestSuite {
    pub name: string;
    pub tests: Array<TestResult>;
    pub passed: int;
    pub failed: int;
    pub skipped: int;
    pub totalTime: int;
    
    pub fn create(suiteName: string) {
        this.name = suiteName;
        this.tests = [];
        this.passed = 0;
        this.failed = 0;
        this.skipped = 0;
        this.totalTime = 0;
    }
}

// Global test suite for simple use
let mut currentSuite: TestSuite = new TestSuite("default");

// ============================================================================
// Test registration and running
// ============================================================================

pub fn suite(name: string) -> TestSuite {
    let mut s = new TestSuite(name);
    return s;
}

pub fn test(name: string, testFn: fn() -> bool) {
    let start = nowMillis();
    let mut result = new TestResult();
    result.name = name;
    
    @cpp {
        try {
            result.passed = testFn();
            result.message = result.passed ? "" : "Test assertion failed";
        } catch (const std::exception& e) {
            result.passed = false;
            result.message = e.what();
        } catch (...) {
            result.passed = false;
            result.message = "Unknown exception";
        }
    }
    
    result.durationMs = nowMillis() - start;
    push(currentSuite.tests, result);
    
    if (result.passed) {
        currentSuite.passed = currentSuite.passed + 1;
        print("  \033[32m✓\033[0m " + name);
    } else {
        currentSuite.failed = currentSuite.failed + 1;
        print("  \033[31m✗\033[0m " + name);
        if (result.message != "") {
            println("    → " + result.message);
        }
    }
    println(" (" + toString(result.durationMs) + "ms)");
    
    currentSuite.totalTime = currentSuite.totalTime + result.durationMs;
}

pub fn testSuite(name: string, setupFn: fn()) {
    println("\n\033[1m" + name + "\033[0m");
    currentSuite = new TestSuite(name);
    setupFn();
    printSummary();
}

pub fn printSummary() {
    println("");
    let total = currentSuite.passed + currentSuite.failed;
    
    if (currentSuite.failed == 0) {
        println("\033[32m" + toString(currentSuite.passed) + "/" + toString(total) + 
                " tests passed\033[0m (" + toString(currentSuite.totalTime) + "ms)");
    } else {
        println("\033[31m" + toString(currentSuite.failed) + " failed\033[0m, " +
                "\033[32m" + toString(currentSuite.passed) + " passed\033[0m (" +
                toString(currentSuite.totalTime) + "ms)");
    }
}

// ============================================================================
// Assertions
// ============================================================================

pub fn assertTrue(condition: bool) -> bool {
    return condition;
}

pub fn assertFalse(condition: bool) -> bool {
    return !condition;
}

pub fn assertEqual(expected: any, actual: any) -> bool {
    @cpp {
        return expected == actual;
    }
}

pub fn assertNotEqual(a: any, b: any) -> bool {
    @cpp {
        return a != b;
    }
}

pub fn assertNull(value: Option<any>) -> bool {
    return isNone(value);
}

pub fn assertNotNull(value: Option<any>) -> bool {
    return isSome(value);
}

pub fn assertApprox(expected: float, actual: float, epsilon: float) -> bool {
    @cpp {
        return std::abs(expected - actual) < epsilon;
    }
}

pub fn assertGreater(a: int, b: int) -> bool {
    return a > b;
}

pub fn assertLess(a: int, b: int) -> bool {
    return a < b;
}

pub fn assertContains(haystack: string, needle: string) -> bool {
    return contains(haystack, needle);
}

pub fn assertStartsWith(s: string, prefix: string) -> bool {
    return startsWith(s, prefix);
}

pub fn assertEndsWith(s: string, suffix: string) -> bool {
    return endsWith(s, suffix);
}

pub fn assertThrows(testFn: fn()) -> bool {
    @cpp {
        try {
            testFn();
            return false;  // Should have thrown
        } catch (...) {
            return true;   // Exception caught as expected
        }
    }
}

pub fn assertArrayEqual(a: Array<any>, b: Array<any>) -> bool {
    @cpp {
        if (a.size() != b.size()) return false;
        for (size_t i = 0; i < a.size(); i++) {
            if (a[i] != b[i]) return false;
        }
        return true;
    }
}

// ============================================================================
// Skip and pending tests
// ============================================================================

pub fn skip(name: string, reason: string) {
    let mut result = new TestResult();
    result.name = name;
    result.passed = true;  // Skipped tests don't count as failures
    result.message = "SKIPPED: " + reason;
    result.durationMs = 0;
    
    push(currentSuite.tests, result);
    currentSuite.skipped = currentSuite.skipped + 1;
    
    println("  \033[33m○\033[0m " + name + " (skipped: " + reason + ")");
}

pub fn pending(name: string) {
    skip(name, "not implemented");
}

// ============================================================================
// Benchmarking
// ============================================================================

pub class BenchResult {
    pub name: string;
    pub iterations: int;
    pub totalMs: int;
    pub avgMs: float;
    pub opsPerSec: float;
}

pub fn bench(name: string, iterations: int, benchFn: fn()) -> BenchResult {
    println("Benchmarking: " + name + " (" + toString(iterations) + " iterations)");
    
    let start = nowMillis();
    for (i in 0..iterations) {
        benchFn();
    }
    let elapsed = nowMillis() - start;
    
    let mut result = new BenchResult();
    result.name = name;
    result.iterations = iterations;
    result.totalMs = elapsed;
    result.avgMs = toFloat(elapsed) / toFloat(iterations);
    result.opsPerSec = toFloat(iterations) / (toFloat(elapsed) / 1000.0);
    
    println("  Total: " + toString(elapsed) + "ms");
    println("  Avg: " + toString(result.avgMs) + "ms/op");
    println("  Ops/sec: " + toString(toInt(result.opsPerSec)));
    
    return result;
}

// ============================================================================
// Test utilities
// ============================================================================

// Create temporary test data
pub fn withTempDir(testFn: fn(string)) {
    @cpp {
        std::string tempDir = std::filesystem::temp_directory_path().string() + 
                              "/magolor_test_" + std::to_string(std::time(nullptr));
        std::filesystem::create_directory(tempDir);
        
        try {
            testFn(tempDir);
        } catch (...) {
            std::filesystem::remove_all(tempDir);
            throw;
        }
        
        std::filesystem::remove_all(tempDir);
    }
}

// Capture stdout during test
pub fn captureOutput(testFn: fn()) -> string {
    @cpp {
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        
        testFn();
        
        std::cout.rdbuf(old);
        return buffer.str();
    }
}
