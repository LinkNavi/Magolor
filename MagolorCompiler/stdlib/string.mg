// Std.String - Comprehensive String Manipulation Module
// Provides string operations, searching, formatting, and conversions

// ============================================================================
// Basic String Properties
// ============================================================================

// Get the length of a string
pub fn length(s: String) -> Int {
    // Implementation provided by LLVM runtime
    return 0;
}

// Check if string is empty
pub fn isEmpty(s: String) -> Bool {
    return length(s) == 0;
}

// Check if string is not empty
pub fn isNotEmpty(s: String) -> Bool {
    return length(s) > 0;
}

// ============================================================================
// String Concatenation and Building
// ============================================================================

// Concatenate two strings
pub fn concat(a: String, b: String) -> String {
    // Implementation provided by LLVM runtime
    return "";
}

// Join array of strings with separator
pub fn join(arr: Array<String>, separator: String) -> String {
    if length(arr) == 0 {
        return "";
    }
    let mut result = arr[0];
    let mut i = 1;
    while i < length(arr) {
        result = concat(result, separator);
        result = concat(result, arr[i]);
        i = i + 1;
    }
    return result;
}

// Repeat string n times
pub fn repeat(s: String, count: Int) -> String {
    let mut result = "";
    let mut i = 0;
    while i < count {
        result = concat(result, s);
        i = i + 1;
    }
    return result;
}

// ============================================================================
// String Extraction
// ============================================================================

// Get substring from start index to end index
pub fn substring(s: String, start: Int, end: Int) -> String {
    // Implementation provided by LLVM runtime
    return "";
}

// Get substring from start index with specified length
pub fn substr(s: String, start: Int, length: Int) -> String {
    return substring(s, start, start + length);
}

// Get character at index
pub fn charAt(s: String, index: Int) -> String {
    return substring(s, index, index + 1);
}

// Get first n characters
pub fn left(s: String, n: Int) -> String {
    return substring(s, 0, n);
}

// Get last n characters
pub fn right(s: String, n: Int) -> String {
    let len = length(s);
    return substring(s, len - n, len);
}

// Get middle portion of string
pub fn mid(s: String, start: Int, len: Int) -> String {
    return substr(s, start, len);
}

// ============================================================================
// String Searching
// ============================================================================

// Find first occurrence of substring (returns index or -1)
pub fn indexOf(s: String, search: String) -> Int {
    // Implementation provided by LLVM runtime
    return -1;
}

// Find last occurrence of substring (returns index or -1)
pub fn lastIndexOf(s: String, search: String) -> Int {
    // Implementation provided by LLVM runtime
    return -1;
}

// Check if string contains substring
pub fn contains(s: String, search: String) -> Bool {
    return indexOf(s, search) != -1;
}

// Check if string starts with prefix
pub fn startsWith(s: String, prefix: String) -> Bool {
    let prefixLen = length(prefix);
    if prefixLen > length(s) {
        return false;
    }
    return substring(s, 0, prefixLen) == prefix;
}

// Check if string ends with suffix
pub fn endsWith(s: String, suffix: String) -> Bool {
    let suffixLen = length(suffix);
    let sLen = length(s);
    if suffixLen > sLen {
        return false;
    }
    return substring(s, sLen - suffixLen, sLen) == suffix;
}

// Count occurrences of substring
pub fn countOccurrences(s: String, search: String) -> Int {
    let mut count = 0;
    let mut pos = 0;
    let searchLen = length(search);
    while pos < length(s) {
        let index = indexOf(substring(s, pos, length(s)), search);
        if index == -1 {
            return count;
        }
        count = count + 1;
        pos = pos + index + searchLen;
    }
    return count;
}

// ============================================================================
// String Modification
// ============================================================================

// Convert string to uppercase
pub fn toUpperCase(s: String) -> String {
    // Implementation provided by LLVM runtime
    return "";
}

// Convert string to lowercase
pub fn toLowerCase(s: String) -> String {
    // Implementation provided by LLVM runtime
    return "";
}

// Capitalize first letter
pub fn capitalize(s: String) -> String {
    if isEmpty(s) {
        return s;
    }
    return concat(toUpperCase(left(s, 1)), substring(s, 1, length(s)));
}

// Convert to title case (capitalize each word)
pub fn toTitleCase(s: String) -> String {
    // Implementation provided by LLVM runtime
    return "";
}

// Reverse a string
pub fn reverse(s: String) -> String {
    // Implementation provided by LLVM runtime
    return "";
}

// Replace all occurrences of old with new
pub fn replace(s: String, old: String, new: String) -> String {
    // Implementation provided by LLVM runtime
    return "";
}

// Replace first occurrence of old with new
pub fn replaceFirst(s: String, old: String, new: String) -> String {
    let index = indexOf(s, old);
    if index == -1 {
        return s;
    }
    let before = substring(s, 0, index);
    let after = substring(s, index + length(old), length(s));
    return concat(concat(before, new), after);
}

// ============================================================================
// String Trimming
// ============================================================================

// Remove whitespace from both ends
pub fn trim(s: String) -> String {
    // Implementation provided by LLVM runtime
    return "";
}

// Remove whitespace from start
pub fn trimStart(s: String) -> String {
    // Implementation provided by LLVM runtime
    return "";
}

// Remove whitespace from end
pub fn trimEnd(s: String) -> String {
    // Implementation provided by LLVM runtime
    return "";
}

// Remove specific characters from both ends
pub fn trimChars(s: String, chars: String) -> String {
    // Implementation provided by LLVM runtime
    return "";
}

// ============================================================================
// String Splitting
// ============================================================================

// Split string by delimiter
pub fn split(s: String, delimiter: String) -> Array<String> {
    // Implementation provided by LLVM runtime
    return [];
}

// Split into lines
pub fn splitLines(s: String) -> Array<String> {
    return split(s, "\n");
}

// Split into words (by whitespace)
pub fn splitWords(s: String) -> Array<String> {
    // Implementation provided by LLVM runtime
    return [];
}

// Split into characters
pub fn toChars(s: String) -> Array<String> {
    let mut chars = [];
    let mut i = 0;
    while i < length(s) {
        push(chars, charAt(s, i));
        i = i + 1;
    }
    return chars;
}

// ============================================================================
// String Padding and Alignment
// ============================================================================

// Pad string to length with spaces on the right
pub fn padEnd(s: String, targetLength: Int) -> String {
    return padEndWith(s, targetLength, " ");
}

// Pad string to length with spaces on the left
pub fn padStart(s: String, targetLength: Int) -> String {
    return padStartWith(s, targetLength, " ");
}

// Pad string on right with custom character
pub fn padEndWith(s: String, targetLength: Int, padChar: String) -> String {
    let currentLen = length(s);
    if currentLen >= targetLength {
        return s;
    }
    let padding = repeat(padChar, targetLength - currentLen);
    return concat(s, padding);
}

// Pad string on left with custom character
pub fn padStartWith(s: String, targetLength: Int, padChar: String) -> String {
    let currentLen = length(s);
    if currentLen >= targetLength {
        return s;
    }
    let padding = repeat(padChar, targetLength - currentLen);
    return concat(padding, s);
}

// Center string with padding
pub fn center(s: String, targetLength: Int) -> String {
    let currentLen = length(s);
    if currentLen >= targetLength {
        return s;
    }
    let totalPadding = targetLength - currentLen;
    let leftPadding = totalPadding / 2;
    let rightPadding = totalPadding - leftPadding;
    return concat(concat(repeat(" ", leftPadding), s), repeat(" ", rightPadding));
}

// ============================================================================
// String Comparison
// ============================================================================

// Compare two strings (returns -1, 0, or 1)
pub fn compare(a: String, b: String) -> Int {
    // Implementation provided by LLVM runtime
    return 0;
}

// Case-insensitive comparison
pub fn compareIgnoreCase(a: String, b: String) -> Bool {
    return toLowerCase(a) == toLowerCase(b);
}

// Check if strings are equal
pub fn equals(a: String, b: String) -> Bool {
    return a == b;
}

// Check if strings are equal (case-insensitive)
pub fn equalsIgnoreCase(a: String, b: String) -> Bool {
    return compareIgnoreCase(a, b);
}

// ============================================================================
// String Validation
// ============================================================================

// Check if string contains only digits
pub fn isDigit(s: String) -> Bool {
    // Implementation provided by LLVM runtime
    return false;
}

// Check if string contains only letters
pub fn isAlpha(s: String) -> Bool {
    // Implementation provided by LLVM runtime
    return false;
}

// Check if string contains only letters and digits
pub fn isAlphanumeric(s: String) -> Bool {
    // Implementation provided by LLVM runtime
    return false;
}

// Check if string contains only whitespace
pub fn isWhitespace(s: String) -> Bool {
    return trim(s) == "";
}

// Check if string contains only lowercase letters
pub fn isLowerCase(s: String) -> Bool {
    return s == toLowerCase(s);
}

// Check if string contains only uppercase letters
pub fn isUpperCase(s: String) -> Bool {
    return s == toUpperCase(s);
}

// ============================================================================
// Type Conversion
// ============================================================================

// Convert integer to string
pub fn toString(value: Int) -> String {
    // Implementation provided by LLVM runtime
    return "";
}

// Convert float to string
pub fn floatToString(value: Float) -> String {
    // Implementation provided by LLVM runtime
    return "";
}

// Convert boolean to string
pub fn boolToString(value: Bool) -> String {
    if value {
        return "true";
    }
    return "false";
}

// Convert string to integer (returns 0 if invalid)
pub fn toInt(s: String) -> Int {
    // Implementation provided by LLVM runtime
    return 0;
}

// Convert string to float (returns 0.0 if invalid)
pub fn toFloat(s: String) -> Float {
    // Implementation provided by LLVM runtime
    return 0.0;
}

// Convert string to boolean
pub fn toBool(s: String) -> Bool {
    let lower = toLowerCase(trim(s));
    return lower == "true" || lower == "1" || lower == "yes";
}

// ============================================================================
// String Formatting
// ============================================================================

// Format string with placeholders {0}, {1}, etc.
pub fn format(template: String, args: Array<String>) -> String {
    let mut result = template;
    let mut i = 0;
    while i < length(args) {
        let placeholder = concat(concat("{", toString(i)), "}");
        result = replace(result, placeholder, args[i]);
        i = i + 1;
    }
    return result;
}

// ============================================================================
// Encoding/Decoding Utilities
// ============================================================================

// URL encode string
pub fn urlEncode(s: String) -> String {
    // Implementation provided by LLVM runtime
    return "";
}

// URL decode string
pub fn urlDecode(s: String) -> String {
    // Implementation provided by LLVM runtime
    return "";
}

// Escape HTML special characters
pub fn escapeHtml(s: String) -> String {
    let mut result = s;
    result = replace(result, "&", "&amp;");
    result = replace(result, "<", "&lt;");
    result = replace(result, ">", "&gt;");
    result = replace(result, "\"", "&quot;");
    result = replace(result, "'", "&#39;");
    return result;
}

// Unescape HTML entities
pub fn unescapeHtml(s: String) -> String {
    let mut result = s;
    result = replace(result, "&lt;", "<");
    result = replace(result, "&gt;", ">");
    result = replace(result, "&quot;", "\"");
    result = replace(result, "&#39;", "'");
    result = replace(result, "&amp;", "&");
    return result;
}
