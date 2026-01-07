// Std.String - String operations

pub fn length(s: string) -> int {
    @cpp {
        return static_cast<int64_t>(s.length());
    }
}

pub fn isEmpty(s: string) -> bool {
    @cpp {
        return s.empty();
    }
}

pub fn charAt(s: string, index: int) -> Option<string> {
    @cpp {
        if (index < 0 || static_cast<size_t>(index) >= s.length()) return std::nullopt;
        return std::string(1, s[static_cast<size_t>(index)]);
    }
}

pub fn substring(s: string, start: int, end: int) -> string {
    @cpp {
        int64_t st = start;
        int64_t en = end;
        if (st < 0) st = 0;
        if (en > static_cast<int64_t>(s.length())) en = s.length();
        if (st >= en) return "";
        return s.substr(static_cast<size_t>(st), static_cast<size_t>(en - st));
    }
}

pub fn slice(s: string, start: int, end: int) -> string {
    @cpp {
        int64_t st = start;
        int64_t en = end;
        if (st < 0) st = 0;
        if (en > static_cast<int64_t>(s.length())) en = s.length();
        if (st >= en) return "";
        return s.substr(static_cast<size_t>(st), static_cast<size_t>(en - st));
    }
}

pub fn toUpper(s: string) -> string {
    @cpp {
        std::string result = s;
        std::transform(result.begin(), result.end(), result.begin(), ::toupper);
        return result;
    }
}

pub fn toLower(s: string) -> string {
    @cpp {
        std::string result = s;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }
}

pub fn trim(s: string) -> string {
    @cpp {
        size_t start = s.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t\n\r");
        return s.substr(start, end - start + 1);
    }
}

pub fn trimStart(s: string) -> string {
    @cpp {
        size_t start = s.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return "";
        return s.substr(start);
    }
}

pub fn trimEnd(s: string) -> string {
    @cpp {
        size_t end = s.find_last_not_of(" \t\n\r");
        if (end == std::string::npos) return "";
        return s.substr(0, end + 1);
    }
}

pub fn startsWith(s: string, prefix: string) -> bool {
    @cpp {
        if (prefix.length() > s.length()) return false;
        return s.compare(0, prefix.length(), prefix) == 0;
    }
}

pub fn endsWith(s: string, suffix: string) -> bool {
    @cpp {
        if (suffix.length() > s.length()) return false;
        return s.compare(s.length() - suffix.length(), suffix.length(), suffix) == 0;
    }
}

pub fn contains(s: string, substr: string) -> bool {
    @cpp {
        return s.find(substr) != std::string::npos;
    }
}

pub fn indexOf(s: string, substr: string) -> Option<int> {
    @cpp {
        size_t pos = s.find(substr);
        if (pos == std::string::npos) return std::nullopt;
        return static_cast<int64_t>(pos);
    }
}

pub fn lastIndexOf(s: string, substr: string) -> Option<int> {
    @cpp {
        size_t pos = s.rfind(substr);
        if (pos == std::string::npos) return std::nullopt;
        return static_cast<int64_t>(pos);
    }
}

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
    @cpp {
        std::string result = s;
        size_t pos = 0;
        while ((pos = result.find(substr, pos)) != std::string::npos) {
            result.erase(pos, substr.length());
        }
        return result;
    }
}

pub fn split(s: string, delim: string) -> Array<string> {
    @cpp {
        std::vector<std::string> result;
        if (delim.empty()) {
            result.push_back(s);
            return result;
        }
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
        if (delim.empty()) {
            result.push_back(s);
            return result;
        }
        char d = delim[0];
        size_t start = 0;
        for (size_t i = 0; i < s.length(); i++) {
            if (s[i] == d) {
                result.push_back(s.substr(start, i - start));
                start = i + 1;
            }
        }
        result.push_back(s.substr(start));
        return result;
    }
}

pub fn splitLines(s: string) -> Array<string> {
    @cpp {
        std::vector<std::string> result;
        std::istringstream stream(s);
        std::string line;
        while (std::getline(stream, line)) {
            result.push_back(line);
        }
        return result;
    }
}

pub fn splitWhitespace(s: string) -> Array<string> {
    @cpp {
        std::vector<std::string> result;
        std::istringstream stream(s);
        std::string word;
        while (stream >> word) {
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

pub fn repeat(s: string, count: int) -> string {
    @cpp {
        std::string result;
        result.reserve(s.length() * count);
        for (int64_t i = 0; i < count; i++) {
            result += s;
        }
        return result;
    }
}

pub fn reverse(s: string) -> string {
    @cpp {
        std::string result = s;
        std::reverse(result.begin(), result.end());
        return result;
    }
}

pub fn padStart(s: string, length: int, pad: string) -> string {
    @cpp {
        if (static_cast<int64_t>(s.length()) >= length || pad.empty()) return s;
        std::string result;
        int64_t needed = length - s.length();
        while (static_cast<int64_t>(result.length()) < needed) {
            result += pad;
        }
        return result.substr(0, needed) + s;
    }
}

pub fn padEnd(s: string, length: int, pad: string) -> string {
    @cpp {
        if (static_cast<int64_t>(s.length()) >= length || pad.empty()) return s;
        std::string result = s;
        while (static_cast<int64_t>(result.length()) < length) {
            result += pad;
        }
        return result.substr(0, length);
    }
}

pub fn capitalize(s: string) -> string {
    @cpp {
        if (s.empty()) return s;
        std::string result = s;
        result[0] = std::toupper(result[0]);
        return result;
    }
}

pub fn titleCase(s: string) -> string {
    @cpp {
        std::string result = s;
        bool newWord = true;
        for (size_t i = 0; i < result.length(); i++) {
            if (std::isspace(result[i])) {
                newWord = true;
            } else if (newWord) {
                result[i] = std::toupper(result[i]);
                newWord = false;
            }
        }
        return result;
    }
}

pub fn parseInt(s: string) -> Option<int> {
    @cpp {
        try {
            size_t pos;
            int64_t val = std::stoll(s, &pos);
            if (pos != s.length()) return std::nullopt;
            return val;
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
            if (pos != s.length()) return std::nullopt;
            return val;
        } catch (...) {
            return std::nullopt;
        }
    }
}

pub fn parseBool(s: string) -> Option<bool> {
    @cpp {
        std::string lower = s;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower == "true" || lower == "1" || lower == "yes") return true;
        if (lower == "false" || lower == "0" || lower == "no") return false;
        return std::nullopt;
    }
}

pub fn formatTemplate(tmpl: string, args: Map<string, string>) -> string {
    @cpp {
        std::string result = tmpl;
        for (const auto& pair : args) {
            std::string placeholder = "{" + pair.first + "}";
            size_t pos = 0;
            while ((pos = result.find(placeholder, pos)) != std::string::npos) {
                result.replace(pos, placeholder.length(), pair.second);
                pos += pair.second.length();
            }
        }
        return result;
    }
}

pub fn escapeHtml(s: string) -> string {
    @cpp {
        std::string result;
        result.reserve(s.length() * 2);
        for (char c : s) {
            switch (c) {
                case '&': result += "&amp;"; break;
                case '<': result += "&lt;"; break;
                case '>': result += "&gt;"; break;
                case '"': result += "&quot;"; break;
                case '\'': result += "&#39;"; break;
                default: result += c; break;
            }
        }
        return result;
    }
}

pub fn escapeJson(s: string) -> string {
    @cpp {
        std::string result;
        result.reserve(s.length() * 2);
        for (char c : s) {
            switch (c) {
                case '\\': result += "\\\\"; break;
                case '"': result += "\\\""; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default: result += c; break;
            }
        }
        return result;
    }
}

pub fn toCharArray(s: string) -> Array<string> {
    @cpp {
        std::vector<std::string> result;
        for (char c : s) {
            result.push_back(std::string(1, c));
        }
        return result;
    }
}

pub fn fromCharCodes(codes: Array<int>) -> string {
    @cpp {
        std::string result;
        for (auto code : codes) {
            result += static_cast<char>(code);
        }
        return result;
    }
}
