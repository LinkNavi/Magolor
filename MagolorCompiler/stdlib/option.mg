// Std.Option - Comprehensive Option/Maybe Type Module
// Provides safe handling of optional values and null cases

// ============================================================================
// Option Type Checking
// ============================================================================

// Check if option contains a value (is Some)
pub fn isSome(opt: Option<Int>) -> Bool {
    // Polymorphic - works with any option type
    // Implementation provided by LLVM runtime
    return false;
}

// Check if option is empty (is None)
pub fn isNone(opt: Option<Int>) -> Bool {
    // Polymorphic - works with any option type
    // Implementation provided by LLVM runtime
    return false;
}

// ============================================================================
// Option Value Extraction
// ============================================================================

// Unwrap option value (panics if None)
pub fn unwrap(opt: Option<Int>) -> Int {
    // Polymorphic - works with any option type
    // Implementation provided by LLVM runtime
    // Throws runtime error if None
    return 0;
}

// Unwrap option value or return default
pub fn unwrapOr(opt: Option<Int>, default: Int) -> Int {
    // Polymorphic - works with any option type
    if isSome(opt) {
        return unwrap(opt);
    }
    return default;
}

// Unwrap option value or compute default from function
pub fn unwrapOrElse(opt: Option<Int>, defaultFn: () -> Int) -> Int {
    if isSome(opt) {
        return unwrap(opt);
    }
    return defaultFn();
}

// Get value if Some, otherwise return None
pub fn unwrapOrNone(opt: Option<Int>) -> Option<Int> {
    return opt;
}

// ============================================================================
// Option Transformation
// ============================================================================

// Transform the contained value by applying a function
pub fn map(opt: Option<Int>, pub fn: (Int) -> Int) -> Option<Int> {
    if isSome(opt) {
        return Some(pub fn(unwrap(opt)));
    }
    return None;
}

// Apply a function that returns an Option (flat map)
pub fn flatMap(opt: Option<Int>, pub fn: (Int) -> Option<Int>) -> Option<Int> {
    if isSome(opt) {
        return pub fn(unwrap(opt));
    }
    return None;
}

// Transform option to another type
pub fn mapOr(opt: Option<Int>, default: Int, pub fn: (Int) -> Int) -> Int {
    if isSome(opt) {
        return pub fn(unwrap(opt));
    }
    return default;
}

// Transform option or compute default
pub fn mapOrElse(opt: Option<Int>, defaultFn: () -> Int, pub fn: (Int) -> Int) -> Int {
    if isSome(opt) {
        return pub fn(unwrap(opt));
    }
    return defaultFn();
}

// ============================================================================
// Option Filtering
// ============================================================================

// Return Some if predicate is true, None otherwise
pub fn filter(opt: Option<Int>, predicate: (Int) -> Bool) -> Option<Int> {
    if isSome(opt) {
        let value = unwrap(opt);
        if predicate(value) {
            return Some(value);
        }
    }
    return None;
}

// ============================================================================
// Option Combination
// ============================================================================

// Return opt if it is Some, otherwise return other
pub fn or(opt: Option<Int>, other: Option<Int>) -> Option<Int> {
    if isSome(opt) {
        return opt;
    }
    return other;
}

// Return opt if it is Some, otherwise compute and return other
pub fn orElse(opt: Option<Int>, pub fn: () -> Option<Int>) -> Option<Int> {
    if isSome(opt) {
        return opt;
    }
    return pub fn();
}

// Return Some if both options are Some, otherwise None
pub fn and(opt1: Option<Int>, opt2: Option<Int>) -> Option<Int> {
    if isSome(opt1) && isSome(opt2) {
        return opt2;
    }
    return None;
}

// Chain options with a function
pub fn andThen(opt: Option<Int>, pub fn: (Int) -> Option<Int>) -> Option<Int> {
    return flatMap(opt, pub fn);
}

// Exclusive or - Some if exactly one is Some
pub fn xor(opt1: Option<Int>, opt2: Option<Int>) -> Option<Int> {
    let has1 = isSome(opt1);
    let has2 = isSome(opt2);
    
    if has1 && !has2 {
        return opt1;
    }
    if !has1 && has2 {
        return opt2;
    }
    return None;
}

// ============================================================================
// Option Comparison
// ============================================================================

// Check if two options are equal
pub fn equals(opt1: Option<Int>, opt2: Option<Int>) -> Bool {
    let has1 = isSome(opt1);
    let has2 = isSome(opt2);
    
    if has1 && has2 {
        return unwrap(opt1) == unwrap(opt2);
    }
    return has1 == has2;
}

// ============================================================================
// Option Utility Functions
// ============================================================================

// Execute function if Some, do nothing if None
pub fn ifSome(opt: Option<Int>, pub fn: (Int) -> Void) {
    if isSome(opt) {
        pub fn(unwrap(opt));
    }
}

// Execute one function if Some, another if None
pub fn match(opt: Option<Int>, someFn: (Int) -> Void, noneFn: () -> Void) {
    if isSome(opt) {
        someFn(unwrap(opt));
    } else {
        noneFn();
    }
}

// Convert option to string representation
pub fn toString(opt: Option<Int>) -> String {
    if isSome(opt) {
        return concat("Some(", concat(intToString(unwrap(opt)), ")"));
    }
    return "None";
}

// Convert option to array (empty or single element)
pub fn toArray(opt: Option<Int>) -> Array<Int> {
    if isSome(opt) {
        return [unwrap(opt)];
    }
    return [];
}

// ============================================================================
// Option Construction Helpers
// ============================================================================

// Create Some from value
pub fn some(value: Int) -> Option<Int> {
    return Some(value);
}

// Create None
pub fn none() -> Option<Int> {
    return None;
}

// Create option from nullable value (implementation-specific)
pub fn fromNullable(value: Int, isNull: Bool) -> Option<Int> {
    if isNull {
        return None;
    }
    return Some(value);
}

// ============================================================================
// Option Validation and Assertions
// ============================================================================

// Unwrap and assert that option is Some (with custom error message)
pub fn expect(opt: Option<Int>, message: String) -> Int {
    if isSome(opt) {
        return unwrap(opt);
    }
    // Throw error with message
    throwError(message);
    return 0;
}

// Assert that option is None (with custom error message)
pub fn expectNone(opt: Option<Int>, message: String) {
    if isSome(opt) {
        throwError(message);
    }
}

// ============================================================================
// Option Iteration
// ============================================================================

// Iterate over option (0 or 1 iterations)
pub fn forEach(opt: Option<Int>, pub fn: (Int) -> Void) {
    if isSome(opt) {
        pub fn(unwrap(opt));
    }
}

// ============================================================================
// Option Zipping
// ============================================================================

// Zip two options into a tuple option
pub fn zip(opt1: Option<Int>, opt2: Option<Int>) -> Option<Array<Int>> {
    if isSome(opt1) && isSome(opt2) {
        return Some([unwrap(opt1), unwrap(opt2)]);
    }
    return None;
}

// Zip two options with a combining function
pub fn zipWith(opt1: Option<Int>, opt2: Option<Int>, pub fn: (Int, Int) -> Int) -> Option<Int> {
    if isSome(opt1) && isSome(opt2) {
        return Some(pub fn(unwrap(opt1), unwrap(opt2)));
    }
    return None;
}

// ============================================================================
// Option Flattening
// ============================================================================

// Flatten nested option (Option<Option<T>> -> Option<T>)
pub fn flatten(opt: Option<Option<Int>>) -> Option<Int> {
    if isSome(opt) {
        return unwrap(opt);
    }
    return None;
}

// ============================================================================
// Option Conversion
// ============================================================================

// Convert option to result type (with error for None)
pub fn toResult(opt: Option<Int>, error: String) -> Result<Int, String> {
    if isSome(opt) {
        return Ok(unwrap(opt));
    }
    return Err(error);
}

// Transpose option of result to result of option
pub fn transpose(opt: Option<Result<Int, String>>) -> Result<Option<Int>, String> {
    if isSome(opt) {
        let result = unwrap(opt);
        if isOk(result) {
            return Ok(Some(unwrapResult(result)));
        }
        return Err(unwrapErr(result));
    }
    return Ok(None);
}

// ============================================================================
// Option Collection Operations
// ============================================================================

// Filter array to options, keeping only Some values
pub fn filterMap(arr: Array<Int>, pub fn: (Int) -> Option<Int>) -> Array<Int> {
    let mut result = [];
    let mut i = 0;
    while i < length(arr) {
        let opt = pub fn(arr[i]);
        if isSome(opt) {
            push(result, unwrap(opt));
        }
        i = i + 1;
    }
    return result;
}

// Collect array of options into option of array (None if any None)
pub fn collect(arr: Array<Option<Int>>) -> Option<Array<Int>> {
    let mut result = [];
    let mut i = 0;
    while i < length(arr) {
        if isNone(arr[i]) {
            return None;
        }
        push(result, unwrap(arr[i]));
        i = i + 1;
    }
    return Some(result);
}

// Find first Some in array of options
pub fn findSome(arr: Array<Option<Int>>) -> Option<Int> {
    let mut i = 0;
    while i < length(arr) {
        if isSome(arr[i]) {
            return arr[i];
        }
        i = i + 1;
    }
    return None;
}

// ============================================================================
// Option Predicates on Values
// ============================================================================

// Check if option contains specific value
pub fn contains(opt: Option<Int>, value: Int) -> Bool {
    if isSome(opt) {
        return unwrap(opt) == value;
    }
    return false;
}

// Check if option value satisfies predicate
pub fn exists(opt: Option<Int>, predicate: (Int) -> Bool) -> Bool {
    if isSome(opt) {
        return predicate(unwrap(opt));
    }
    return false;
}

// ============================================================================
// Option Cloning
// ============================================================================

// Clone an option (creates a copy)
pub fn clone(opt: Option<Int>) -> Option<Int> {
    if isSome(opt) {
        return Some(unwrap(opt));
    }
    return None;
}

// ============================================================================
// Option Replace
// ============================================================================

// Replace value in Some, keep None as None
pub fn replace(opt: Option<Int>, newValue: Int) -> Option<Int> {
    if isSome(opt) {
        return Some(newValue);
    }
    return None;
}

// Take value out of option, leaving None
pub fn take(opt: Option<Int>) -> Option<Int> {
    let result = opt;
    opt = None;
    return result;
}

// ============================================================================
// Advanced Option Patterns
// ============================================================================

// Try to get value, or run recovery function
pub fn rescue(opt: Option<Int>, recoveryFn: () -> Int) -> Int {
    return unwrapOrElse(opt, recoveryFn);
}

// Chain multiple optional operations
pub fn chain(opt: Option<Int>, operations: Array<(Int) -> Option<Int>>) -> Option<Int> {
    let mut current = opt;
    let mut i = 0;
    while i < length(operations) {
        if isNone(current) {
            return None;
        }
        current = operations[i](unwrap(current));
        i = i + 1;
    }
    return current;
}
