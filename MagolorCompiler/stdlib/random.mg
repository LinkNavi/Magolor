// Std.Random - Random Number Generation Module
// Provides various random number generation functions and utilities

// ============================================================================
// Basic Random Generation
// ============================================================================

// Generate random integer between 0 and max (exclusive)
pub fn randomInt(max: Int) -> Int {
    // Implementation provided by LLVM runtime
    return 0;
}

// Generate random integer between min and max (inclusive)
pub fn randomRange(min: Int, max: Int) -> Int {
    return min + randomInt(max - min + 1);
}

// Generate random float between 0.0 and 1.0
pub fn random() -> Float {
    // Implementation provided by LLVM runtime
    return 0.0;
}

// Generate random float between min and max
pub fn randomFloat(min: Float, max: Float) -> Float {
    return min + (random() * (max - min));
}

// Generate random boolean (50/50 chance)
pub fn randomBool() -> Bool {
    return randomInt(2) == 1;
}

// Generate random boolean with custom probability (0.0 to 1.0)
pub fn randomBoolWithProbability(probability: Float) -> Bool {
    return random() < probability;
}

// ============================================================================
// Seeded Random Generation
// ============================================================================

// Set random seed for reproducible results
pub fn setSeed(seed: Int) {
    // Implementation provided by LLVM runtime
}

// Generate random number with specific seed
pub fn randomWithSeed(seed: Int, max: Int) -> Int {
    setSeed(seed);
    return randomInt(max);
}

// ============================================================================
// Random Array Operations
// ============================================================================

// Get random element from array
pub fn randomChoice(arr: Array<Int>) -> Int {
    if isEmpty(arr) {
        return 0;
    }
    return arr[randomInt(length(arr))];
}

// Get n random elements from array (with replacement)
pub fn randomChoices(arr: Array<Int>, count: Int) -> Array<Int> {
    let mut result = [];
    let mut i = 0;
    while i < count {
        push(result, randomChoice(arr));
        i = i + 1;
    }
    return result;
}

// Get n unique random elements from array (without replacement)
pub fn randomSample(arr: Array<Int>, count: Int) -> Array<Int> {
    if count >= length(arr) {
        return shuffle(arr);
    }
    
    let mut result = [];
    let mut available = clone(arr);
    let mut i = 0;
    while i < count {
        let index = randomInt(length(available));
        push(result, available[index]);
        remove(available, index);
        i = i + 1;
    }
    return result;
}

// Shuffle array randomly (in-place)
pub fn shuffle(arr: Array<Int>) {
    let n = length(arr);
    let mut i = n - 1;
    while i > 0 {
        let j = randomInt(i + 1);
        // Swap arr[i] and arr[j]
        let temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
        i = i - 1;
    }
}

// Create new shuffled copy of array
pub fn shuffled(arr: Array<Int>) -> Array<Int> {
    let copy = clone(arr);
    shuffle(copy);
    return copy;
}

// ============================================================================
// Random String Generation
// ============================================================================

// Generate random string of given length from charset
pub fn randomString(length: Int, charset: String) -> String {
    let mut result = "";
    let charsetLen = length(charset);
    let mut i = 0;
    while i < length {
        let index = randomInt(charsetLen);
        result = concat(result, charAt(charset, index));
        i = i + 1;
    }
    return result;
}

// Generate random alphanumeric string
pub fn randomAlphanumeric(length: Int) -> String {
    return randomString(length, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
}

// Generate random alphabetic string (lowercase)
pub fn randomAlpha(length: Int) -> String {
    return randomString(length, "abcdefghijklmnopqrstuvwxyz");
}

// Generate random numeric string
pub fn randomDigits(length: Int) -> String {
    return randomString(length, "0123456789");
}

// Generate random hex string
pub fn randomHex(length: Int) -> String {
    return randomString(length, "0123456789abcdef");
}

// Generate random UUID (version 4)
pub fn randomUUID() -> String {
    // Format: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
    let mut uuid = "";
    uuid = concat(uuid, randomHex(8));
    uuid = concat(uuid, "-");
    uuid = concat(uuid, randomHex(4));
    uuid = concat(uuid, "-4");
    uuid = concat(uuid, randomHex(3));
    uuid = concat(uuid, "-");
    let y = randomChoice(["8", "9", "a", "b"]);
    uuid = concat(uuid, y);
    uuid = concat(uuid, randomHex(3));
    uuid = concat(uuid, "-");
    uuid = concat(uuid, randomHex(12));
    return uuid;
}

// ============================================================================
// Weighted Random Selection
// ============================================================================

// Choose from array based on weights
pub fn randomWeighted(choices: Array<Int>, weights: Array<Int>) -> Int {
    if isEmpty(choices) || isEmpty(weights) {
        return 0;
    }
    
    // Calculate total weight
    let totalWeight = sum(weights);
    let rand = randomInt(totalWeight);
    
    let mut cumulative = 0;
    let mut i = 0;
    while i < length(choices) {
        cumulative = cumulative + weights[i];
        if rand < cumulative {
            return choices[i];
        }
        i = i + 1;
    }
    
    return choices[length(choices) - 1];
}

// ============================================================================
// Random Distributions
// ============================================================================

// Generate random number with Gaussian/normal distribution
pub fn randomGaussian(mean: Float, stddev: Float) -> Float {
    // Box-Muller transform
    let u1 = random();
    let u2 = random();
    let z0 = sqrt(-2.0 * log(u1)) * cos(2.0 * PI * u2);
    return mean + (z0 * stddev);
}

// Generate random number with exponential distribution
pub fn randomExponential(lambda: Float) -> Float {
    return -log(random()) / lambda;
}

// Generate random number with Poisson distribution
pub fn randomPoisson(lambda: Float) -> Int {
    let L = exp(-lambda);
    let mut k = 0;
    let mut p = 1.0;
    
    while p > L {
        k = k + 1;
        p = p * random();
    }
    
    return k - 1;
}

// ============================================================================
// Random Geometry
// ============================================================================

// Generate random point in 2D space [0,1] x [0,1]
pub fn randomPoint2D() -> Array<Float> {
    return [random(), random()];
}

// Generate random point in 3D space [0,1] x [0,1] x [0,1]
pub fn randomPoint3D() -> Array<Float> {
    return [random(), random(), random()];
}

// Generate random point in circle with radius r
pub fn randomPointInCircle(radius: Float) -> Array<Float> {
    let angle = randomFloat(0.0, 2.0 * PI);
    let r = sqrt(random()) * radius;
    return [r * cos(angle), r * sin(angle)];
}

// Generate random angle in radians [0, 2π)
pub fn randomAngle() -> Float {
    return randomFloat(0.0, 2.0 * PI);
}

// Generate random angle in degrees [0, 360)
pub fn randomAngleDegrees() -> Float {
    return randomFloat(0.0, 360.0);
}

// ============================================================================
// Random Color Generation
// ============================================================================

// Generate random RGB color (0-255 for each component)
pub fn randomColor() -> Array<Int> {
    return [randomInt(256), randomInt(256), randomInt(256)];
}

// Generate random hex color string (#RRGGBB)
pub fn randomColorHex() -> String {
    return concat("#", randomHex(6));
}

// Generate random grayscale color
pub fn randomGrayscale() -> Int {
    return randomInt(256);
}

// ============================================================================
// Probability Utilities
// ============================================================================

// Coin flip - returns "heads" or "tails"
pub fn coinFlip() -> String {
    if randomBool() {
        return "heads";
    }
    return "tails";
}

// Roll a die with n sides
pub fn rollDie(sides: Int) -> Int {
    return randomRange(1, sides);
}

// Roll multiple dice and return sum
pub fn rollDice(count: Int, sides: Int) -> Int {
    let mut total = 0;
    let mut i = 0;
    while i < count {
        total = total + rollDie(sides);
        i = i + 1;
    }
    return total;
}

// Roll multiple dice and return array of results
pub fn rollDiceArray(count: Int, sides: Int) -> Array<Int> {
    let mut results = [];
    let mut i = 0;
    while i < count {
        push(results, rollDie(sides));
        i = i + 1;
    }
    return results;
}

// Simulate probability event (returns true with given probability)
pub fn probEvent(probability: Float) -> Bool {
    return random() < probability;
}

// Simulate percentage chance (0-100)
pub fn percentChance(percent: Int) -> Bool {
    return randomInt(100) < percent;
}

// ============================================================================
// Random Timing
// ============================================================================

// Generate random delay in milliseconds within range
pub fn randomDelay(minMs: Int, maxMs: Int) -> Int {
    return randomRange(minMs, maxMs);
}

// Sleep for random duration within range
pub fn randomSleep(minMs: Int, maxMs: Int) {
    sleep(randomDelay(minMs, maxMs));
}

// ============================================================================
// Random Testing Utilities
// ============================================================================

// Generate array of random integers for testing
pub fn randomIntArray(size: Int, min: Int, max: Int) -> Array<Int> {
    let mut arr = [];
    let mut i = 0;
    while i < size {
        push(arr, randomRange(min, max));
        i = i + 1;
    }
    return arr;
}

// Generate array of random floats for testing
pub fn randomFloatArray(size: Int, min: Float, max: Float) -> Array<Float> {
    let mut arr = [];
    let mut i = 0;
    while i < size {
        push(arr, randomFloat(min, max));
        i = i + 1;
    }
    return arr;
}

// Generate random boolean array
pub fn randomBoolArray(size: Int) -> Array<Bool> {
    let mut arr = [];
    let mut i = 0;
    while i < size {
        push(arr, randomBool());
        i = i + 1;
    }
    return arr;
}

// ============================================================================
// Random Name/Data Generation
// ============================================================================

// Generate random name from predefined list
pub fn randomName() -> String {
    let names = ["Alice", "Bob", "Charlie", "Diana", "Eve", "Frank", "Grace", "Henry", "Ivy", "Jack"];
    return randomChoice(names);
}

// Generate random email address
pub fn randomEmail() -> String {
    let name = toLowerCase(randomName());
    let domain = randomChoice(["gmail.com", "yahoo.com", "hotmail.com", "example.com"]);
    return concat(concat(concat(name, toString(randomInt(1000))), "@"), domain);
}

// Generate random phone number (format: XXX-XXX-XXXX)
pub fn randomPhone() -> String {
    return concat(concat(concat(concat(concat(randomDigits(3), "-"), randomDigits(3)), "-"), randomDigits(4)), "");
}
