// Std.Map - HashMap/Dictionary operations
// Key-value storage with O(1) average access

// ============================================================================
// String-String Map operations (most common use case)
// ============================================================================

pub fn sizeStrStr(map: Map<string, string>) -> int {
    @cpp { return static_cast<int64_t>(map.size()); }
}

pub fn isEmptyStrStr(map: Map<string, string>) -> bool {
    @cpp { return map.empty(); }
}

pub fn clearStrStr(map: Map<string, string>) -> Map<string, string> {
    @cpp { return std::unordered_map<std::string, std::string>(); }
}

pub fn getStrStr(map: Map<string, string>, key: string) -> Option<string> {
    @cpp {
        auto it = map.find(key);
        if (it != map.end()) {
            return std::make_optional(it->second);
        }
        return std::nullopt;
    }
}

pub fn getOrStrStr(map: Map<string, string>, key: string, defaultVal: string) -> string {
    @cpp {
        auto it = map.find(key);
        if (it != map.end()) {
            return it->second;
        }
        return defaultVal;
    }
}

pub fn containsStrStr(map: Map<string, string>, key: string) -> bool {
    @cpp { return map.find(key) != map.end(); }
}

pub fn insertStrStr(map: Map<string, string>, key: string, value: string) -> Map<string, string> {
    @cpp {
        auto result = map;
        result[key] = value;
        return result;
    }
}

pub fn setStrStr(map: Map<string, string>, key: string, value: string) -> Map<string, string> {
    @cpp {
        auto result = map;
        result[key] = value;
        return result;
    }
}

pub fn removeStrStr(map: Map<string, string>, key: string) -> Map<string, string> {
    @cpp {
        auto result = map;
        result.erase(key);
        return result;
    }
}

pub fn keysStrStr(map: Map<string, string>) -> Array<string> {
    @cpp {
        std::vector<std::string> result;
        result.reserve(map.size());
        for (const auto& pair : map) {
            result.push_back(pair.first);
        }
        return result;
    }
}

pub fn valuesStrStr(map: Map<string, string>) -> Array<string> {
    @cpp {
        std::vector<std::string> result;
        result.reserve(map.size());
        for (const auto& pair : map) {
            result.push_back(pair.second);
        }
        return result;
    }
}

// ============================================================================
// String-Int Map operations - FIXED: Use int64_t consistently
// ============================================================================

pub fn sizeStrInt(map: Map<string, int>) -> int {
    @cpp { return static_cast<int64_t>(map.size()); }
}

pub fn isEmptyStrInt(map: Map<string, int>) -> bool {
    @cpp { return map.empty(); }
}

pub fn clearStrInt(map: Map<string, int>) -> Map<string, int> {
    @cpp { return std::unordered_map<std::string, int64_t>(); }
}

pub fn getStrInt(map: Map<string, int>, key: string) -> Option<int> {
    @cpp {
        auto it = map.find(key);
        if (it != map.end()) {
            return std::make_optional(it->second);
        }
        return std::nullopt;
    }
}

pub fn getOrStrInt(map: Map<string, int>, key: string, defaultVal: int) -> int {
    @cpp {
        auto it = map.find(key);
        if (it != map.end()) {
            return it->second;
        }
        return defaultVal;
    }
}

pub fn containsStrInt(map: Map<string, int>, key: string) -> bool {
    @cpp { return map.find(key) != map.end(); }
}

pub fn insertStrInt(map: Map<string, int>, key: string, value: int) -> Map<string, int> {
    @cpp {
        std::unordered_map<std::string, int64_t> result = map;
        result[key] = value;
        return result;
    }
}

pub fn setStrInt(map: Map<string, int>, key: string, value: int) -> Map<string, int> {
    @cpp {
        std::unordered_map<std::string, int64_t> result = map;
        result[key] = value;
        return result;
    }
}

pub fn removeStrInt(map: Map<string, int>, key: string) -> Map<string, int> {
    @cpp {
        std::unordered_map<std::string, int64_t> result = map;
        result.erase(key);
        return result;
    }
}

pub fn keysStrInt(map: Map<string, int>) -> Array<string> {
    @cpp {
        std::vector<std::string> result;
        result.reserve(map.size());
        for (const auto& pair : map) {
            result.push_back(pair.first);
        }
        return result;
    }
}

pub fn valuesStrInt(map: Map<string, int>) -> Array<int> {
    @cpp {
        std::vector<int64_t> result;
        result.reserve(map.size());
        for (const auto& pair : map) {
            result.push_back(pair.second);
        }
        return result;
    }
}

pub fn incrementStrInt(map: Map<string, int>, key: string) -> Map<string, int> {
    @cpp {
        std::unordered_map<std::string, int64_t> result = map;
        result[key]++;
        return result;
    }
}

pub fn decrementStrInt(map: Map<string, int>, key: string) -> Map<string, int> {
    @cpp {
        std::unordered_map<std::string, int64_t> result = map;
        result[key]--;
        return result;
    }
}

// ============================================================================
// Int-Int Map operations - FIXED: Use int64_t consistently
// ============================================================================

pub fn sizeIntInt(map: Map<int, int>) -> int {
    @cpp { return static_cast<int64_t>(map.size()); }
}

pub fn isEmptyIntInt(map: Map<int, int>) -> bool {
    @cpp { return map.empty(); }
}

pub fn getIntInt(map: Map<int, int>, key: int) -> Option<int> {
    @cpp {
        auto it = map.find(key);
        if (it != map.end()) {
            return std::make_optional(it->second);
        }
        return std::nullopt;
    }
}

pub fn getOrIntInt(map: Map<int, int>, key: int, defaultVal: int) -> int {
    @cpp {
        auto it = map.find(key);
        if (it != map.end()) {
            return it->second;
        }
        return defaultVal;
    }
}

pub fn containsIntInt(map: Map<int, int>, key: int) -> bool {
    @cpp { return map.find(key) != map.end(); }
}

pub fn insertIntInt(map: Map<int, int>, key: int, value: int) -> Map<int, int> {
    @cpp {
        std::unordered_map<int64_t, int64_t> result = map;
        result[key] = value;
        return result;
    }
}

pub fn removeIntInt(map: Map<int, int>, key: int) -> Map<int, int> {
    @cpp {
        std::unordered_map<int64_t, int64_t> result = map;
        result.erase(key);
        return result;
    }
}

pub fn keysIntInt(map: Map<int, int>) -> Array<int> {
    @cpp {
        std::vector<int64_t> result;
        result.reserve(map.size());
        for (const auto& pair : map) {
            result.push_back(pair.first);
        }
        return result;
    }
}

pub fn valuesIntInt(map: Map<int, int>) -> Array<int> {
    @cpp {
        std::vector<int64_t> result;
        result.reserve(map.size());
        for (const auto& pair : map) {
            result.push_back(pair.second);
        }
        return result;
    }
}

// ============================================================================
// Int-String Map operations - FIXED: Use int64_t consistently
// ============================================================================

pub fn sizeIntStr(map: Map<int, string>) -> int {
    @cpp { return static_cast<int64_t>(map.size()); }
}

pub fn getIntStr(map: Map<int, string>, key: int) -> Option<string> {
    @cpp {
        auto it = map.find(key);
        if (it != map.end()) {
            return std::make_optional(it->second);
        }
        return std::nullopt;
    }
}

pub fn getOrIntStr(map: Map<int, string>, key: int, defaultVal: string) -> string {
    @cpp {
        auto it = map.find(key);
        if (it != map.end()) {
            return it->second;
        }
        return defaultVal;
    }
}

pub fn containsIntStr(map: Map<int, string>, key: int) -> bool {
    @cpp { return map.find(key) != map.end(); }
}

pub fn insertIntStr(map: Map<int, string>, key: int, value: string) -> Map<int, string> {
    @cpp {
        std::unordered_map<int64_t, std::string> result = map;
        result[key] = value;
        return result;
    }
}

pub fn removeIntStr(map: Map<int, string>, key: int) -> Map<int, string> {
    @cpp {
        std::unordered_map<int64_t, std::string> result = map;
        result.erase(key);
        return result;
    }
}
