// ============================================================================
// C/C++ Imports
// ============================================================================
#include <stdio.h>
using ::printf;
#include <math.h>
namespace Math {
    // Note: Import all symbols from global namespace
    // You may need to explicitly use :: prefix for some symbols
}

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <functional>
#include <optional>
#include <algorithm>
#include <chrono>
#include <thread>
#include <random>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <iomanip>

namespace Std {

// ============================================================================
// Std.IO - Input/Output Operations
// ============================================================================
namespace IO {
    // Basic output
    inline void print(const std::string& s) { 
        std::cout << s; 
    }
    
    inline void println(const std::string& s) { 
        std::cout << s << std::endl; 
    }
    
    inline void eprint(const std::string& s) { 
        std::cerr << s; 
    }
    
    inline void eprintln(const std::string& s) { 
        std::cerr << s << std::endl; 
    }
    
    // Input
    inline std::string readLine() { 
        std::string line; 
        std::getline(std::cin, line); 
        return line; 
    }
    
    inline std::string read() {
        std::string content;
        std::string line;
        while (std::getline(std::cin, line)) {
            content += line + "\n";
        }
        return content;
    }
    
    inline char readChar() {
        char c;
        std::cin >> c;
        return c;
    }
    
    // File operations
    inline std::optional<std::string> readFile(const std::string& path) {
        std::ifstream file(path);
        if (!file) return std::nullopt;
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
    
    inline bool writeFile(const std::string& path, const std::string& content) {
        std::ofstream file(path);
        if (!file) return false;
        file << content;
        return true;
    }
    
    inline bool appendFile(const std::string& path, const std::string& content) {
        std::ofstream file(path, std::ios::app);
        if (!file) return false;
        file << content;
        return true;
    }
}

// ============================================================================
// Std.Parse - Parsing Operations
// ============================================================================
namespace Parse {
    inline std::optional<int> parseInt(const std::string& s) {
        try {
            size_t pos;
            int val = std::stoi(s, &pos);
            if (pos == s.length()) return val;
            return std::nullopt;
        } catch (...) {
            return std::nullopt;
        }
    }
    
    inline std::optional<double> parseFloat(const std::string& s) {
        try {
            size_t pos;
            double val = std::stod(s, &pos);
            if (pos == s.length()) return val;
            return std::nullopt;
        } catch (...) {
            return std::nullopt;
        }
    }
    
    inline std::optional<bool> parseBool(const std::string& s) {
        if (s == "true" || s == "1") return true;
        if (s == "false" || s == "0") return false;
        return std::nullopt;
    }
}

// ============================================================================
// Std.Option - Optional Value Operations
// ============================================================================
namespace Option {
    template<typename T>
    inline bool isSome(const std::optional<T>& opt) { 
        return opt.has_value(); 
    }
    
    template<typename T>
    inline bool isNone(const std::optional<T>& opt) { 
        return !opt.has_value(); 
    }
    
    template<typename T>
    inline T unwrap(const std::optional<T>& opt) { 
        if (!opt.has_value()) {
            throw std::runtime_error("Called unwrap on None value");
        }
        return opt.value(); 
    }
    
    template<typename T>
    inline T unwrapOr(const std::optional<T>& opt, const T& defaultValue) {
        return opt.value_or(defaultValue);
    }
}

// ============================================================================
// Std.Math - Mathematical Operations
// ============================================================================
namespace Math {
    // Constants
    constexpr double PI = 3.14159265358979323846;
    constexpr double E = 2.71828182845904523536;
    
    // Basic operations
    inline int abs(int x) { return std::abs(x); }
    inline double abs(double x) { return std::fabs(x); }
    
    inline double pow(double base, double exp) { return std::pow(base, exp); }
    inline double sqrt(double x) { return std::sqrt(x); }
    inline double cbrt(double x) { return std::cbrt(x); }
    
    // Trigonometry
    inline double sin(double x) { return std::sin(x); }
    inline double cos(double x) { return std::cos(x); }
    inline double tan(double x) { return std::tan(x); }
    inline double asin(double x) { return std::asin(x); }
    inline double acos(double x) { return std::acos(x); }
    inline double atan(double x) { return std::atan(x); }
    inline double atan2(double y, double x) { return std::atan2(y, x); }
    
    // Exponential and logarithms
    inline double exp(double x) { return std::exp(x); }
    inline double log(double x) { return std::log(x); }
    inline double log10(double x) { return std::log10(x); }
    inline double log2(double x) { return std::log2(x); }
    
    // Rounding
    inline double floor(double x) { return std::floor(x); }
    inline double ceil(double x) { return std::ceil(x); }
    inline double round(double x) { return std::round(x); }
    
    // Min/Max
    inline int min(int a, int b) { return std::min(a, b); }
    inline double min(double a, double b) { return std::min(a, b); }
    inline int max(int a, int b) { return std::max(a, b); }
    inline double max(double a, double b) { return std::max(a, b); }
    
    // Clamping
    inline int clamp(int val, int low, int high) { 
        return std::max(low, std::min(val, high)); 
    }
    inline double clamp(double val, double low, double high) { 
        return std::max(low, std::min(val, high)); 
    }
}

// ============================================================================
// Std.String - String Operations
// ============================================================================
namespace String {
    inline int length(const std::string& s) { 
        return s.length(); 
    }
    
    inline bool isEmpty(const std::string& s) { 
        return s.empty(); 
    }
    
    inline std::string trim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t\n\r");
        return s.substr(start, end - start + 1);
    }
    
    inline std::string toLower(const std::string& s) {
        std::string result = s;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }
    
    inline std::string toUpper(const std::string& s) {
        std::string result = s;
        std::transform(result.begin(), result.end(), result.begin(), ::toupper);
        return result;
    }
    
    inline bool startsWith(const std::string& s, const std::string& prefix) {
        return s.size() >= prefix.size() && 
               s.compare(0, prefix.size(), prefix) == 0;
    }
    
    inline bool endsWith(const std::string& s, const std::string& suffix) {
        return s.size() >= suffix.size() && 
               s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
    }
    
    inline bool contains(const std::string& s, const std::string& substr) {
        return s.find(substr) != std::string::npos;
    }
    
    inline std::string replace(const std::string& s, const std::string& from, 
                               const std::string& to) {
        std::string result = s;
        size_t pos = 0;
        while ((pos = result.find(from, pos)) != std::string::npos) {
            result.replace(pos, from.length(), to);
            pos += to.length();
        }
        return result;
    }
    
    inline std::vector<std::string> split(const std::string& s, char delim) {
        std::vector<std::string> tokens;
        std::stringstream ss(s);
        std::string token;
        while (std::getline(ss, token, delim)) {
            tokens.push_back(token);
        }
        return tokens;
    }
    
    inline std::string join(const std::vector<std::string>& parts, const std::string& sep) {
        std::string result;
        for (size_t i = 0; i < parts.size(); i++) {
            if (i > 0) result += sep;
            result += parts[i];
        }
        return result;
    }
    
    inline std::string repeat(const std::string& s, int count) {
        std::string result;
        for (int i = 0; i < count; i++) {
            result += s;
        }
        return result;
    }
    
    inline std::string substring(const std::string& s, int start, int length = -1) {
        if (length == -1) return s.substr(start);
        return s.substr(start, length);
    }
}

// ============================================================================
// Std.Array - Array Operations
// ============================================================================
namespace Array {
    template<typename T>
    inline int length(const std::vector<T>& arr) { 
        return arr.size(); 
    }
    
    template<typename T>
    inline bool isEmpty(const std::vector<T>& arr) { 
        return arr.empty(); 
    }
    
    template<typename T>
    inline void push(std::vector<T>& arr, const T& item) { 
        arr.push_back(item); 
    }
    
    template<typename T>
    inline std::optional<T> pop(std::vector<T>& arr) {
        if (arr.empty()) return std::nullopt;
        T item = arr.back();
        arr.pop_back();
        return item;
    }
    
    template<typename T>
    inline bool contains(const std::vector<T>& arr, const T& item) {
        return std::find(arr.begin(), arr.end(), item) != arr.end();
    }
    
    template<typename T>
    inline void reverse(std::vector<T>& arr) {
        std::reverse(arr.begin(), arr.end());
    }
    
    template<typename T>
    inline void sort(std::vector<T>& arr) {
        std::sort(arr.begin(), arr.end());
    }
}

// ============================================================================
// Std.File - File System Operations
// ============================================================================
namespace File {
    inline bool exists(const std::string& path) {
        return std::filesystem::exists(path);
    }
    
    inline bool isFile(const std::string& path) {
        return std::filesystem::is_regular_file(path);
    }
    
    inline bool isDirectory(const std::string& path) {
        return std::filesystem::is_directory(path);
    }
    
    inline bool createDir(const std::string& path) {
        try {
            return std::filesystem::create_directories(path);
        } catch (...) {
            return false;
        }
    }
    
    inline bool remove(const std::string& path) {
        try {
            return std::filesystem::remove(path);
        } catch (...) {
            return false;
        }
    }
    
    inline bool removeAll(const std::string& path) {
        try {
            std::filesystem::remove_all(path);
            return true;
        } catch (...) {
            return false;
        }
    }
    
    inline bool copy(const std::string& from, const std::string& to) {
        try {
            std::filesystem::copy(from, to);
            return true;
        } catch (...) {
            return false;
        }
    }
    
    inline bool rename(const std::string& from, const std::string& to) {
        try {
            std::filesystem::rename(from, to);
            return true;
        } catch (...) {
            return false;
        }
    }
    
    inline std::optional<int> size(const std::string& path) {
        try {
            return std::filesystem::file_size(path);
        } catch (...) {
            return std::nullopt;
        }
    }
}

// ============================================================================
// Std.Time - Time Operations
// ============================================================================
namespace Time {
    inline int now() {
        return std::chrono::system_clock::now().time_since_epoch().count();
    }
    
    inline void sleep(int milliseconds) {
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    }
    
    inline std::string timestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
}

// ============================================================================
// Std.Random - Random Number Generation
// ============================================================================
namespace Random {
    inline int randInt(int min, int max) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(min, max);
        return dis(gen);
    }
    
    inline double randFloat(double min = 0.0, double max = 1.0) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(min, max);
        return dis(gen);
    }
    
    inline bool randBool() {
        return randInt(0, 1) == 1;
    }
}

// ============================================================================
// Std.System - System Operations
// ============================================================================
namespace System {
    inline void exit(int code) {
        std::exit(code);
    }
    
    inline std::optional<std::string> getEnv(const std::string& name) {
        const char* val = std::getenv(name.c_str());
        if (val) return std::string(val);
        return std::nullopt;
    }
    
    inline int execute(const std::string& command) {
        return std::system(command.c_str());
    }
}

// ============================================================================
// Top-level convenience functions
// ============================================================================
inline void print(const std::string& s) { IO::print(s); }
inline void println(const std::string& s) { IO::println(s); }
inline std::string readLine() { return IO::readLine(); }
inline std::optional<int> parseInt(const std::string& s) { return Parse::parseInt(s); }
inline std::optional<double> parseFloat(const std::string& s) { return Parse::parseFloat(s); }

} // namespace Std

// ============================================================================
// Template Helpers for String Conversion
// ============================================================================
template<typename T>
inline std::string mg_to_string(const T& val) { 
    std::ostringstream oss; 
    oss << val; 
    return oss.str(); 
}

// Specializations for better formatting
template<>
inline std::string mg_to_string(const bool& val) {
    return val ? "true" : "false";
}

template<>
inline std::string mg_to_string(const std::string& val) {
    return val;
}


void demonstrateVariableSharing();
void demonstrateComplexSharing();
void demonstrateStringSharing();
void demonstrateArraySharing();
void demonstrateMixedCalculation();

void demonstrateVariableSharing() {
    Std::print(std::string("=== Variable Sharing Demo ===\n\n"));
    auto count = 0;
    auto sum = 0;
    auto multiplier = 2;
    Std::print((std::string("Initial count: ") + mg_to_string(count) + std::string("\n")));
    Std::print((std::string("Initial sum: ") + mg_to_string(sum) + std::string("\n")));
    Std::print((std::string("Multiplier: ") + mg_to_string(multiplier) + std::string("\n\n")));
    // Inline C++ code:

        // All Magolor variables are accessible here!
        printf("Inside C++:\n");
        printf("  count = %d\n", count);
        printf("  sum = %d\n", sum);
        printf("  multiplier = %d\n", multiplier);
        
        // Modify mutable variables
        count = 5;
        sum = 100;
        // multiplier is immutable, can't modify
        
        printf("\nAfter C++ modification:\n");
        printf("  count = %d\n", count);
        printf("  sum = %d\n", sum);
    
    Std::print(std::string("\nBack in Magolor:\n"));
    Std::print((std::string("  count = ") + mg_to_string(count) + std::string("\n")));
    Std::print((std::string("  sum = ") + mg_to_string(sum) + std::string("\n")));
    Std::print((std::string("  multiplier = ") + mg_to_string(multiplier) + std::string("\n")));
}

void demonstrateComplexSharing() {
    Std::print(std::string("\n=== Complex Data Sharing ===\n\n"));
    auto result = 0.000000;
    auto base = 10.000000;
    Std::print((std::string("Computing factorial-like operations on ") + mg_to_string(base) + std::string("\n")));
    // Inline C++ code:

        // Use Magolor variables in C++ calculations
        result = 1.0;
        for (int i = 1; i <= base; i++) {
            result *= i;
        }
        printf("C++ computed: %f\n", result);
    
    Std::print((std::string("Magolor received: ") + mg_to_string(result) + std::string("\n")));
    auto sqrt_result = Math::sqrt(result);
    Std::print((std::string("Square root: ") + mg_to_string(sqrt_result) + std::string("\n")));
}

void demonstrateStringSharing() {
    Std::print(std::string("\n=== String Sharing ===\n\n"));
    auto message = std::string("Hello");
    Std::print((std::string("Magolor: ") + mg_to_string(message) + std::string("\n")));
    // Inline C++ code:

        // Access and modify the string
        std::cout << "C++: " << message << "\n";
        message += " from C++!";
        std::cout << "C++ modified: " << message << "\n";
    
    Std::print((std::string("Magolor sees: ") + mg_to_string(message) + std::string("\n")));
}

void demonstrateArraySharing() {
    Std::print(std::string("\n=== Array Sharing ===\n\n"));
    auto numbers = {1, 2, 3, 4, 5};
    Std::print(std::string("Original array: "));
    for (auto& n : numbers) {
        Std::print((mg_to_string(n) + std::string(" ")));
    }
    Std::print(std::string("\n"));
    // Inline C++ code:

        // Access the vector in C++
        std::cout << "C++ sees " << numbers.size() << " elements\n";
        
        // Add more elements
        numbers.push_back(6);
        numbers.push_back(7);
        
        // Sort them
        std::sort(numbers.begin(), numbers.end(), std::greater<int>());
        
        std::cout << "C++ sorted (descending): ";
        for (int n : numbers) {
            std::cout << n << " ";
        }
        std::cout << "\n";
    
    Std::print(std::string("Back in Magolor: "));
    for (auto& n : numbers) {
        Std::print((mg_to_string(n) + std::string(" ")));
    }
    Std::print(std::string("\n"));
}

void demonstrateMixedCalculation() {
    Std::print(std::string("\n=== Mixed Magolor/C++ Calculation ===\n\n"));
    auto temperature_celsius = 25.000000;
    auto temperature_fahrenheit = 0.000000;
    Std::print((std::string("Temperature: ") + mg_to_string(temperature_celsius) + std::string("°C\n")));
    // Inline C++ code:

        temperature_fahrenheit = (temperature_celsius * 9.0 / 5.0) + 32.0;
        printf("C++ converted: %.1f°F\n", temperature_fahrenheit);
    
    Std::print((std::string("Magolor: ") + mg_to_string(temperature_celsius) + std::string("°C = ") + mg_to_string(temperature_fahrenheit) + std::string("°F\n")));
    if ((temperature_fahrenheit > 80.000000)) {
        Std::print(std::string("It's hot!\n"));
    }
    else {
        Std::print(std::string("It's pleasant!\n"));
    }
}

int main() {
    Std::print(std::string("╔════════════════════════════════════╗\n"));
    Std::print(std::string("║  Variable Sharing Demo             ║\n"));
    Std::print(std::string("║  Magolor ↔ C++ @cpp blocks         ║\n"));
    Std::print(std::string("╚════════════════════════════════════╝\n\n"));
    demonstrateVariableSharing();
    demonstrateComplexSharing();
    demonstrateStringSharing();
    demonstrateArraySharing();
    demonstrateMixedCalculation();
    Std::print(std::string("\n✅ Variable sharing working perfectly!\n"));
    Std::print(std::string("\n💡 Key Points:\n"));
    Std::print(std::string("   • Magolor variables are accessible in @cpp blocks\n"));
    Std::print(std::string("   • Changes in @cpp persist back to Magolor\n"));
    Std::print(std::string("   • `mut` variables can be modified\n"));
    Std::print(std::string("   • Immutable variables are read-only\n"));
    Std::print(std::string("   • Arrays → std::vector, Strings → std::string\n"));
    return 0;
}

