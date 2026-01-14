// Std.Math - Comprehensive Mathematical Functions Module
// Provides constants, basic operations, trigonometry, and advanced math

// ============================================================================
// Mathematical Constants
// ============================================================================

let PI = 3.14159265358979323846;
let E = 2.71828182845904523536;
let TAU = 6.28318530717958647692;  // 2 * PI
let PHI = 1.61803398874989484820;  // Golden ratio
let SQRT2 = 1.41421356237309504880;
let SQRT3 = 1.73205080756887729352;
let LN2 = 0.69314718055994530942;
let LN10 = 2.30258509299404568402;
let LOG2E = 1.44269504088896340736;
let LOG10E = 0.43429448190325182765;

// ============================================================================
// Basic Operations
// ============================================================================

// Absolute value of a float
pub fn abs(x: Float) -> Float {
    // Implementation provided by LLVM runtime
    return 0.0;
}

// Absolute value of an integer
pub fn absInt(x: Int) -> Int {
    if x < 0 {
        return -x;
    }
    return x;
}

// Sign function (-1, 0, or 1)
pub fn sign(x: Float) -> Int {
    if x > 0.0 {
        return 1;
    }
    if x < 0.0 {
        return -1;
    }
    return 0;
}

// Minimum of two integers
pub fn min(a: Int, b: Int) -> Int {
    if a < b {
        return a;
    }
    return b;
}

// Maximum of two integers
pub fn max(a: Int, b: Int) -> Int {
    if a > b {
        return a;
    }
    return b;
}

// Minimum of two floats
pub fn minFloat(a: Float, b: Float) -> Float {
    if a < b {
        return a;
    }
    return b;
}

// Maximum of two floats
pub fn maxFloat(a: Float, b: Float) -> Float {
    if a > b {
        return a;
    }
    return b;
}

// Clamp value between min and max
pub fn clamp(x: Float, minVal: Float, maxVal: Float) -> Float {
    if x < minVal {
        return minVal;
    }
    if x > maxVal {
        return maxVal;
    }
    return x;
}

// ============================================================================
// Power and Root Functions
// ============================================================================

// Square root
pub fn sqrt(x: Float) -> Float {
    // Implementation provided by LLVM runtime
    return 0.0;
}

// Cube root
pub fn cbrt(x: Float) -> Float {
    // Implementation provided by LLVM runtime
    return 0.0;
}

// Power function (base^exponent)
pub fn pow(base: Float, exp: Float) -> Float {
    // Implementation provided by LLVM runtime
    return 0.0;
}

// Square a number (x^2)
pub fn square(x: Float) -> Float {
    return x * x;
}

// Cube a number (x^3)
pub fn cube(x: Float) -> Float {
    return x * x * x;
}

// Nth root
pub fn nthRoot(x: Float, n: Float) -> Float {
    return pow(x, 1.0 / n);
}

// ============================================================================
// Exponential and Logarithmic Functions
// ============================================================================

// Natural exponential (e^x)
pub fn exp(x: Float) -> Float {
    // Implementation provided by LLVM runtime
    return 0.0;
}

// Base-2 exponential (2^x)
pub fn exp2(x: Float) -> Float {
    return pow(2.0, x);
}

// Natural logarithm (ln)
pub fn log(x: Float) -> Float {
    // Implementation provided by LLVM runtime
    return 0.0;
}

// Base-10 logarithm
pub fn log10(x: Float) -> Float {
    // Implementation provided by LLVM runtime
    return 0.0;
}

// Base-2 logarithm
pub fn log2(x: Float) -> Float {
    return log(x) / LN2;
}

// Logarithm with custom base
pub fn logBase(x: Float, base: Float) -> Float {
    return log(x) / log(base);
}

// ============================================================================
// Trigonometric Functions (Radians)
// ============================================================================

// Sine
pub fn sin(x: Float) -> Float {
    // Implementation provided by LLVM runtime
    return 0.0;
}

// Cosine
pub fn cos(x: Float) -> Float {
    // Implementation provided by LLVM runtime
    return 0.0;
}

// Tangent
pub fn tan(x: Float) -> Float {
    // Implementation provided by LLVM runtime
    return 0.0;
}

// Arcsine
pub fn asin(x: Float) -> Float {
    // Implementation provided by LLVM runtime
    return 0.0;
}

// Arccosine
pub fn acos(x: Float) -> Float {
    // Implementation provided by LLVM runtime
    return 0.0;
}

// Arctangent
pub fn atan(x: Float) -> Float {
    // Implementation provided by LLVM runtime
    return 0.0;
}

// Two-argument arctangent (atan2)
pub fn atan2(y: Float, x: Float) -> Float {
    // Implementation provided by LLVM runtime
    return 0.0;
}

// ============================================================================
// Hyperbolic Functions
// ============================================================================

// Hyperbolic sine
pub fn sinh(x: Float) -> Float {
    return (exp(x) - exp(-x)) / 2.0;
}

// Hyperbolic cosine
pub fn cosh(x: Float) -> Float {
    return (exp(x) + exp(-x)) / 2.0;
}

// Hyperbolic tangent
pub fn tanh(x: Float) -> Float {
    let expPos = exp(x);
    let expNeg = exp(-x);
    return (expPos - expNeg) / (expPos + expNeg);
}

// ============================================================================
// Rounding Functions
// ============================================================================

// Round down (floor)
pub fn floor(x: Float) -> Float {
    // Implementation provided by LLVM runtime
    return 0.0;
}

// Round up (ceiling)
pub fn ceil(x: Float) -> Float {
    // Implementation provided by LLVM runtime
    return 0.0;
}

// Round to nearest integer
pub fn round(x: Float) -> Float {
    if x >= 0.0 {
        return floor(x + 0.5);
    }
    return ceil(x - 0.5);
}

// Truncate towards zero
pub fn trunc(x: Float) -> Float {
    if x >= 0.0 {
        return floor(x);
    }
    return ceil(x);
}

// Get fractional part
pub fn fract(x: Float) -> Float {
    return x - floor(x);
}

// ============================================================================
// Angular Conversion Functions
// ============================================================================

// Convert degrees to radians
pub fn toRadians(degrees: Float) -> Float {
    return degrees * PI / 180.0;
}

// Convert radians to degrees
pub fn toDegrees(radians: Float) -> Float {
    return radians * 180.0 / PI;
}

// ============================================================================
// Utility Functions
// ============================================================================

// Check if number is NaN
pub fn isNaN(x: Float) -> Bool {
    return x != x;
}

// Check if number is infinite
pub fn isInfinite(x: Float) -> Bool {
    // Implementation provided by LLVM runtime
    return false;
}

// Check if number is finite
pub fn isFinite(x: Float) -> Bool {
    return !isNaN(x) && !isInfinite(x);
}

// Linear interpolation
pub fn lerp(a: Float, b: Float, t: Float) -> Float {
    return a + (b - a) * t;
}

// Smooth step interpolation
pub fn smoothstep(edge0: Float, edge1: Float, x: Float) -> Float {
    let t = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

// Remainder of division
pub fn mod(x: Float, y: Float) -> Float {
    return x - y * floor(x / y);
}

// Greatest common divisor
pub fn gcd(a: Int, b: Int) -> Int {
    let mut x = absInt(a);
    let mut y = absInt(b);
    while y != 0 {
        let temp = y;
        y = x % y;
        x = temp;
    }
    return x;
}

// Least common multiple
pub fn lcm(a: Int, b: Int) -> Int {
    if a == 0 || b == 0 {
        return 0;
    }
    return absInt(a * b) / gcd(a, b);
}

// Factorial
pub fn factorial(n: Int) -> Int {
    if n <= 1 {
        return 1;
    }
    let mut result = 1;
    let mut i = 2;
    while i <= n {
        result = result * i;
        i = i + 1;
    }
    return result;
}

// Check if number is prime
pub fn isPrime(n: Int) -> Bool {
    if n <= 1 {
        return false;
    }
    if n <= 3 {
        return true;
    }
    if n % 2 == 0 || n % 3 == 0 {
        return false;
    }
    let mut i = 5;
    while i * i <= n {
        if n % i == 0 || n % (i + 2) == 0 {
            return false;
        }
        i = i + 6;
    }
    return true;
}

// ============================================================================
// Vector/Geometry Helpers
// ============================================================================

// Distance between two 2D points
pub fn distance2D(x1: Float, y1: Float, x2: Float, y2: Float) -> Float {
    let dx = x2 - x1;
    let dy = y2 - y1;
    return sqrt(dx * dx + dy * dy);
}

// Distance between two 3D points
pub fn distance3D(x1: Float, y1: Float, z1: Float, x2: Float, y2: Float, z2: Float) -> Float {
    let dx = x2 - x1;
    let dy = y2 - y1;
    let dz = z2 - z1;
    return sqrt(dx * dx + dy * dy + dz * dz);
}

// Magnitude of 2D vector
pub fn magnitude2D(x: Float, y: Float) -> Float {
    return sqrt(x * x + y * y);
}

// Magnitude of 3D vector
pub fn magnitude3D(x: Float, y: Float, z: Float) -> Float {
    return sqrt(x * x + y * y + z * z);
}

// Dot product of 2D vectors
pub fn dot2D(x1: Float, y1: Float, x2: Float, y2: Float) -> Float {
    return x1 * x2 + y1 * y2;
}

// Dot product of 3D vectors
pub fn dot3D(x1: Float, y1: Float, z1: Float, x2: Float, y2: Float, z2: Float) -> Float {
    return x1 * x2 + y1 * y2 + z1 * z2;
}
