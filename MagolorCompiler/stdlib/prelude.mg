// Std.Core.Prelude - Fundamental types and operations
// This module is automatically imported into every Magolor program

// ============================================================================
// Option<T> - Represents optional values
// ============================================================================
pub class Option {
    // Option is a built-in generic type, these are helper methods
}

// Check if an Option contains a value
pub fn isSome(opt: Option<any>) -> bool {
    @cpp {
        return opt.has_value();
    }
}

// Check if an Option is empty
pub fn isNone(opt: Option<any>) -> bool {
    @cpp {
        return !opt.has_value();
    }
}

// Extract value from Option, panic if None
pub fn unwrap(opt: Option<any>) -> any {
    @cpp {
        if (!opt.has_value()) {
            throw std::runtime_error("Called unwrap on None value");
        }
        return opt.value();
    }
}

// Extract value from Option, or return default
pub fn unwrapOr(opt: Option<any>, defaultVal: any) -> any {
    @cpp {
        return opt.value_or(defaultVal);
    }
}

// Map a function over an Option
pub fn map(opt: Option<any>, f: fn(any) -> any) -> Option<any> {
    @cpp {
        if (opt.has_value()) {
            return std::make_optional(f(opt.value()));
        }
        return std::nullopt;
    }
}

// ============================================================================
// Result<T, E> - Represents success or failure
// ============================================================================
pub class Result {
    pub ok: bool;
    pub value: any;
    pub error: string;
}

pub fn Ok(value: any) -> Result {
    let mut r = new Result();
    r.ok = true;
    r.value = value;
    r.error = "";
    return r;
}

pub fn Err(message: string) -> Result {
    let mut r = new Result();
    r.ok = false;
    r.error = message;
    return r;
}

pub fn isOk(r: Result) -> bool {
    return r.ok;
}

pub fn isErr(r: Result) -> bool {
    return !r.ok;
}

// ============================================================================
// Assertions and debugging
// ============================================================================
pub fn assert(condition: bool, message: string) {
    if (!condition) {
        @cpp {
            throw std::runtime_error("Assertion failed: " + message);
        }
    }
}

pub fn assertEq(a: any, b: any, message: string) {
    @cpp {
        if (a != b) {
            throw std::runtime_error("Assertion failed: " + message + " (values not equal)");
        }
    }
}

pub fn panic(message: string) {
    @cpp {
        throw std::runtime_error("Panic: " + message);
    }
}

pub fn unreachable() {
    @cpp {
        throw std::runtime_error("Reached unreachable code");
    }
}

// ============================================================================
// Type conversions
// ============================================================================
pub fn toString(value: int) -> string {
    @cpp {
        return std::to_string(value);
    }
}

pub fn toString(value: float) -> string {
    @cpp {
        return std::to_string(value);
    }
}

pub fn toString(value: bool) -> string {
    @cpp {
        return value ? "true" : "false";
    }
}

pub fn toInt(value: float) -> int {
    @cpp {
        return static_cast<int>(value);
    }
}

pub fn toFloat(value: int) -> float {
    @cpp {
        return static_cast<double>(value);
    }
}

// ============================================================================
// Comparison helpers
// ============================================================================
pub fn min(a: int, b: int) -> int {
    if (a < b) { return a; }
    return b;
}

pub fn max(a: int, b: int) -> int {
    if (a > b) { return a; }
    return b;
}

pub fn minf(a: float, b: float) -> float {
    if (a < b) { return a; }
    return b;
}

pub fn maxf(a: float, b: float) -> float {
    if (a > b) { return a; }
    return b;
}

pub fn clamp(value: int, low: int, high: int) -> int {
    return max(low, min(value, high));
}

pub fn clampf(value: float, low: float, high: float) -> float {
    return maxf(low, minf(value, high));
}

// ============================================================================
// Memory/pointer utilities (for advanced use)
// ============================================================================
pub fn sizeOf(typeName: string) -> int {
    @cpp {
        // This is a placeholder - actual implementation depends on type system
        return 0;
    }
}
