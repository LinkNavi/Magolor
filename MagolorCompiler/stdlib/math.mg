// Std.Math - Mathematical operations
// Provides mathematical constants and functions

using Std.Core.Prelude;

// ============================================================================
// Constants
// ============================================================================

pub static PI: float = 3.14159265358979323846;
pub static E: float = 2.71828182845904523536;
pub static TAU: float = 6.28318530717958647692;
pub static PHI: float = 1.61803398874989484820;  // Golden ratio
pub static SQRT2: float = 1.41421356237309504880;
pub static LN2: float = 0.69314718055994530942;
pub static LN10: float = 2.30258509299404568402;

pub static MAX_INT: int = 2147483647;
pub static MIN_INT: int = -2147483648;
pub static INFINITY: float = 1.0 / 0.0;
pub static NEG_INFINITY: float = -1.0 / 0.0;

// ============================================================================
// Basic operations
// ============================================================================

pub fn abs(x: int) -> int {
    @cpp { return std::abs(x); }
}

pub fn absf(x: float) -> float {
    @cpp { return std::fabs(x); }
}

pub fn sign(x: int) -> int {
    if (x > 0) { return 1; }
    if (x < 0) { return -1; }
    return 0;
}

pub fn signf(x: float) -> int {
    if (x > 0.0) { return 1; }
    if (x < 0.0) { return -1; }
    return 0;
}

// ============================================================================
// Power and roots
// ============================================================================

pub fn pow(base: float, exp: float) -> float {
    @cpp { return std::pow(base, exp); }
}

pub fn powi(base: int, exp: int) -> int {
    @cpp { return static_cast<int>(std::pow(base, exp)); }
}

pub fn sqrt(x: float) -> float {
    @cpp { return std::sqrt(x); }
}

pub fn cbrt(x: float) -> float {
    @cpp { return std::cbrt(x); }
}

pub fn nthRoot(x: float, n: int) -> float {
    return pow(x, 1.0 / toFloat(n));
}

pub fn hypot(x: float, y: float) -> float {
    @cpp { return std::hypot(x, y); }
}

// ============================================================================
// Trigonometric functions
// ============================================================================

pub fn sin(x: float) -> float {
    @cpp { return std::sin(x); }
}

pub fn cos(x: float) -> float {
    @cpp { return std::cos(x); }
}

pub fn tan(x: float) -> float {
    @cpp { return std::tan(x); }
}

pub fn asin(x: float) -> float {
    @cpp { return std::asin(x); }
}

pub fn acos(x: float) -> float {
    @cpp { return std::acos(x); }
}

pub fn atan(x: float) -> float {
    @cpp { return std::atan(x); }
}

pub fn atan2(y: float, x: float) -> float {
    @cpp { return std::atan2(y, x); }
}

// Hyperbolic functions
pub fn sinh(x: float) -> float {
    @cpp { return std::sinh(x); }
}

pub fn cosh(x: float) -> float {
    @cpp { return std::cosh(x); }
}

pub fn tanh(x: float) -> float {
    @cpp { return std::tanh(x); }
}

// ============================================================================
// Exponential and logarithmic
// ============================================================================

pub fn exp(x: float) -> float {
    @cpp { return std::exp(x); }
}

pub fn exp2(x: float) -> float {
    @cpp { return std::exp2(x); }
}

pub fn log(x: float) -> float {
    @cpp { return std::log(x); }
}

pub fn log2(x: float) -> float {
    @cpp { return std::log2(x); }
}

pub fn log10(x: float) -> float {
    @cpp { return std::log10(x); }
}

pub fn logBase(x: float, base: float) -> float {
    return log(x) / log(base);
}

// ============================================================================
// Rounding
// ============================================================================

pub fn floor(x: float) -> float {
    @cpp { return std::floor(x); }
}

pub fn ceil(x: float) -> float {
    @cpp { return std::ceil(x); }
}

pub fn round(x: float) -> float {
    @cpp { return std::round(x); }
}

pub fn trunc(x: float) -> float {
    @cpp { return std::trunc(x); }
}

pub fn floorInt(x: float) -> int {
    @cpp { return static_cast<int>(std::floor(x)); }
}

pub fn ceilInt(x: float) -> int {
    @cpp { return static_cast<int>(std::ceil(x)); }
}

pub fn roundInt(x: float) -> int {
    @cpp { return static_cast<int>(std::round(x)); }
}

// ============================================================================
// Modular arithmetic
// ============================================================================

pub fn mod(a: int, b: int) -> int {
    @cpp { return ((a % b) + b) % b; }  // Always positive
}

pub fn modf(a: float, b: float) -> float {
    @cpp { return std::fmod(a, b); }
}

pub fn gcd(a: int, b: int) -> int {
    @cpp {
        while (b != 0) {
            int t = b;
            b = a % b;
            a = t;
        }
        return std::abs(a);
    }
}

pub fn lcm(a: int, b: int) -> int {
    return abs(a * b) / gcd(a, b);
}

// ============================================================================
// Comparison and special values
// ============================================================================

pub fn isNaN(x: float) -> bool {
    @cpp { return std::isnan(x); }
}

pub fn isInf(x: float) -> bool {
    @cpp { return std::isinf(x); }
}

pub fn isFinite(x: float) -> bool {
    @cpp { return std::isfinite(x); }
}

pub fn approxEq(a: float, b: float, epsilon: float) -> float {
    return absf(a - b) < epsilon;
}

// ============================================================================
// Conversion utilities
// ============================================================================

pub fn degToRad(deg: float) -> float {
    return deg * PI / 180.0;
}

pub fn radToDeg(rad: float) -> float {
    return rad * 180.0 / PI;
}

// ============================================================================
// Factorial and combinatorics
// ============================================================================

pub fn factorial(n: int) -> int {
    if (n <= 1) { return 1; }
    let mut result = 1;
    for (i in 2..(n + 1)) {
        result = result * i;
    }
    return result;
}

pub fn permutations(n: int, r: int) -> int {
    return factorial(n) / factorial(n - r);
}

pub fn combinations(n: int, r: int) -> int {
    return factorial(n) / (factorial(r) * factorial(n - r));
}

// ============================================================================
// Interpolation
// ============================================================================

pub fn lerp(a: float, b: float, t: float) -> float {
    return a + (b - a) * t;
}

pub fn inverseLerp(a: float, b: float, value: float) -> float {
    return (value - a) / (b - a);
}

pub fn remap(value: float, fromLow: float, fromHigh: float, toLow: float, toHigh: float) -> float {
    let t = inverseLerp(fromLow, fromHigh, value);
    return lerp(toLow, toHigh, t);
}

pub fn smoothstep(edge0: float, edge1: float, x: float) -> float {
    let t = clampf((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}
