// Std.Array - Array/Vector operations

pub fn lengthInt(arr: Array<int>) -> int {
    @cpp {
        return static_cast<int64_t>(arr.size());
    }
}

pub fn lengthStr(arr: Array<string>) -> int {
    @cpp {
        return static_cast<int64_t>(arr.size());
    }
}

pub fn isEmptyInt(arr: Array<int>) -> bool {
    @cpp {
        return arr.empty();
    }
}

pub fn isEmptyStr(arr: Array<string>) -> bool {
    @cpp {
        return arr.empty();
    }
}

// FIX: Return new array with value appended
pub fn pushInt(arr: Array<int>, value: int) -> Array<int> {
    @cpp {
        auto result = arr;
        result.push_back(value);
        return result;
    }
}

pub fn pushStr(arr: Array<string>, value: string) -> Array<string> {
    @cpp {
        auto result = arr;
        result.push_back(value);
        return result;
    }
}

// FIX: Return pair-like struct with popped value and new array
pub fn popInt(arr: Array<int>) -> int {
    @cpp {
        if (arr.empty()) return 0;
        return arr.back();
    }
}

pub fn popStr(arr: Array<string>) -> string {
    @cpp {
        if (arr.empty()) return "";
        return arr.back();
    }
}

// Helper to get array without last element
pub fn popArrayInt(arr: Array<int>) -> Array<int> {
    @cpp {
        if (arr.empty()) return arr;
        auto result = arr;
        result.pop_back();
        return result;
    }
}

pub fn popArrayStr(arr: Array<string>) -> Array<string> {
    @cpp {
        if (arr.empty()) return arr;
        auto result = arr;
        result.pop_back();
        return result;
    }
}

pub fn firstInt(arr: Array<int>) -> Option<int> {
    @cpp {
        if (arr.empty()) return std::nullopt;
        return arr.front();
    }
}

pub fn firstStr(arr: Array<string>) -> Option<string> {
    @cpp {
        if (arr.empty()) return std::nullopt;
        return arr.front();
    }
}

pub fn lastInt(arr: Array<int>) -> Option<int> {
    @cpp {
        if (arr.empty()) return std::nullopt;
        return arr.back();
    }
}

pub fn lastStr(arr: Array<string>) -> Option<string> {
    @cpp {
        if (arr.empty()) return std::nullopt;
        return arr.back();
    }
}

pub fn getInt(arr: Array<int>, index: int) -> Option<int> {
    @cpp {
        if (index < 0 || static_cast<size_t>(index) >= arr.size()) return std::nullopt;
        return arr[static_cast<size_t>(index)];
    }
}

pub fn getStr(arr: Array<string>, index: int) -> Option<string> {
    @cpp {
        if (index < 0 || static_cast<size_t>(index) >= arr.size()) return std::nullopt;
        return arr[static_cast<size_t>(index)];
    }
}

// FIX: Return new array with value set at index
pub fn setInt(arr: Array<int>, index: int, value: int) -> Array<int> {
    @cpp {
        if (index < 0 || static_cast<size_t>(index) >= arr.size()) return arr;
        auto result = arr;
        result[static_cast<size_t>(index)] = value;
        return result;
    }
}

pub fn setStr(arr: Array<string>, index: int, value: string) -> Array<string> {
    @cpp {
        if (index < 0 || static_cast<size_t>(index) >= arr.size()) return arr;
        auto result = arr;
        result[static_cast<size_t>(index)] = value;
        return result;
    }
}

pub fn sliceInt(arr: Array<int>, start: int, end: int) -> Array<int> {
    @cpp {
        std::vector<int64_t> result;
        int64_t s = start < 0 ? 0 : start;
        int64_t e = end > static_cast<int64_t>(arr.size()) ? arr.size() : end;
        for (int64_t i = s; i < e; i++) {
            result.push_back(arr[i]);
        }
        return result;
    }
}

pub fn sliceStr(arr: Array<string>, start: int, end: int) -> Array<string> {
    @cpp {
        std::vector<std::string> result;
        int64_t s = start < 0 ? 0 : start;
        int64_t e = end > static_cast<int64_t>(arr.size()) ? arr.size() : end;
        for (int64_t i = s; i < e; i++) {
            result.push_back(arr[i]);
        }
        return result;
    }
}

pub fn concatInt(a: Array<int>, b: Array<int>) -> Array<int> {
    @cpp {
        std::vector<int64_t> result = a;
        result.insert(result.end(), b.begin(), b.end());
        return result;
    }
}

pub fn concatStr(a: Array<string>, b: Array<string>) -> Array<string> {
    @cpp {
        std::vector<std::string> result = a;
        result.insert(result.end(), b.begin(), b.end());
        return result;
    }
}

pub fn reverseInt(arr: Array<int>) -> Array<int> {
    @cpp {
        std::vector<int64_t> result = arr;
        std::reverse(result.begin(), result.end());
        return result;
    }
}

pub fn reverseStr(arr: Array<string>) -> Array<string> {
    @cpp {
        std::vector<std::string> result = arr;
        std::reverse(result.begin(), result.end());
        return result;
    }
}

pub fn containsInt(arr: Array<int>, value: int) -> bool {
    @cpp {
        return std::find(arr.begin(), arr.end(), value) != arr.end();
    }
}

pub fn containsStr(arr: Array<string>, value: string) -> bool {
    @cpp {
        return std::find(arr.begin(), arr.end(), value) != arr.end();
    }
}

pub fn indexOfInt(arr: Array<int>, value: int) -> Option<int> {
    @cpp {
        auto it = std::find(arr.begin(), arr.end(), value);
        if (it == arr.end()) return std::nullopt;
        return static_cast<int64_t>(std::distance(arr.begin(), it));
    }
}

pub fn indexOfStr(arr: Array<string>, value: string) -> Option<int> {
    @cpp {
        auto it = std::find(arr.begin(), arr.end(), value);
        if (it == arr.end()) return std::nullopt;
        return static_cast<int64_t>(std::distance(arr.begin(), it));
    }
}

// FIX: Return new empty array
pub fn clearInt(arr: Array<int>) -> Array<int> {
    @cpp {
        return std::vector<int64_t>();
    }
}

pub fn clearStr(arr: Array<string>) -> Array<string> {
    @cpp {
        return std::vector<std::string>();
    }
}

// Creates array filled with value
pub fn filled(size: int, value: int) -> Array<int> {
    @cpp {
        return std::vector<int64_t>(static_cast<size_t>(size), static_cast<int64_t>(value));
    }
}

// Creates array of zeros
pub fn zeros(size: int) -> Array<int> {
    @cpp {
        return std::vector<int64_t>(static_cast<size_t>(size), 0);
    }
}

// Creates array of ones
pub fn ones(size: int) -> Array<int> {
    @cpp {
        return std::vector<int64_t>(static_cast<size_t>(size), 1);
    }
}

pub fn range(start: int, end: int) -> Array<int> {
    @cpp {
        std::vector<int64_t> result;
        for (int64_t i = start; i < end; i++) {
            result.push_back(i);
        }
        return result;
    }
}

pub fn sum(arr: Array<int>) -> int {
    @cpp {
        int64_t total = 0;
        for (auto v : arr) total += v;
        return total;
    }
}

pub fn min(arr: Array<int>) -> Option<int> {
    @cpp {
        if (arr.empty()) return std::nullopt;
        return *std::min_element(arr.begin(), arr.end());
    }
}

pub fn max(arr: Array<int>) -> Option<int> {
    @cpp {
        if (arr.empty()) return std::nullopt;
        return *std::max_element(arr.begin(), arr.end());
    }
}

pub fn sort(arr: Array<int>) -> Array<int> {
    @cpp {
        std::vector<int64_t> result = arr;
        std::sort(result.begin(), result.end());
        return result;
    }
}

pub fn sortDesc(arr: Array<int>) -> Array<int> {
    @cpp {
        std::vector<int64_t> result = arr;
        std::sort(result.begin(), result.end(), std::greater<int64_t>());
        return result;
    }
}

pub fn unique(arr: Array<int>) -> Array<int> {
    @cpp {
        std::vector<int64_t> result = arr;
        std::sort(result.begin(), result.end());
        result.erase(std::unique(result.begin(), result.end()), result.end());
        return result;
    }
}

pub fn sortStr(arr: Array<string>) -> Array<string> {
    @cpp {
        std::vector<std::string> result = arr;
        std::sort(result.begin(), result.end());
        return result;
    }
}

pub fn uniqueStr(arr: Array<string>) -> Array<string> {
    @cpp {
        std::vector<std::string> result = arr;
        std::sort(result.begin(), result.end());
        result.erase(std::unique(result.begin(), result.end()), result.end());
        return result;
    }
}
