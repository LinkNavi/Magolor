// Std.Array - Comprehensive Array Manipulation Module
// Provides generic array operations, transformations, and algorithms

// ============================================================================
// Basic Array Properties
// ============================================================================

// Get the length of an array
pub fn length(arr: Array<Int>) -> Int {
    // Polymorphic - works with any array type
    // Implementation provided by LLVM runtime
    return 0;
}

// Check if array is empty
pub fn isEmpty(arr: Array<Int>) -> Bool {
    return length(arr) == 0;
}

// Check if array is not empty
pub fn isNotEmpty(arr: Array<Int>) -> Bool {
    return length(arr) > 0;
}

// ============================================================================
// Array Modification
// ============================================================================

// Add element to end of array
pub fn push(arr: Array<Int>, value: Int) {
    // Polymorphic - works with any array type
    // Implementation provided by LLVM runtime
}

// Remove and return last element
pub fn pop(arr: Array<Int>) -> Int {
    // Polymorphic - works with any array type
    // Implementation provided by LLVM runtime
    return 0;
}

// Insert element at beginning
pub fn unshift(arr: Array<Int>, value: Int) {
    // Polymorphic - works with any array type
    // Implementation provided by LLVM runtime
}

// Remove and return first element
pub fn shift(arr: Array<Int>) -> Int {
    // Polymorphic - works with any array type
    // Implementation provided by LLVM runtime
    return 0;
}

// Insert element at specific index
 pub fn insert(arr: Array<Int>, index: Int, value: Int) {
    // Polymorphic - works with any array type
    // Implementation provided by LLVM runtime
}

// Remove element at specific index
pub fn remove(arr: Array<Int>, index: Int) -> Int {
    // Polymorphic - works with any array type
    // Implementation provided by LLVM runtime
    return 0;
}

// Clear all elements from array
pub fn clear(arr: Array<Int>) {
    // Implementation provided by LLVM runtime
}

// ============================================================================
// Array Access
// ============================================================================

// Get element at index (with bounds checking)
pub fn get(arr: Array<Int>, index: Int) -> Int {
    // Polymorphic - works with any array type
    // Implementation provided by LLVM runtime
    return 0;
}

// Set element at index (with bounds checking)
pub fn set(arr: Array<Int>, index: Int, value: Int) {
    // Polymorphic - works with any array type
    // Implementation provided by LLVM runtime
}

// Get first element
pub fn first(arr: Array<Int>) -> Int {
    return arr[0];
}

// Get last element
pub fn last(arr: Array<Int>) -> Int {
    return arr[length(arr) - 1];
}

// ============================================================================
// Array Slicing and Extraction
// ============================================================================

// Get subarray from start to end index
pub fn slice(arr: Array<Int>, start: Int, end: Int) -> Array<Int> {
    // Implementation provided by LLVM runtime
    return [];
}

// Get subarray from start index with length
pub fn subarray(arr: Array<Int>, start: Int, len: Int) -> Array<Int> {
    return slice(arr, start, start + len);
}

// Take first n elements
pub fn take(arr: Array<Int>, n: Int) -> Array<Int> {
    return slice(arr, 0, n);
}

// Skip first n elements
pub fn skip(arr: Array<Int>, n: Int) -> Array<Int> {
    return slice(arr, n, length(arr));
}

// Take last n elements
pub fn takeLast(arr: Array<Int>, n: Int) -> Array<Int> {
    let len = length(arr);
    return slice(arr, len - n, len);
}

// ============================================================================
// Array Searching
// ============================================================================

// Find index of first occurrence (returns -1 if not found)
pub fn indexOf(arr: Array<Int>, value: Int) -> Int {
    let mut i = 0;
    while i < length(arr) {
        if arr[i] == value {
            return i;
        }
        i = i + 1;
    }
    return -1;
}

// Find index of last occurrence (returns -1 if not found)
pub fn lastIndexOf(arr: Array<Int>, value: Int) -> Int {
    let mut i = length(arr) - 1;
    while i >= 0 {
        if arr[i] == value {
            return i;
        }
        i = i - 1;
    }
    return -1;
}

// Check if array contains value
pub fn contains(arr: Array<Int>, value: Int) -> Bool {
    return indexOf(arr, value) != -1;
}

// Count occurrences of value
pub fn count(arr: Array<Int>, value: Int) -> Int {
    let mut cnt = 0;
    let mut i = 0;
    while i < length(arr) {
        if arr[i] == value {
            cnt = cnt + 1;
        }
        i = i + 1;
    }
    return cnt;
}

// ============================================================================
// Array Transformation
// ============================================================================

// Create new array by applying function to each element
pub fn map(arr: Array<Int>, pub fn: (Int) -> Int) -> Array<Int> {
    let mut result = [];
    let mut i = 0;
    while i < length(arr) {
        push(result, pub fn(arr[i]));
        i = i + 1;
    }
    return result;
}

// Filter array by predicate function
pub fn filter(arr: Array<Int>, predicate: (Int) -> Bool) -> Array<Int> {
    let mut result = [];
    let mut i = 0;
    while i < length(arr) {
        if predicate(arr[i]) {
            push(result, arr[i]);
        }
        i = i + 1;
    }
    return result;
}

// Reduce array to single value
pub fn reduce(arr: Array<Int>, initial: Int, pub fn: (Int, Int) -> Int) -> Int {
    let mut accumulator = initial;
    let mut i = 0;
    while i < length(arr) {
        accumulator = pub fn(accumulator, arr[i]);
        i = i + 1;
    }
    return accumulator;
}

// Flatten nested array by one level
pub fn flatten(arr: Array<Array<Int>>) -> Array<Int> {
    let mut result = [];
    let mut i = 0;
    while i < length(arr) {
        let mut j = 0;
        while j < length(arr[i]) {
            push(result, arr[i][j]);
            j = j + 1;
        }
        i = i + 1;
    }
    return result;
}

// Reverse array (creates new array)
pub fn reverse(arr: Array<Int>) -> Array<Int> {
    let mut result = [];
    let mut i = length(arr) - 1;
    while i >= 0 {
        push(result, arr[i]);
        i = i - 1;
    }
    return result;
}

// ============================================================================
// Array Sorting (In-place)
// ============================================================================

// Sort array in ascending order (in-place)
pub fn sort(arr: Array<Int>) {
    // Bubble sort implementation
    let n = length(arr);
    let mut i = 0;
    while i < n - 1 {
        let mut j = 0;
        while j < n - i - 1 {
            if arr[j] > arr[j + 1] {
                // Swap
                let temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
            j = j + 1;
        }
        i = i + 1;
    }
}

// Sort array in descending order (in-place)
pub fn sortDescending(arr: Array<Int>) {
    let n = length(arr);
    let mut i = 0;
    while i < n - 1 {
        let mut j = 0;
        while j < n - i - 1 {
            if arr[j] < arr[j + 1] {
                // Swap
                let temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
            j = j + 1;
        }
        i = i + 1;
    }
}

// Check if array is sorted
pub fn isSorted(arr: Array<Int>) -> Bool {
    let mut i = 0;
    while i < length(arr) - 1 {
        if arr[i] > arr[i + 1] {
            return false;
        }
        i = i + 1;
    }
    return true;
}

// ============================================================================
// Array Aggregation
// ============================================================================

// Sum all elements (for numeric arrays)
pub fn sum(arr: Array<Int>) -> Int {
    let mut total = 0;
    let mut i = 0;
    while i < length(arr) {
        total = total + arr[i];
        i = i + 1;
    }
    return total;
}

// Calculate average (for numeric arrays)
pub fn average(arr: Array<Int>) -> Float {
    if isEmpty(arr) {
        return 0.0;
    }
    return toFloat(sum(arr)) / toFloat(length(arr));
}

// Find minimum value
pub fn min(arr: Array<Int>) -> Int {
    if isEmpty(arr) {
        return 0;
    }
    let mut minVal = arr[0];
    let mut i = 1;
    while i < length(arr) {
        if arr[i] < minVal {
            minVal = arr[i];
        }
        i = i + 1;
    }
    return minVal;
}

// Find maximum value
pub fn max(arr: Array<Int>) -> Int {
    if isEmpty(arr) {
        return 0;
    }
    let mut maxVal = arr[0];
    let mut i = 1;
    while i < length(arr) {
        if arr[i] > maxVal {
            maxVal = arr[i];
        }
        i = i + 1;
    }
    return maxVal;
}

// Product of all elements
pub fn product(arr: Array<Int>) -> Int {
    let mut result = 1;
    let mut i = 0;
    while i < length(arr) {
        result = result * arr[i];
        i = i + 1;
    }
    return result;
}

// ============================================================================
// Array Predicates
// ============================================================================

// Check if all elements satisfy predicate
pub fn all(arr: Array<Int>, predicate: (Int) -> Bool) -> Bool {
    let mut i = 0;
    while i < length(arr) {
        if !predicate(arr[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

// Check if any element satisfies predicate
pub fn any(arr: Array<Int>, predicate: (Int) -> Bool) -> Bool {
    let mut i = 0;
    while i < length(arr) {
        if predicate(arr[i]) {
            return true;
        }
        i = i + 1;
    }
    return false;
}

// Check if no elements satisfy predicate
pub fn none(arr: Array<Int>, predicate: (Int) -> Bool) -> Bool {
    return !any(arr, predicate);
}

// ============================================================================
// Array Combination
// ============================================================================

// Concatenate two arrays
pub fn concat(a: Array<Int>, b: Array<Int>) -> Array<Int> {
    let mut result = [];
    let mut i = 0;
    while i < length(a) {
        push(result, a[i]);
        i = i + 1;
    }
    i = 0;
    while i < length(b) {
        push(result, b[i]);
        i = i + 1;
    }
    return result;
}

// Zip two arrays into array of pairs
pub fn zip(a: Array<Int>, b: Array<Int>) -> Array<Array<Int>> {
    let mut result = [];
    let minLen = min(length(a), length(b));
    let mut i = 0;
    while i < minLen {
        let pair = [a[i], b[i]];
        push(result, pair);
        i = i + 1;
    }
    return result;
}

// ============================================================================
// Array Utilities
// ============================================================================

// Create array filled with value
pub fn fill(value: Int, count: Int) -> Array<Int> {
    let mut arr = [];
    let mut i = 0;
    while i < count {
        push(arr, value);
        i = i + 1;
    }
    return arr;
}

// Create array with range of values [start, end)
pub fn range(start: Int, end: Int) -> Array<Int> {
    let mut arr = [];
    let mut i = start;
    while i < end {
        push(arr, i);
        i = i + 1;
    }
    return arr;
}

// Create array with range and step
pub fn rangeStep(start: Int, end: Int, step: Int) -> Array<Int> {
    let mut arr = [];
    let mut i = start;
    if step > 0 {
        while i < end {
            push(arr, i);
            i = i + step;
        }
    } else if step < 0 {
        while i > end {
            push(arr, i);
            i = i + step;
        }
    }
    return arr;
}

// Remove duplicates from array
pub fn unique(arr: Array<Int>) -> Array<Int> {
    let mut result = [];
    let mut i = 0;
    while i < length(arr) {
        if !contains(result, arr[i]) {
            push(result, arr[i]);
        }
        i = i + 1;
    }
    return result;
}

// Rotate array left by n positions
pub fn rotateLeft(arr: Array<Int>, n: Int) -> Array<Int> {
    let len = length(arr);
    if len == 0 {
        return arr;
    }
    let positions = n % len;
    return concat(slice(arr, positions, len), slice(arr, 0, positions));
}

// Rotate array right by n positions
pub fn rotateRight(arr: Array<Int>, n: Int) -> Array<Int> {
    let len = length(arr);
    if len == 0 {
        return arr;
    }
    return rotateLeft(arr, len - (n % len));
}

// Shuffle array randomly
pub fn shuffle(arr: Array<Int>) {
    // Implementation provided by LLVM runtime (requires random)
}

// Create a copy of array
pub fn clone(arr: Array<Int>) -> Array<Int> {
    let mut copy = [];
    let mut i = 0;
    while i < length(arr) {
        push(copy, arr[i]);
        i = i + 1;
    }
    return copy;
}

// Partition array into chunks of size n
pub fn chunk(arr: Array<Int>, size: Int) -> Array<Array<Int>> {
    let mut result = [];
    let mut i = 0;
    while i < length(arr) {
        let end = min(i + size, length(arr));
        push(result, slice(arr, i, end));
        i = i + size;
    }
    return result;
}

// Find differences between two arrays (elements in a but not in b)
pub fn difference(a: Array<Int>, b: Array<Int>) -> Array<Int> {
    let mut result = [];
    let mut i = 0;
    while i < length(a) {
        if !contains(b, a[i]) {
            push(result, a[i]);
        }
        i = i + 1;
    }
    return result;
}

// Find intersection of two arrays
pub fn intersection(a: Array<Int>, b: Array<Int>) -> Array<Int> {
    let mut result = [];
    let mut i = 0;
    while i < length(a) {
        if contains(b, a[i]) && !contains(result, a[i]) {
            push(result, a[i]);
        }
        i = i + 1;
    }
    return result;
}

// Find union of two arrays (no duplicates)
pub fn union(a: Array<Int>, b: Array<Int>) -> Array<Int> {
    return unique(concat(a, b));
}
