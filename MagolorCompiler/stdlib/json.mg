// Std.Json - JSON parsing and generation
// Full JSON support with typed access

using Std.Core.Prelude;

// ============================================================================
// JSON Value types
// ============================================================================

pub class JsonValue {
    pub kind: string;  // "null", "bool", "number", "string", "array", "object"
    pub boolVal: bool;
    pub numVal: float;
    pub strVal: string;
    pub arrVal: Array<JsonValue>;
    pub objVal: Map<string, JsonValue>;
    
    pub fn create() {
        this.kind = "null";
        this.boolVal = false;
        this.numVal = 0.0;
        this.strVal = "";
    }
}

// ============================================================================
// Constructors
// ============================================================================

pub fn jsonNull() -> JsonValue {
    let mut v = new JsonValue();
    v.kind = "null";
    return v;
}

pub fn jsonBool(b: bool) -> JsonValue {
    let mut v = new JsonValue();
    v.kind = "bool";
    v.boolVal = b;
    return v;
}

pub fn jsonNumber(n: float) -> JsonValue {
    let mut v = new JsonValue();
    v.kind = "number";
    v.numVal = n;
    return v;
}

pub fn jsonInt(n: int) -> JsonValue {
    return jsonNumber(toFloat(n));
}

pub fn jsonString(s: string) -> JsonValue {
    let mut v = new JsonValue();
    v.kind = "string";
    v.strVal = s;
    return v;
}

pub fn jsonArray(arr: Array<JsonValue>) -> JsonValue {
    let mut v = new JsonValue();
    v.kind = "array";
    v.arrVal = arr;
    return v;
}

pub fn jsonObject(obj: Map<string, JsonValue>) -> JsonValue {
    let mut v = new JsonValue();
    v.kind = "object";
    v.objVal = obj;
    return v;
}

// ============================================================================
// Type checking
// ============================================================================

pub fn isNull(v: JsonValue) -> bool { return v.kind == "null"; }
pub fn isBool(v: JsonValue) -> bool { return v.kind == "bool"; }
pub fn isNumber(v: JsonValue) -> bool { return v.kind == "number"; }
pub fn isString(v: JsonValue) -> bool { return v.kind == "string"; }
pub fn isArray(v: JsonValue) -> bool { return v.kind == "array"; }
pub fn isObject(v: JsonValue) -> bool { return v.kind == "object"; }

// ============================================================================
// Value extraction
// ============================================================================

pub fn asBool(v: JsonValue) -> Option<bool> {
    if (v.kind != "bool") { return None; }
    return Some(v.boolVal);
}

pub fn asNumber(v: JsonValue) -> Option<float> {
    if (v.kind != "number") { return None; }
    return Some(v.numVal);
}

pub fn asInt(v: JsonValue) -> Option<int> {
    if (v.kind != "number") { return None; }
    return Some(toInt(v.numVal));
}

pub fn asString(v: JsonValue) -> Option<string> {
    if (v.kind != "string") { return None; }
    return Some(v.strVal);
}

pub fn asArray(v: JsonValue) -> Option<Array<JsonValue>> {
    if (v.kind != "array") { return None; }
    return Some(v.arrVal);
}

pub fn asObject(v: JsonValue) -> Option<Map<string, JsonValue>> {
    if (v.kind != "object") { return None; }
    return Some(v.objVal);
}

// ============================================================================
// Object access
// ============================================================================

pub fn get(v: JsonValue, key: string) -> Option<JsonValue> {
    if (v.kind != "object") { return None; }
    return Map.get(v.objVal, key);
}

pub fn getPath(v: JsonValue, path: Array<string>) -> Option<JsonValue> {
    let mut current = v;
    for (key in path) {
        let next = get(current, key);
        match next {
            Some(val) => { current = val; },
            None => return None
        }
    }
    return Some(current);
}

pub fn has(v: JsonValue, key: string) -> bool {
    if (v.kind != "object") { return false; }
    return Map.contains(v.objVal, key);
}

pub fn keys(v: JsonValue) -> Array<string> {
    if (v.kind != "object") { return []; }
    return Map.keys(v.objVal);
}

// ============================================================================
// Array access
// ============================================================================

pub fn at(v: JsonValue, index: int) -> Option<JsonValue> {
    if (v.kind != "array") { return None; }
    return Array.get(v.arrVal, index);
}

pub fn len(v: JsonValue) -> int {
    if (v.kind == "array") { return length(v.arrVal); }
    if (v.kind == "object") { return Map.size(v.objVal); }
    if (v.kind == "string") { return length(v.strVal); }
    return 0;
}

// ============================================================================
// Parsing
// ============================================================================

pub fn parse(json: string) -> Option<JsonValue> {
    @cpp {
        // Simplified JSON parser
        size_t pos = 0;
        
        std::function<JsonValue(const std::string&, size_t&)> parseValue;
        
        auto skipWhitespace = [](const std::string& s, size_t& pos) {
            while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t' || 
                                       s[pos] == '\n' || s[pos] == '\r')) {
                pos++;
            }
        };
        
        std::function<std::string(const std::string&, size_t&)> parseString = 
            [](const std::string& s, size_t& pos) -> std::string {
            pos++; // skip opening quote
            std::string result;
            while (pos < s.size() && s[pos] != '"') {
                if (s[pos] == '\\' && pos + 1 < s.size()) {
                    pos++;
                    switch (s[pos]) {
                        case 'n': result += '\n'; break;
                        case 't': result += '\t'; break;
                        case 'r': result += '\r'; break;
                        case '"': result += '"'; break;
                        case '\\': result += '\\'; break;
                        default: result += s[pos];
                    }
                } else {
                    result += s[pos];
                }
                pos++;
            }
            pos++; // skip closing quote
            return result;
        };
        
        parseValue = [&](const std::string& s, size_t& pos) -> JsonValue {
            skipWhitespace(s, pos);
            
            JsonValue result;
            result.kind = "null";
            
            if (pos >= s.size()) return result;
            
            char c = s[pos];
            
            if (c == 'n') { // null
                pos += 4;
                result.kind = "null";
            } else if (c == 't') { // true
                pos += 4;
                result.kind = "bool";
                result.boolVal = true;
            } else if (c == 'f') { // false
                pos += 5;
                result.kind = "bool";
                result.boolVal = false;
            } else if (c == '"') {
                result.kind = "string";
                result.strVal = parseString(s, pos);
            } else if (c == '-' || (c >= '0' && c <= '9')) {
                size_t start = pos;
                if (s[pos] == '-') pos++;
                while (pos < s.size() && ((s[pos] >= '0' && s[pos] <= '9') || s[pos] == '.')) {
                    pos++;
                }
                result.kind = "number";
                result.numVal = std::stod(s.substr(start, pos - start));
            } else if (c == '[') {
                pos++;
                result.kind = "array";
                skipWhitespace(s, pos);
                if (s[pos] != ']') {
                    result.arrVal.push_back(parseValue(s, pos));
                    skipWhitespace(s, pos);
                    while (pos < s.size() && s[pos] == ',') {
                        pos++;
                        result.arrVal.push_back(parseValue(s, pos));
                        skipWhitespace(s, pos);
                    }
                }
                pos++; // skip ]
            } else if (c == '{') {
                pos++;
                result.kind = "object";
                skipWhitespace(s, pos);
                if (s[pos] != '}') {
                    skipWhitespace(s, pos);
                    std::string key = parseString(s, pos);
                    skipWhitespace(s, pos);
                    pos++; // skip :
                    result.objVal[key] = parseValue(s, pos);
                    skipWhitespace(s, pos);
                    while (pos < s.size() && s[pos] == ',') {
                        pos++;
                        skipWhitespace(s, pos);
                        key = parseString(s, pos);
                        skipWhitespace(s, pos);
                        pos++; // skip :
                        result.objVal[key] = parseValue(s, pos);
                        skipWhitespace(s, pos);
                    }
                }
                pos++; // skip }
            }
            
            return result;
        };
        
        try {
            return std::make_optional(parseValue(json, pos));
        } catch (...) {
            return std::nullopt;
        }
    }
}

// ============================================================================
// Serialization
// ============================================================================

pub fn stringify(v: JsonValue) -> string {
    return stringifyIndent(v, 0, "");
}

pub fn stringifyPretty(v: JsonValue) -> string {
    return stringifyIndent(v, 0, "  ");
}

fn stringifyIndent(v: JsonValue, depth: int, indent: string) -> string {
    match v.kind {
        "null" => return "null",
        "bool" => return if v.boolVal { "true" } else { "false" },
        "number" => return toString(v.numVal),
        "string" => return "\"" + escapeJson(v.strVal) + "\"",
        "array" => {
            if (isEmpty(v.arrVal)) { return "[]"; }
            let mut parts: Array<string> = [];
            for (item in v.arrVal) {
                push(parts, stringifyIndent(item, depth + 1, indent));
            }
            if (indent == "") {
                return "[" + join(parts, ",") + "]";
            }
            let nl = "\n" + repeat(indent, depth + 1);
            let nlClose = "\n" + repeat(indent, depth);
            return "[" + nl + join(parts, "," + nl) + nlClose + "]";
        },
        "object" => {
            if (Map.isEmpty(v.objVal)) { return "{}"; }
            let mut parts: Array<string> = [];
            for (key in Map.keys(v.objVal)) {
                let val = unwrap(Map.get(v.objVal, key));
                push(parts, "\"" + escapeJson(key) + "\":" + 
                     (if indent != "" { " " } else { "" }) +
                     stringifyIndent(val, depth + 1, indent));
            }
            if (indent == "") {
                return "{" + join(parts, ",") + "}";
            }
            let nl = "\n" + repeat(indent, depth + 1);
            let nlClose = "\n" + repeat(indent, depth);
            return "{" + nl + join(parts, "," + nl) + nlClose + "}";
        }
    }
    return "null";
}

fn escapeJson(s: string) -> string {
    let mut result = s;
    result = replace(result, "\\", "\\\\");
    result = replace(result, "\"", "\\\"");
    result = replace(result, "\n", "\\n");
    result = replace(result, "\r", "\\r");
    result = replace(result, "\t", "\\t");
    return result;
}
