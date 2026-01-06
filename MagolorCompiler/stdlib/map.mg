// Std.Map - HashMap/Dictionary operations
// Key-value storage with O(1) average access

using Std.Core.Prelude;

// ============================================================================
// Basic operations
// ============================================================================

pub fn size(map: Map<any, any>) -> int {
    @cpp { return map.size(); }
}

pub fn isEmpty(map: Map<any, any>) -> bool {
    @cpp { return map.empty(); }
}

pub fn clear(map: Map<any, any>) {
    @cpp { map.clear(); }
}

// ============================================================================
// Element access
// ============================================================================

pub fn get(map: Map<any, any>, key: any) -> Option<any> {
    @cpp {
        auto it = map.find(key);
        if (it != map.end()) {
            return std::make_optional(it->second);
        }
        return std::nullopt;
    }
}

pub fn getOr(map: Map<any, any>, key: any, defaultVal: any) -> any {
    @cpp {
        auto it = map.find(key);
        if (it != map.end()) {
            return it->second;
        }
        return defaultVal;
    }
}

pub fn contains(map: Map<any, any>, key: any) -> bool {
    @cpp { return map.find(key) != map.end(); }
}

// ============================================================================
// Modification
// ============================================================================

pub fn insert(map: Map<any, any>, key: any, value: any) {
    @cpp { map[key] = value; }
}

pub fn set(map: Map<any, any>, key: any, value: any) {
    @cpp { map[key] = value; }
}

pub fn remove(map: Map<any, any>, key: any) -> bool {
    @cpp {
        auto it = map.find(key);
        if (it != map.end()) {
            map.erase(it);
            return true;
        }
        return false;
    }
}

pub fn update(map: Map<any, any>, key: any, f: fn(any) -> any) {
    @cpp {
        auto it = map.find(key);
        if (it != map.end()) {
            it->second = f(it->second);
        }
    }
}

pub fn insertIfAbsent(map: Map<any, any>, key: any, value: any) -> bool {
    @cpp {
        if (map.find(key) == map.end()) {
            map[key] = value;
            return true;
        }
        return false;
    }
}

// ============================================================================
// Keys and values
// ============================================================================

pub fn keys(map: Map<any, any>) -> Array<any> {
    @cpp {
        std::vector<decltype(map)::key_type> result;
        result.reserve(map.size());
        for (const auto& pair : map) {
            result.push_back(pair.first);
        }
        return result;
    }
}

pub fn values(map: Map<any, any>) -> Array<any> {
    @cpp {
        std::vector<decltype(map)::mapped_type> result;
        result.reserve(map.size());
        for (const auto& pair : map) {
            result.push_back(pair.second);
        }
        return result;
    }
}

pub fn entries(map: Map<any, any>) -> Array<Array<any>> {
    @cpp {
        std::vector<std::pair<decltype(map)::key_type, decltype(map)::mapped_type>> result;
        result.reserve(map.size());
        for (const auto& pair : map) {
            result.push_back(pair);
        }
        return result;
    }
}

// ============================================================================
// Iteration
// ============================================================================

pub fn forEach(map: Map<any, any>, f: fn(any, any)) {
    @cpp {
        for (const auto& pair : map) {
            f(pair.first, pair.second);
        }
    }
}

pub fn mapValues(map: Map<any, any>, f: fn(any) -> any) -> Map<any, any> {
    @cpp {
        std::unordered_map<decltype(map)::key_type, decltype(f(map.begin()->second))> result;
        for (const auto& pair : map) {
            result[pair.first] = f(pair.second);
        }
        return result;
    }
}

pub fn filterMap(map: Map<any, any>, predicate: fn(any, any) -> bool) -> Map<any, any> {
    @cpp {
        decltype(map) result;
        for (const auto& pair : map) {
            if (predicate(pair.first, pair.second)) {
                result[pair.first] = pair.second;
            }
        }
        return result;
    }
}

// ============================================================================
// Merging
// ============================================================================

pub fn merge(a: Map<any, any>, b: Map<any, any>) -> Map<any, any> {
    @cpp {
        auto result = a;
        for (const auto& pair : b) {
            result[pair.first] = pair.second;
        }
        return result;
    }
}

pub fn mergeWith(a: Map<any, any>, b: Map<any, any>, f: fn(any, any) -> any) -> Map<any, any> {
    @cpp {
        auto result = a;
        for (const auto& pair : b) {
            auto it = result.find(pair.first);
            if (it != result.end()) {
                it->second = f(it->second, pair.second);
            } else {
                result[pair.first] = pair.second;
            }
        }
        return result;
    }
}

// ============================================================================
// Creation helpers
// ============================================================================

pub fn fromEntries(entries: Array<Array<any>>) -> Map<any, any> {
    @cpp {
        std::unordered_map<decltype(entries[0][0]), decltype(entries[0][1])> result;
        for (const auto& entry : entries) {
            if (entry.size() >= 2) {
                result[entry[0]] = entry[1];
            }
        }
        return result;
    }
}

pub fn invert(map: Map<any, any>) -> Map<any, any> {
    @cpp {
        std::unordered_map<decltype(map)::mapped_type, decltype(map)::key_type> result;
        for (const auto& pair : map) {
            result[pair.second] = pair.first;
        }
        return result;
    }
}

// ============================================================================
// Counting and grouping
// ============================================================================

pub fn countBy(arr: Array<any>, keyFn: fn(any) -> any) -> Map<any, int> {
    @cpp {
        std::unordered_map<decltype(keyFn(arr[0])), int> result;
        for (const auto& item : arr) {
            result[keyFn(item)]++;
        }
        return result;
    }
}

pub fn groupBy(arr: Array<any>, keyFn: fn(any) -> any) -> Map<any, Array<any>> {
    @cpp {
        std::unordered_map<decltype(keyFn(arr[0])), std::vector<decltype(arr)::value_type>> result;
        for (const auto& item : arr) {
            result[keyFn(item)].push_back(item);
        }
        return result;
    }
}
