// Std.String - String manipulation
// Comprehensive string operations

using Std.Core.Prelude;

// ============================================================================
// Basic operations
// ============================================================================

pub fn length(s: string) -> int {
    @cpp { return s.length(); }
}

pub fn isEmpty(s: string) -> bool {
    @cpp { return s.empty(); }
}

pub fn charAt(s: string, index: int) -> string {
    @cpp { return std::string(1, s[index]); }
}

pub fn charCodeAt(s: string, index: int) -> int {
    @cpp { return static_cast<int>(s[index]); }
}

pub fn fromCharCode(code: int) -> string {
    @cpp { return std::string(1, static_cast<char>(code)); }
}

// ============================================================================
// Case conversion
// ============================================================================

pub fn toLower(s: string) -> string {
    @cpp {
        std::string result = s;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }
}

pub fn toUpper(s: string) -> string {
    @cpp {
        std::string result = s;
        std::transform(result.begin(), result.end(), result.begin(), ::toupper);
        return result;
    }
}

pub fn capitalize(s: string) -> string {
    if (isEmpty(s)) { return s; }
    return toUpper(charAt(s, 0)) + substring(s, 1);
}

pub fn titleCase(s: string) -> string {
    let words = split(s, " ");
    let mut result: Array<string> = [];
    for (word in words) {
        push(result, capitalize(toLower(word)));
    }
    return join(result, " ");
}

// ============================================================================
// Trimming
// ============================================================================

pub fn trim(s: string) -> string {
    @cpp {
        size_t start = s.find_first_not_of(" \t\n\r\f\v");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t\n\r\f\v");
        return s.substr(start, end - start + 1);
    }
}

pub fn trimStart(s: string) -> string {
    @cpp {
        size_t start = s.find_first_not_of(" \t\n\r\f\v");
        if (start == std::string::npos) return "";
        return s.substr(start);
    }
}

pub fn trimEnd(s: string) -> string {
    @cpp {
        size_t end = s.find_last_not_of(" \t\n\r\f\v");
        if (end == std::string::npos) return "";
        return s.substr(0, end + 1);
    }
}

// ============================================================================
// Search operations
// ============================================================================

pub fn contains(s: string, substr: string) -> bool {
    @cpp { return s.find(substr) != std::string::npos; }
}

pub fn startsWith(s: string, prefix: string) -> bool {
    @cpp {
        return s.size() >= prefix.size() && 
               s.compare(0, prefix.size(), prefix) == 0;
    }
}

pub fn endsWith(s: string, suffix: string) -> bool {
    @cpp {
        return s.size() >= suffix.size() && 
               s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
    }
}

pub fn indexOf(s: string, substr: string) -> Option<int> {
    @cpp {
        size_t pos = s.find(substr);
        if (pos != std::string::npos) {
            return std::make_optional(static_cast<int>(pos));
        }
        return std::nullopt;
    }
}

pub fn lastIndexOf(s: string, substr: string) -> Option<int> {
    @cpp {
        size_t pos = s.rfind(substr);
        if (pos != std::string::npos) {
            return std::make_optional(static_cast<int>(pos));
        }
        return std::nullopt;
    }
}

pub fn count(s: string, substr: string) -> int {
    @cpp {
        int count = 0;
        size_t pos = 0;
        while ((pos = s.find(substr, pos)) != std::string::npos) {
            count++;
            pos += substr.length();
        }
        return count;
    }
}

// ============================================================================
// Substring operations
// ============================================================================

pub fn substring(s: string, start: int) -> string {
    @cpp { return s.substr(start); }
}

pub fn substringLen(s: string, start: int, len: int) -> string {
    @cpp { return s.substr(start, len); }
}

pub fn slice(s: string, start: int, endIdx: int) -> string {
    @cpp { return s.substr(start, endIdx - start); }
}

// ============================================================================
// Replacement
// ============================================================================

pub fn replace(s: string, from: string, to: string) -> string {
    @cpp {
        std::string result = s;
        size_t pos = 0;
        while ((pos = result.find(from, pos)) != std::string::npos) {
            result.replace(pos, from.length(), to);
            pos += to.length();
        }
        return result;
    }
}

pub fn replaceFirst(s: string, from: string, to: string) -> string {
    @cpp {
        std::string result = s;
        size_t pos = result.find(from);
        if (pos != std::string::npos) {
            result.replace(pos, from.length(), to);
        }
        return result;
    }
}

pub fn remove(s: string, substr: string) -> string {
    return replace(s, substr, "");
}

// ============================================================================
// Split and join
// ============================================================================

pub fn split(s: string, delim: string) -> Array<string> {
    @cpp {
        std::vector<std::string> result;
        size_t start = 0;
        size_t end;
        while ((end = s.find(delim, start)) != std::string::npos) {
            result.push_back(s.substr(start, end - start));
            start = end + delim.length();
        }
        result.push_back(s.substr(start));
        return result;
    }
}

pub fn splitChar(s: string, delim: string) -> Array<string> {
    @cpp {
        std::vector<std::string> result;
        std::stringstream ss(s);
        std::string token;
        char d = delim[0];
        while (std::getline(ss, token, d)) {
            result.push_back(token);
        }
        return result;
    }
}

pub fn splitLines(s: string) -> Array<string> {
    return split(replace(s, "\r\n", "\n"), "\n");
}

pub fn splitWhitespace(s: string) -> Array<string> {
    @cpp {
        std::vector<std::string> result;
        std::stringstream ss(s);
        std::string word;
        while (ss >> word) {
            result.push_back(word);
        }
        return result;
    }
}

pub fn join(parts: Array<string>, sep: string) -> string {
    @cpp {
        std::string result;
        for (size_t i = 0; i < parts.size(); i++) {
            if (i > 0) result += sep;
            result += parts[i];
        }
        return result;
    }
}

// ============================================================================
// Repetition and padding
// ============================================================================

pub fn repeat(s: string, count: int) -> string {
    @cpp {
        std::string result;
        result.reserve(s.length() * count);
        for (int i = 0; i < count; i++) {
            result += s;
        }
        return result;
    }
}

pub fn padStart(s: string, targetLen: int, padStr: string) -> string {
    let padLen = targetLen - length(s);
    if (padLen <= 0) { return s; }
    let fullPad = repeat(padStr, (padLen / length(padStr)) + 1);
    return substringLen(fullPad, 0, padLen) + s;
}

pub fn padEnd(s: string, targetLen: int, padStr: string) -> string {
    let padLen = targetLen - length(s);
    if (padLen <= 0) { return s; }
    let fullPad = repeat(padStr, (padLen / length(padStr)) + 1);
    return s + substringLen(fullPad, 0, padLen);
}

// ============================================================================
// Reversal
// ============================================================================

pub fn reverse(s: string) -> string {
    @cpp {
        std::string result = s;
        std::reverse(result.begin(), result.end());
        return result;
    }
}

// ============================================================================
// Character classification
// ============================================================================

pub fn isDigit(c: string) -> bool {
    @cpp { return !c.empty() && std::isdigit(c[0]); }
}

pub fn isAlpha(c: string) -> bool {
    @cpp { return !c.empty() && std::isalpha(c[0]); }
}

pub fn isAlphaNum(c: string) -> bool {
    @cpp { return !c.empty() && std::isalnum(c[0]); }
}

pub fn isWhitespace(c: string) -> bool {
    @cpp { return !c.empty() && std::isspace(c[0]); }
}

pub fn isUpper(c: string) -> bool {
    @cpp { return !c.empty() && std::isupper(c[0]); }
}

pub fn isLower(c: string) -> bool {
    @cpp { return !c.empty() && std::islower(c[0]); }
}

// ============================================================================
// Parsing
// ============================================================================

pub fn parseInt(s: string) -> Option<int> {
    @cpp {
        try {
            size_t pos;
            int val = std::stoi(s, &pos);
            if (pos == s.length()) return std::make_optional(val);
            return std::nullopt;
        } catch (...) {
            return std::nullopt;
        }
    }
}

pub fn parseFloat(s: string) -> Option<float> {
    @cpp {
        try {
            size_t pos;
            double val = std::stod(s, &pos);
            if (pos == s.length()) return std::make_optional(val);
            return std::nullopt;
        } catch (...) {
            return std::nullopt;
        }
    }
}

pub fn parseBool(s: string) -> Option<bool> {
    let lower = toLower(trim(s));
    if (lower == "true" || lower == "1" || lower == "yes") {
        return Some(true);
    }
    if (lower == "false" || lower == "0" || lower == "no") {
        return Some(false);
    }
    return None;
}

// ============================================================================
// Formatting
// ============================================================================

pub fn format(template: string, args: Map<string, string>) -> string {
    let mut result = template;
    for (key in args.keys()) {
        result = replace(result, "{" + key + "}", args.get(key));
    }
    return result;
}

// Escape special characters for various contexts
pub fn escapeHtml(s: string) -> string {
    let mut result = s;
    result = replace(result, "&", "&amp;");
    result = replace(result, "<", "&lt;");
    result = replace(result, ">", "&gt;");
    result = replace(result, "\"", "&quot;");
    result = replace(result, "'", "&#39;");
    return result;
}

pub fn escapeJson(s: string) -> string {
    let mut result = s;
    result = replace(result, "\\", "\\\\");
    result = replace(result, "\"", "\\\"");
    result = replace(result, "\n", "\\n");
    result = replace(result, "\r", "\\r");
    result = replace(result, "\t", "\\t");
    return result;
}
