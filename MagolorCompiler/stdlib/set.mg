// Std.Set - HashSet operations
// Unique element collection with O(1) average access

using Std.Core.Prelude;

// ============================================================================
// Basic operations
// ============================================================================

pub fn size(set: Set<any>) -> int {
    @cpp { return set.size(); }
}

pub fn isEmpty(set: Set<any>) -> bool {
    @cpp { return set.empty(); }
}

pub fn clear(set: Set<any>) {
    @cpp { set.clear(); }
}

// ============================================================================
// Element operations
// ============================================================================

pub fn contains(set: Set<any>, item: any) -> bool {
    @cpp { return set.find(item) != set.end(); }
}

pub fn insert(set: Set<any>, item: any) -> bool {
    @cpp {
        auto result = set.insert(item);
        return result.second;  // true if inserted, false if already exists
    }
}

pub fn remove(set: Set<any>, item: any) -> bool {
    @cpp {
        return set.erase(item) > 0;
    }
}

// ============================================================================
// Set operations
// ============================================================================

pub fn union_(a: Set<any>, b: Set<any>) -> Set<any> {
    @cpp {
        decltype(a) result = a;
        for (const auto& item : b) {
            result.insert(item);
        }
        return result;
    }
}

pub fn intersection(a: Set<any>, b: Set<any>) -> Set<any> {
    @cpp {
        decltype(a) result;
        for (const auto& item : a) {
            if (b.find(item) != b.end()) {
                result.insert(item);
            }
        }
        return result;
    }
}

pub fn difference(a: Set<any>, b: Set<any>) -> Set<any> {
    @cpp {
        decltype(a) result;
        for (const auto& item : a) {
            if (b.find(item) == b.end()) {
                result.insert(item);
            }
        }
        return result;
    }
}

pub fn symmetricDifference(a: Set<any>, b: Set<any>) -> Set<any> {
    @cpp {
        decltype(a) result;
        for (const auto& item : a) {
            if (b.find(item) == b.end()) {
                result.insert(item);
            }
        }
        for (const auto& item : b) {
            if (a.find(item) == a.end()) {
                result.insert(item);
            }
        }
        return result;
    }
}

// ============================================================================
// Set predicates
// ============================================================================

pub fn isSubset(a: Set<any>, b: Set<any>) -> bool {
    @cpp {
        for (const auto& item : a) {
            if (b.find(item) == b.end()) {
                return false;
            }
        }
        return true;
    }
}

pub fn isSuperset(a: Set<any>, b: Set<any>) -> bool {
    return isSubset(b, a);
}

pub fn isDisjoint(a: Set<any>, b: Set<any>) -> bool {
    @cpp {
        for (const auto& item : a) {
            if (b.find(item) != b.end()) {
                return false;
            }
        }
        return true;
    }
}

pub fn equals(a: Set<any>, b: Set<any>) -> bool {
    if (size(a) != size(b)) { return false; }
    return isSubset(a, b);
}

// ============================================================================
// Conversion
// ============================================================================

pub fn toArray(set: Set<any>) -> Array<any> {
    @cpp {
        return std::vector<decltype(set)::value_type>(set.begin(), set.end());
    }
}

pub fn fromArray(arr: Array<any>) -> Set<any> {
    @cpp {
        return std::unordered_set<decltype(arr)::value_type>(arr.begin(), arr.end());
    }
}

// ============================================================================
// Iteration
// ============================================================================

pub fn forEach(set: Set<any>, f: fn(any)) {
    @cpp {
        for (const auto& item : set) {
            f(item);
        }
    }
}

pub fn map(set: Set<any>, f: fn(any) -> any) -> Set<any> {
    @cpp {
        std::unordered_set<decltype(f(*set.begin()))> result;
        for (const auto& item : set) {
            result.insert(f(item));
        }
        return result;
    }
}

pub fn filter(set: Set<any>, predicate: fn(any) -> bool) -> Set<any> {
    @cpp {
        decltype(set) result;
        for (const auto& item : set) {
            if (predicate(item)) {
                result.insert(item);
            }
        }
        return result;
    }
}

pub fn any(set: Set<any>, predicate: fn(any) -> bool) -> bool {
    @cpp {
        for (const auto& item : set) {
            if (predicate(item)) {
                return true;
            }
        }
        return false;
    }
}

pub fn all(set: Set<any>, predicate: fn(any) -> bool) -> bool {
    @cpp {
        for (const auto& item : set) {
            if (!predicate(item)) {
                return false;
            }
        }
        return true;
    }
}

// ============================================================================
// Aggregation
// ============================================================================

pub fn reduce(set: Set<any>, initial: any, f: fn(any, any) -> any) -> any {
    @cpp {
        auto acc = initial;
        for (const auto& item : set) {
            acc = f(acc, item);
        }
        return acc;
    }
}
