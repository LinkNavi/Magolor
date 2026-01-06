// Std.Random - Random number generation
// Cryptographically-aware random utilities

using Std.Core.Prelude;

// ============================================================================
// Basic random numbers
// ============================================================================

// Random integer in range [min, max]
pub fn randInt(min: int, max: int) -> int {
    @cpp {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(min, max);
        return dis(gen);
    }
}

// Random float in range [0.0, 1.0)
pub fn rand() -> float {
    @cpp {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0.0, 1.0);
        return dis(gen);
    }
}

// Random float in range [min, max)
pub fn randFloat(min: float, max: float) -> float {
    @cpp {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(min, max);
        return dis(gen);
    }
}

// Random boolean
pub fn randBool() -> bool {
    return randInt(0, 1) == 1;
}

// Random boolean with probability
pub fn randBoolProb(probability: float) -> bool {
    return rand() < probability;
}

// ============================================================================
// Seeded random (reproducible)
// ============================================================================

pub class Rng {
    seed: int;
    
    pub fn create(s: int) {
        this.seed = s;
    }
    
    pub fn nextInt(min: int, max: int) -> int {
        @cpp {
            static std::mt19937 gen(this->seed);
            std::uniform_int_distribution<> dis(min, max);
            return dis(gen);
        }
    }
    
    pub fn nextFloat() -> float {
        @cpp {
            static std::mt19937 gen(this->seed);
            std::uniform_real_distribution<> dis(0.0, 1.0);
            return dis(gen);
        }
    }
}

pub fn newRng(seed: int) -> Rng {
    let mut rng = new Rng(seed);
    return rng;
}

// ============================================================================
// Random selection
// ============================================================================

// Pick random element from array
pub fn choice(arr: Array<any>) -> Option<any> {
    if (isEmpty(arr)) { return None; }
    let idx = randInt(0, length(arr) - 1);
    return Some(arr[idx]);
}

// Pick n random elements (with replacement)
pub fn choices(arr: Array<any>, n: int) -> Array<any> {
    @cpp {
        std::vector<decltype(arr)::value_type> result;
        result.reserve(n);
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, arr.size() - 1);
        for (int i = 0; i < n; i++) {
            result.push_back(arr[dis(gen)]);
        }
        return result;
    }
}

// Pick n random elements (without replacement)
pub fn sample(arr: Array<any>, n: int) -> Array<any> {
    @cpp {
        auto copy = arr;
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::shuffle(copy.begin(), copy.end(), gen);
        if (n > static_cast<int>(copy.size())) {
            n = copy.size();
        }
        return std::vector<decltype(arr)::value_type>(copy.begin(), copy.begin() + n);
    }
}

// Shuffle array in place
pub fn shuffle(arr: Array<any>) {
    @cpp {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::shuffle(arr.begin(), arr.end(), gen);
    }
}

// Return shuffled copy
pub fn shuffled(arr: Array<any>) -> Array<any> {
    @cpp {
        auto copy = arr;
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::shuffle(copy.begin(), copy.end(), gen);
        return copy;
    }
}

// ============================================================================
// Weighted random
// ============================================================================

// Pick element with weights
pub fn weightedChoice(items: Array<any>, weights: Array<float>) -> Option<any> {
    @cpp {
        if (items.empty() || weights.empty()) return std::nullopt;
        
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::discrete_distribution<> dis(weights.begin(), weights.end());
        
        int idx = dis(gen);
        return std::make_optional(items[idx]);
    }
}

// ============================================================================
// Distribution-based random
// ============================================================================

// Normal (Gaussian) distribution
pub fn randNormal(mean: float, stddev: float) -> float {
    @cpp {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::normal_distribution<> dis(mean, stddev);
        return dis(gen);
    }
}

// Exponential distribution
pub fn randExponential(lambda: float) -> float {
    @cpp {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::exponential_distribution<> dis(lambda);
        return dis(gen);
    }
}

// Poisson distribution
pub fn randPoisson(mean: float) -> int {
    @cpp {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::poisson_distribution<> dis(mean);
        return dis(gen);
    }
}

// ============================================================================
// Random string generation
// ============================================================================

pub fn randString(length: int) -> string {
    let chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    let mut result = "";
    for (i in 0..length) {
        let idx = randInt(0, 61);
        result = result + charAt(chars, idx);
    }
    return result;
}

pub fn randAlpha(length: int) -> string {
    let chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    let mut result = "";
    for (i in 0..length) {
        let idx = randInt(0, 51);
        result = result + charAt(chars, idx);
    }
    return result;
}

pub fn randDigits(length: int) -> string {
    let mut result = "";
    for (i in 0..length) {
        result = result + toString(randInt(0, 9));
    }
    return result;
}

pub fn randHex(length: int) -> string {
    let chars = "0123456789abcdef";
    let mut result = "";
    for (i in 0..length) {
        let idx = randInt(0, 15);
        result = result + charAt(chars, idx);
    }
    return result;
}

// ============================================================================
// UUID generation
// ============================================================================

pub fn uuid4() -> string {
    // Generate UUID v4 (random)
    let hex = randHex(32);
    return substring(hex, 0, 8) + "-" +
           substring(hex, 8, 4) + "-" +
           "4" + substring(hex, 13, 3) + "-" +
           substring(hex, 16, 4) + "-" +
           substring(hex, 20, 12);
}

// ============================================================================
// Cryptographically secure random
// ============================================================================

pub fn secureRandBytes(length: int) -> Array<int> {
    @cpp {
        std::vector<int> bytes(length);
        std::random_device rd;
        for (int i = 0; i < length; i++) {
            bytes[i] = rd() % 256;
        }
        return bytes;
    }
}

pub fn secureRandHex(length: int) -> string {
    let bytes = secureRandBytes(length);
    let mut result = "";
    let hexChars = "0123456789abcdef";
    for (b in bytes) {
        result = result + charAt(hexChars, b / 16) + charAt(hexChars, b % 16);
    }
    return result;
}
