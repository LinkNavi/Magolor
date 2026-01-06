// Std.Array - Array/Vector operations
// Dynamic array operations and utilities

using Std.Core.Prelude;

// ============================================================================
// Basic operations
// ============================================================================

pub fn length(arr: Array<any>) -> int {
    @cpp { return arr.size(); }
}

pub fn isEmpty(arr: Array<any>) -> bool {
    @cpp { return arr.empty(); }
}

pub fn capacity(arr: Array<any>) -> int {
    @cpp { return arr.capacity(); }
}

// ============================================================================
// Element access
// ============================================================================

pub fn get(arr: Array<any>, index: int) -> Option<any> {
    @cpp {
        if (index >= 0 && index < static_cast<int>(arr.size())) {
            return std::make_optional(arr[index]);
        }
        return std::nullopt;
    }
}

pub fn first(arr: Array<any>) -> Option<any> {
    if (isEmpty(arr)) { return None; }
    return Some(arr[0]);
}

pub fn last(arr: Array<any>) -> Option<any> {
    if (isEmpty(arr)) { return None; }
    return Some(arr[length(arr) - 1]);
}

// ============================================================================
// Modification
// ============================================================================

pub fn push(arr: Array<any>, item: any) {
    @cpp { arr.push_back(item); }
}

pub fn pop(arr: Array<any>) -> Option<any> {
    @cpp {
        if (arr.empty()) return std::nullopt;
        auto item = arr.back();
        arr.pop_back();
        return std::make_optional(item);
    }
}

pub fn insert(arr: Array<any>, index: int, item: any) {
    @cpp { arr.insert(arr.begin() + index, item); }
}

pub fn removeAt(arr: Array<any>, index: int) -> Option<any> {
    @cpp {
        if (index < 0 || index >= static_cast<int>(arr.size())) {
            return std::nullopt;
        }
        auto item = arr[index];
        arr.erase(arr.begin() + index);
        return std::make_optional(item);
    }
}

pub fn clear(arr: Array<any>) {
    @cpp { arr.clear(); }
}

pub fn resize(arr: Array<any>, newSize: int) {
    @cpp { arr.resize(newSize); }
}

pub fn reserve(arr: Array<any>, capacity: int) {
    @cpp { arr.reserve(capacity); }
}

// ============================================================================
// Search
// ============================================================================

pub fn contains(arr: Array<any>, item: any) -> bool {
    @cpp {
        return std::find(arr.begin(), arr.end(), item) != arr.end();
    }
}

pub fn indexOf(arr: Array<any>, item: any) -> Option<int> {
    @cpp {
        auto it = std::find(arr.begin(), arr.end(), item);
        if (it != arr.end()) {
            return std::make_optional(static_cast<int>(std::distance(arr.begin(), it)));
        }
        return std::nullopt;
    }
}

pub fn lastIndexOf(arr: Array<any>, item: any) -> Option<int> {
    @cpp {
        for (int i = arr.size() - 1; i >= 0; i--) {
            if (arr[i] == item) {
                return std::make_optional(i);
            }
        }
        return std::nullopt;
    }
}

pub fn count(arr: Array<any>, item: any) -> int {
    @cpp {
        return std::count(arr.begin(), arr.end(), item);
    }
}

// ============================================================================
// Ordering
// ============================================================================

pub fn reverse(arr: Array<any>) {
    @cpp { std::reverse(arr.begin(), arr.end()); }
}

pub fn sort(arr: Array<any>) {
    @cpp { std::sort(arr.begin(), arr.end()); }
}

pub fn sortDesc(arr: Array<any>) {
    @cpp { std::sort(arr.begin(), arr.end(), std::greater<>()); }
}

pub fn shuffle(arr: Array<any>) {
    @cpp {
        static std::random_device rd;
        static std::mt19937 g(rd());
        std::shuffle(arr.begin(), arr.end(), g);
    }
}

// ============================================================================
// Transformation
// ============================================================================

pub fn slice(arr: Array<any>, start: int, endIdx: int) -> Array<any> {
    @cpp {
        if (start < 0) start = 0;
        if (endIdx > static_cast<int>(arr.size())) endIdx = arr.size();
        return std::vector<decltype(arr)::value_type>(arr.begin() + start, arr.begin() + endIdx);
    }
}

pub fn concat(a: Array<any>, b: Array<any>) -> Array<any> {
    @cpp {
        auto result = a;
        result.insert(result.end(), b.begin(), b.end());
        return result;
    }
}

pub fn flatten(arr: Array<Array<any>>) -> Array<any> {
    @cpp {
        std::vector<typename decltype(arr)::value_type::value_type> result;
        for (const auto& inner : arr) {
            result.insert(result.end(), inner.begin(), inner.end());
        }
        return result;
    }
}

pub fn dedupe(arr: Array<any>) -> Array<any> {
    @cpp {
        auto result = arr;
        std::sort(result.begin(), result.end());
        result.erase(std::unique(result.begin(), result.end()), result.end());
        return result;
    }
}

// ============================================================================
// Higher-order functions
// ============================================================================

pub fn map(arr: Array<any>, f: fn(any) -> any) -> Array<any> {
    @cpp {
        std::vector<decltype(f(arr[0]))> result;
        result.reserve(arr.size());
        for (const auto& item : arr) {
            result.push_back(f(item));
        }
        return result;
    }
}

pub fn filter(arr: Array<any>, predicate: fn(any) -> bool) -> Array<any> {
    @cpp {
        std::vector<decltype(arr)::value_type> result;
        for (const auto& item : arr) {
            if (predicate(item)) {
                result.push_back(item);
            }
        }
        return result;
    }
}

pub fn reduce(arr: Array<any>, initial: any, f: fn(any, any) -> any) -> any {
    @cpp {
        auto acc = initial;
        for (const auto& item : arr) {
            acc = f(acc, item);
        }
        return acc;
    }
}

pub fn forEach(arr: Array<any>, f: fn(any)) {
    for (item in arr) {
        f(item);
    }
}

pub fn find(arr: Array<any>, predicate: fn(any) -> bool) -> Option<any> {
    @cpp {
        for (const auto& item : arr) {
            if (predicate(item)) {
                return std::make_optional(item);
            }
        }
        return std::nullopt;
    }
}

pub fn findIndex(arr: Array<any>, predicate: fn(any) -> bool) -> Option<int> {
    @cpp {
        for (size_t i = 0; i < arr.size(); i++) {
            if (predicate(arr[i])) {
                return std::make_optional(static_cast<int>(i));
            }
        }
        return std::nullopt;
    }
}

pub fn any(arr: Array<any>, predicate: fn(any) -> bool) -> bool {
    @cpp {
        return std::any_of(arr.begin(), arr.end(), predicate);
    }
}

pub fn all(arr: Array<any>, predicate: fn(any) -> bool) -> bool {
    @cpp {
        return std::all_of(arr.begin(), arr.end(), predicate);
    }
}

pub fn none(arr: Array<any>, predicate: fn(any) -> bool) -> bool {
    @cpp {
        return std::none_of(arr.begin(), arr.end(), predicate);
    }
}

// ============================================================================
// Aggregation
// ============================================================================

pub fn sum(arr: Array<int>) -> int {
    @cpp {
        return std::accumulate(arr.begin(), arr.end(), 0);
    }
}

pub fn sumf(arr: Array<float>) -> float {
    @cpp {
        return std::accumulate(arr.begin(), arr.end(), 0.0);
    }
}

pub fn minVal(arr: Array<int>) -> Option<int> {
    @cpp {
        if (arr.empty()) return std::nullopt;
        return std::make_optional(*std::min_element(arr.begin(), arr.end()));
    }
}

pub fn maxVal(arr: Array<int>) -> Option<int> {
    @cpp {
        if (arr.empty()) return std::nullopt;
        return std::make_optional(*std::max_element(arr.begin(), arr.end()));
    }
}

// ============================================================================
// Creation helpers
// ============================================================================

pub fn range(start: int, endVal: int) -> Array<int> {
    @cpp {
        std::vector<int> result;
        for (int i = start; i < endVal; i++) {
            result.push_back(i);
        }
        return result;
    }
}

pub fn rangeStep(start: int, endVal: int, step: int) -> Array<int> {
    @cpp {
        std::vector<int> result;
        for (int i = start; i < endVal; i += step) {
            result.push_back(i);
        }
        return result;
    }
}

pub fn filled(size: int, value: any) -> Array<any> {
    @cpp {
        return std::vector<decltype(value)>(size, value);
    }
}

pub fn zeros(size: int) -> Array<int> {
    @cpp { return std::vector<int>(size, 0); }
}

pub fn ones(size: int) -> Array<int> {
    @cpp { return std::vector<int>(size, 1); }
}

// ============================================================================
// Zip operations
// ============================================================================

pub fn zip(a: Array<any>, b: Array<any>) -> Array<Array<any>> {
    @cpp {
        std::vector<std::vector<decltype(a)::value_type>> result;
        size_t len = std::min(a.size(), b.size());
        for (size_t i = 0; i < len; i++) {
            result.push_back({a[i], b[i]});
        }
        return result;
    }
}

pub fn enumerate(arr: Array<any>) -> Array<Array<any>> {
    @cpp {
        std::vector<std::pair<int, decltype(arr)::value_type>> result;
        for (size_t i = 0; i < arr.size(); i++) {
            result.push_back({static_cast<int>(i), arr[i]});
        }
        return result;
    }
}
