#pragma once
#include <sstream>
#include <string>
#include <unordered_map>
class StdLibGenerator {
public:
  static std::string generateAll() {
    std::stringstream ss;

    ss << generateIncludes();
    ss << "\n";

    ss << "namespace Std {\n\n";

    ss << generateIO();
    ss << generateParse();
    ss << generateOption();
    ss << generateMath();
    ss << generateString(); // This now has indexOf and toString
    ss << generateArray();  // This now has create()
    ss << generateMap();
    ss << generateSet();
    ss << generateFile();
    ss << generateNetwork();
    ss << generateTime();
    ss << generateRandom();
    ss << generateSystem();
ss << generateCrypto();
    ss << generateTopLevel(); // This now has global toString

    ss << "} // namespace Std\n\n";

    ss << generateTemplateHelpers();
    ss << generateGlobalOptionHelpers(); // ADD THIS LINE

    return ss.str();
  }

private:
  static std::string generateIncludes() {
    return R"(#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
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
#include <sys/socket.h>    // ADD THIS LINE
#include <netinet/in.h>    // ADD THIS LINE
#include <arpa/inet.h>     // ADD THIS LINE
#include <unistd.h>        // ADD THIS LINE (if not already present)
)";
  }

  static std::string generateIO() {
    return R"(// ============================================================================
// Std.IO - Input/Output Operations
// ============================================================================
namespace IO {
    inline void print(const std::string& s) { std::cout << s; }
    inline void println(const std::string& s) { std::cout << s << std::endl; }
    inline void eprint(const std::string& s) { std::cerr << s; }
    inline void eprintln(const std::string& s) { std::cerr << s << std::endl; }
    
    inline std::string readLine() { 
        std::string line; 
        std::getline(std::cin, line); 
        return line; 
    }
    
    inline std::string read() {
        std::string content, line;
        while (std::getline(std::cin, line)) content += line + "\n";
        return content;
    }
    
    inline char readChar() { char c; std::cin >> c; return c; }
    
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

)";
  }

  static std::string generateParse() {
    return R"(// ============================================================================
// Std.Parse - Parsing Operations
// ============================================================================
namespace Parse {
    inline std::optional<int> parseInt(const std::string& s) {
        try {
            size_t pos;
            int val = std::stoi(s, &pos);
            if (pos == s.length()) return val;
            return std::nullopt;
        } catch (...) { return std::nullopt; }
    }
    
    inline std::optional<double> parseFloat(const std::string& s) {
        try {
            size_t pos;
            double val = std::stod(s, &pos);
            if (pos == s.length()) return val;
            return std::nullopt;
        } catch (...) { return std::nullopt; }
    }
    
    inline std::optional<bool> parseBool(const std::string& s) {
        if (s == "true" || s == "1") return true;
        if (s == "false" || s == "0") return false;
        return std::nullopt;
    }
}

)";
  }
  static std::string generateOption() {
    return R"(// ============================================================================
// Std.Option - Optional Value Operations
// ============================================================================
namespace Option {
    template<typename T>
    inline bool isSome(const std::optional<T>& opt) { return opt.has_value(); }
    
    template<typename T>
    inline bool isNone(const std::optional<T>& opt) { return !opt.has_value(); }
    
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

)";
  }

  static std::string generateMath() {
    return R"(// ============================================================================
// Std.Math - Mathematical Operations
// ============================================================================
namespace Math {
    constexpr double PI = 3.14159265358979323846;
    constexpr double E = 2.71828182845904523536;
    
    inline int abs(int x) { return std::abs(x); }
    inline double abs(double x) { return std::fabs(x); }
    inline double pow(double base, double exp) { return std::pow(base, exp); }
    inline double sqrt(double x) { return std::sqrt(x); }
    inline double cbrt(double x) { return std::cbrt(x); }
    
    inline double sin(double x) { return std::sin(x); }
    inline double cos(double x) { return std::cos(x); }
    inline double tan(double x) { return std::tan(x); }
    inline double asin(double x) { return std::asin(x); }
    inline double acos(double x) { return std::acos(x); }
    inline double atan(double x) { return std::atan(x); }
    inline double atan2(double y, double x) { return std::atan2(y, x); }
    
    inline double exp(double x) { return std::exp(x); }
    inline double log(double x) { return std::log(x); }
    inline double log10(double x) { return std::log10(x); }
    inline double log2(double x) { return std::log2(x); }
    
    inline double floor(double x) { return std::floor(x); }
    inline double ceil(double x) { return std::ceil(x); }
    inline double round(double x) { return std::round(x); }
    
    inline int min(int a, int b) { return std::min(a, b); }
    inline double min(double a, double b) { return std::min(a, b); }
    inline int max(int a, int b) { return std::max(a, b); }
    inline double max(double a, double b) { return std::max(a, b); }
    
    inline int clamp(int val, int low, int high) { 
        return std::max(low, std::min(val, high)); 
    }
    inline double clamp(double val, double low, double high) { 
        return std::max(low, std::min(val, high)); 
    }
}

)";
  }

  static std::string generateString() {
    return R"(// ============================================================================
// Std.String - String Operations
// ============================================================================
namespace String {
    inline int length(const std::string& s) { return s.length(); }
    inline bool isEmpty(const std::string& s) { return s.empty(); }
    
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
        return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
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
        while (std::getline(ss, token, delim)) tokens.push_back(token);
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
     inline std::optional<int> indexOf(const std::string& s, const std::string& substr) {
        size_t pos = s.find(substr);
        if (pos != std::string::npos) {
            return static_cast<int>(pos);
        }
        return std::nullopt;
    }
     
    // MISSING: toString conversion (for integers)
    inline std::string toString(int value) {
        return std::to_string(value);
    }
    
    inline std::string toString(double value) {
        return std::to_string(value);
    }
    
    inline std::string toString(bool value) {
        return value ? "true" : "false";
    }
    inline std::string repeat(const std::string& s, int count) {
        std::string result;
        for (int i = 0; i < count; i++) result += s;
        return result;
    }
    
    inline std::string substring(const std::string& s, int start, int length = -1) {
        if (length == -1) return s.substr(start);
        return s.substr(start, length);
    }
}

)";
  }
  static std::string generateGlobalOptionHelpers() {
    return R"(// ============================================================================
// Global Option Helper Functions (no namespace required)
// ============================================================================
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

)";
  }

static std::string generateCrypto() {
    return R"(// ============================================================================
// Std.Crypto - Cryptography Operations (AES-256-GCM)
// ============================================================================
namespace Crypto {
    // AES-256-GCM implementation using OpenSSL-compatible approach
    // This is a simplified version - production should use a proper crypto library
    
    inline std::vector<uint8_t> deriveKey(const std::string& password, 
                                          const std::vector<uint8_t>& salt) {
        // PBKDF2-like key derivation (simplified)
        std::vector<uint8_t> key(32); // 256 bits
        
        // Simple iterative hash (NOT secure for production!)
        std::string data = password;
        for (size_t i = 0; i < salt.size(); i++) {
            data += static_cast<char>(salt[i]);
        }
        
        // Hash iterations
        for (int iter = 0; iter < 10000; iter++) {
            std::hash<std::string> hasher;
            size_t hash = hasher(data);
            data = std::to_string(hash);
        }
        
        // Fill key with hash output
        for (size_t i = 0; i < 32; i++) {
            key[i] = static_cast<uint8_t>(data[i % data.size()] ^ (i * 7));
        }
        
        return key;
    }
    
    inline std::vector<uint8_t> generateSalt(size_t length = 16) {
        std::vector<uint8_t> salt(length);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        
        for (size_t i = 0; i < length; i++) {
            salt[i] = static_cast<uint8_t>(dis(gen));
        }
        
        return salt;
    }
    
    inline std::vector<uint8_t> generateIV(size_t length = 12) {
        return generateSalt(length); // IV is like salt
    }
    
    // Simple XOR cipher (for demonstration - use real AES in production)
    inline std::vector<uint8_t> encrypt(const std::vector<uint8_t>& plaintext,
                                       const std::vector<uint8_t>& key,
                                       const std::vector<uint8_t>& iv) {
        std::vector<uint8_t> ciphertext(plaintext.size());
        
        for (size_t i = 0; i < plaintext.size(); i++) {
            uint8_t keyByte = key[i % key.size()];
            uint8_t ivByte = iv[i % iv.size()];
            ciphertext[i] = plaintext[i] ^ keyByte ^ ivByte;
        }
        
        return ciphertext;
    }
    
    inline std::vector<uint8_t> decrypt(const std::vector<uint8_t>& ciphertext,
                                       const std::vector<uint8_t>& key,
                                       const std::vector<uint8_t>& iv) {
        // XOR is symmetric
        return encrypt(ciphertext, key, iv);
    }
    
    // High-level encrypt with password
    struct EncryptedData {
        std::vector<uint8_t> salt;
        std::vector<uint8_t> iv;
        std::vector<uint8_t> ciphertext;
    };
    
    inline EncryptedData encryptWithPassword(const std::vector<uint8_t>& data,
                                            const std::string& password) {
        EncryptedData result;
        result.salt = generateSalt();
        result.iv = generateIV();
        
        auto key = deriveKey(password, result.salt);
        result.ciphertext = encrypt(data, key, result.iv);
        
        return result;
    }
    
    inline std::vector<uint8_t> decryptWithPassword(const EncryptedData& encrypted,
                                                    const std::string& password) {
        auto key = deriveKey(password, encrypted.salt);
        return decrypt(encrypted.ciphertext, key, encrypted.iv);
    }
}

)";
}


  static std::string generateArray() {
    return R"(// ============================================================================
// Std.Array - Array Operations
// ============================================================================
namespace Array {
    template<typename T>
    inline int length(const std::vector<T>& arr) { return arr.size(); }
       template<typename T>
    inline std::vector<T> create() {
        return std::vector<T>();
    }
    template<typename T>
    inline bool isEmpty(const std::vector<T>& arr) { return arr.empty(); }
    
    template<typename T>
    inline void push(std::vector<T>& arr, const T& item) { arr.push_back(item); }
    
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
    
    template<typename T>
    inline std::optional<int> indexOf(const std::vector<T>& arr, const T& item) {
        auto it = std::find(arr.begin(), arr.end(), item);
        if (it != arr.end()) return std::distance(arr.begin(), it);
        return std::nullopt;
    }
    
    template<typename T>
    inline void clear(std::vector<T>& arr) { arr.clear(); }
}

)";
  }

  static std::string generateMap() {
    return R"(// ============================================================================
// Std.Map - HashMap/Dictionary Operations
// ============================================================================
namespace Map {
    template<typename K, typename V>
    using HashMap = std::unordered_map<K, V>;
    
    template<typename K, typename V>
    inline HashMap<K, V> create() { return HashMap<K, V>(); }
    
    template<typename K, typename V>
    inline void insert(HashMap<K,V>& map, const K& key, const V& value) {
        map[key] = value;
    }
    
    template<typename K, typename V>
    inline std::optional<V> get(const HashMap<K,V>& map, const K& key) {
        auto it = map.find(key);
        if (it != map.end()) return it->second;
        return std::nullopt;
    }
    
    template<typename K, typename V>
    inline V getOr(const HashMap<K,V>& map, const K& key, const V& defaultValue) {
        auto it = map.find(key);
        if (it != map.end()) return it->second;
        return defaultValue;
    }
    
    template<typename K, typename V>
    inline bool contains(const HashMap<K,V>& map, const K& key) {
        return map.find(key) != map.end();
    }
    
    template<typename K, typename V>
    inline void remove(HashMap<K,V>& map, const K& key) {
        map.erase(key);
    }
    
    template<typename K, typename V>
    inline int size(const HashMap<K,V>& map) { return map.size(); }
    
    template<typename K, typename V>
    inline bool isEmpty(const HashMap<K,V>& map) { return map.empty(); }
    
    template<typename K, typename V>
    inline void clear(HashMap<K,V>& map) { map.clear(); }
    
    template<typename K, typename V>
    inline std::vector<K> keys(const HashMap<K,V>& map) {
        std::vector<K> result;
        for (const auto& pair : map) result.push_back(pair.first);
        return result;
    }
    
    template<typename K, typename V>
    inline std::vector<V> values(const HashMap<K,V>& map) {
        std::vector<V> result;
        for (const auto& pair : map) result.push_back(pair.second);
        return result;
    }
}

)";
  }

  static std::string generateSet() {
    return R"(// ============================================================================
// Std.Set - HashSet Operations
// ============================================================================
namespace Set {
    template<typename T>
    using HashSet = std::unordered_set<T>;
    
    template<typename T>
    inline HashSet<T> create() { return HashSet<T>(); }
    
    template<typename T>
    inline void insert(HashSet<T>& set, const T& item) {
        set.insert(item);
    }
    
    template<typename T>
    inline bool contains(const HashSet<T>& set, const T& item) {
        return set.find(item) != set.end();
    }
    
    template<typename T>
    inline void remove(HashSet<T>& set, const T& item) {
        set.erase(item);
    }
    
    template<typename T>
    inline int size(const HashSet<T>& set) { return set.size(); }
    
    template<typename T>
    inline bool isEmpty(const HashSet<T>& set) { return set.empty(); }
    
    template<typename T>
    inline void clear(HashSet<T>& set) { set.clear(); }
    
    template<typename T>
    inline std::vector<T> toArray(const HashSet<T>& set) {
        return std::vector<T>(set.begin(), set.end());
    }
    
    template<typename T>
    inline HashSet<T> union_(const HashSet<T>& a, const HashSet<T>& b) {
        HashSet<T> result = a;
        for (const auto& item : b) result.insert(item);
        return result;
    }
    
    template<typename T>
    inline HashSet<T> intersection(const HashSet<T>& a, const HashSet<T>& b) {
        HashSet<T> result;
        for (const auto& item : a) {
            if (b.find(item) != b.end()) result.insert(item);
        }
        return result;
    }
    
    template<typename T>
    inline HashSet<T> difference(const HashSet<T>& a, const HashSet<T>& b) {
        HashSet<T> result;
        for (const auto& item : a) {
            if (b.find(item) == b.end()) result.insert(item);
        }
        return result;
    }
}

)";
  }

  static std::string generateFile() {
    return R"(// ============================================================================
// Std.File - File System Operations (High-level & DB-friendly)
// ============================================================================
#include <filesystem>
#include <fstream>
#include <optional>
#include <vector>
#include <string>

namespace File {

    // ------------------------------------------------------------------------
    // Path utilities
    // ------------------------------------------------------------------------
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
        } catch (...) { return false; }
    }

    inline bool remove(const std::string& path) {
        try {
            return std::filesystem::remove(path);
        } catch (...) { return false; }
    }

    inline bool removeAll(const std::string& path) {
        try {
            std::filesystem::remove_all(path);
            return true;
        } catch (...) { return false; }
    }

    inline bool copy(const std::string& from, const std::string& to) {
        try {
            std::filesystem::copy(from, to);
            return true;
        } catch (...) { return false; }
    }

    inline bool rename(const std::string& from, const std::string& to) {
        try {
            std::filesystem::rename(from, to);
            return true;
        } catch (...) { return false; }
    }

    inline std::optional<uint64_t> size(const std::string& path) {
        try {
            return std::filesystem::file_size(path);
        } catch (...) { return std::nullopt; }
    }

    // ------------------------------------------------------------------------
    // File handle (DB-friendly)
    // ------------------------------------------------------------------------
    struct Handle {
        std::fstream stream;
    };

    enum class Mode { Read, Write, ReadWrite, Append };
    enum class Seek { Begin, Current, End };

    // ------------------------------------------------------------------------
    // Open / Close
    // ------------------------------------------------------------------------
    inline std::optional<Handle> open(const std::string& path, Mode mode) {
        std::ios::openmode m = std::ios::binary;

        switch (mode) {
            case Mode::Read:      m |= std::ios::in; break;
            case Mode::Write:     m |= std::ios::out | std::ios::trunc; break;
            case Mode::ReadWrite: m |= std::ios::in | std::ios::out; break;
            case Mode::Append:    m |= std::ios::out | std::ios::app; break;
        }

        Handle h;
        h.stream.open(path, m);
        if (!h.stream.is_open())
            return std::nullopt;

        return h;
    }

    inline void close(Handle& h) {
        if (h.stream.is_open())
            h.stream.close();
    }

    // ------------------------------------------------------------------------
    // Byte I/O
    // ------------------------------------------------------------------------
    inline std::vector<uint8_t> read(Handle& h, size_t bytes) {
        std::vector<uint8_t> buffer(bytes);
        h.stream.read(reinterpret_cast<char*>(buffer.data()), bytes);
        buffer.resize(static_cast<size_t>(h.stream.gcount()));
        return buffer;
    }

    inline bool write(Handle& h, const std::vector<uint8_t>& data) {
        h.stream.write(reinterpret_cast<const char*>(data.data()), data.size());
        return h.stream.good();
    }

    inline bool flush(Handle& h) {
        h.stream.flush();
        return h.stream.good();
    }

    // ------------------------------------------------------------------------
    // Seeking
    // ------------------------------------------------------------------------
    inline bool seek(Handle& h, int64_t offset, Seek origin) {
        std::ios::seekdir dir;
        switch (origin) {
            case Seek::Begin:   dir = std::ios::beg; break;
            case Seek::Current: dir = std::ios::cur; break;
            case Seek::End:     dir = std::ios::end; break;
        }
        h.stream.seekg(offset, dir);
        h.stream.seekp(offset, dir);
        return h.stream.good();
    }

    inline int64_t tell(Handle& h) {
        return static_cast<int64_t>(h.stream.tellg());
    }

    // ------------------------------------------------------------------------
    // High-level convenience helpers
    // ------------------------------------------------------------------------
    inline bool write_u32(Handle& h, uint32_t value) {
        return write(h, std::vector<uint8_t>{
            static_cast<uint8_t>(value & 0xFF),
            static_cast<uint8_t>((value >> 8) & 0xFF),
            static_cast<uint8_t>((value >> 16) & 0xFF),
            static_cast<uint8_t>((value >> 24) & 0xFF)
        });
    }

    inline bool write_u64(Handle& h, uint64_t value) {
        std::vector<uint8_t> data(8);
        for (int i = 0; i < 8; i++) {
            data[i] = static_cast<uint8_t>((value >> (i * 8)) & 0xFF);
        }
        return write(h, data);
    }

    inline bool read_bytes(Handle& h, std::vector<uint8_t>& out, size_t count) {
        out.resize(count);
        h.stream.read(reinterpret_cast<char*>(out.data()), count);
        out.resize(static_cast<size_t>(h.stream.gcount()));
        return h.stream.good();
    }

} // namespace File
)";
  }

  static std::string generateTime() {
    return R"(// ============================================================================
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

)";
  }

  static std::string generateRandom() {
    return R"(// ============================================================================
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

)";
  }

  static std::string generateSystem() {
    return R"(// ============================================================================
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

)";
  }
  static std::string generateNetwork() {
    return R"(// ============================================================================
// Std.Network - Cross-Platform Web Development Runtime with Submodules
// ============================================================================
namespace Network {
    // ========================================================================
    // Platform detection and socket abstraction - MUST BE FIRST
    // ========================================================================
    #ifdef _WIN32
        #define PLATFORM_WINDOWS
    #else
        #define PLATFORM_UNIX
    #endif
    
    #ifdef PLATFORM_WINDOWS
        #include <winsock2.h>
        #include <ws2tcpip.h>
        #pragma comment(lib, "ws2_32.lib")
        typedef SOCKET socket_t;
        #define INVALID_SOCKET INVALID_SOCKET
    #else
        #include <sys/socket.h>
        #include <netinet/in.h>
        #include <arpa/inet.h>
        #include <unistd.h>
        #include <fcntl.h>
        typedef int socket_t;
        #define INVALID_SOCKET -1
    #endif
    
    // Network initialization/cleanup
    inline void initNetwork() {
        #ifdef PLATFORM_WINDOWS
            WSADATA wsaData;
            WSAStartup(MAKEWORD(2, 2), &wsaData);
        #endif
    }
    
    inline void cleanupNetwork() {
        #ifdef PLATFORM_WINDOWS
            WSACleanup();
        #endif
    }
    
    // ========================================================================
    // HTTP Status Codes - Define early for use throughout
    // ========================================================================
    namespace Status {
        // 2xx Success
        constexpr int OK = 200;
        constexpr int CREATED = 201;
        constexpr int ACCEPTED = 202;
        constexpr int NO_CONTENT = 204;
        
        // 3xx Redirection
        constexpr int MOVED_PERMANENTLY = 301;
        constexpr int FOUND = 302;
        constexpr int SEE_OTHER = 303;
        constexpr int NOT_MODIFIED = 304;
        constexpr int TEMPORARY_REDIRECT = 307;
        constexpr int PERMANENT_REDIRECT = 308;
        
        // 4xx Client Errors
        constexpr int BAD_REQUEST = 400;
        constexpr int UNAUTHORIZED = 401;
        constexpr int FORBIDDEN = 403;
        constexpr int NOT_FOUND = 404;
        constexpr int METHOD_NOT_ALLOWED = 405;
        constexpr int CONFLICT = 409;
        constexpr int GONE = 410;
        constexpr int PAYLOAD_TOO_LARGE = 413;
        constexpr int URI_TOO_LONG = 414;
        constexpr int UNSUPPORTED_MEDIA_TYPE = 415;
        constexpr int TOO_MANY_REQUESTS = 429;
        
        // 5xx Server Errors
        constexpr int INTERNAL_SERVER_ERROR = 500;
        constexpr int NOT_IMPLEMENTED = 501;
        constexpr int BAD_GATEWAY = 502;
        constexpr int SERVICE_UNAVAILABLE = 503;
        constexpr int GATEWAY_TIMEOUT = 504;
        
        inline std::string toString(int code) {
            switch (code) {
                case 200: return "OK";
                case 201: return "Created";
                case 202: return "Accepted";
                case 204: return "No Content";
                case 301: return "Moved Permanently";
                case 302: return "Found";
                case 303: return "See Other";
                case 304: return "Not Modified";
                case 307: return "Temporary Redirect";
                case 308: return "Permanent Redirect";
                case 400: return "Bad Request";
                case 401: return "Unauthorized";
                case 403: return "Forbidden";
                case 404: return "Not Found";
                case 405: return "Method Not Allowed";
                case 409: return "Conflict";
                case 410: return "Gone";
                case 413: return "Payload Too Large";
                case 414: return "URI Too Long";
                case 415: return "Unsupported Media Type";
                case 429: return "Too Many Requests";
                case 500: return "Internal Server Error";
                case 501: return "Not Implemented";
                case 502: return "Bad Gateway";
                case 503: return "Service Unavailable";
                case 504: return "Gateway Timeout";
                default: return "Unknown";
            }
        }
    }
    
    // ========================================================================
    // Cookie Management - Define early
    // ========================================================================
    struct Cookie {
        std::string name;
        std::string value;
        std::string path = "/";
        std::string domain;
        int maxAge = -1;
        bool httpOnly = false;
        bool secure = false;
        std::string sameSite = "Lax";
        
        std::string serialize() const {
            std::string result = name + "=" + value;
            if (!path.empty()) result += "; Path=" + path;
            if (!domain.empty()) result += "; Domain=" + domain;
            if (maxAge >= 0) result += "; Max-Age=" + std::to_string(maxAge);
            if (httpOnly) result += "; HttpOnly";
            if (secure) result += "; Secure";
            if (!sameSite.empty()) result += "; SameSite=" + sameSite;
            return result;
        }
    };
    
    // ========================================================================
    // HTTP Request/Response - Define BEFORE other classes use them
    // ========================================================================
    struct HttpRequest {
        std::string method;
        std::string path;
        std::string version;
        std::unordered_map<std::string, std::string> headers;
        std::unordered_map<std::string, std::string> query;
        std::unordered_map<std::string, std::string> cookies;
        std::unordered_map<std::string, std::string> formData;
        std::string body;
        std::string remoteAddr;
        
        std::string getHeader(const std::string& name) const {
            auto it = headers.find(name);
            return it != headers.end() ? it->second : "";
        }
        
        std::string getQuery(const std::string& name) const {
            auto it = query.find(name);
            return it != query.end() ? it->second : "";
        }
        
        std::string getCookie(const std::string& name) const {
            auto it = cookies.find(name);
            return it != cookies.end() ? it->second : "";
        }
        
        std::string getForm(const std::string& name) const {
            auto it = formData.find(name);
            return it != formData.end() ? it->second : "";
        }
        
        bool isJson() const {
            return getHeader("Content-Type").find("application/json") != std::string::npos;
        }
        
        bool isForm() const {
            auto ct = getHeader("Content-Type");
            return ct.find("application/x-www-form-urlencoded") != std::string::npos ||
                   ct.find("multipart/form-data") != std::string::npos;
        }
        
        bool acceptsJson() const {
            return getHeader("Accept").find("application/json") != std::string::npos;
        }
        
        bool acceptsHtml() const {
            return getHeader("Accept").find("text/html") != std::string::npos;
        }
    };
    
    struct HttpResponse {
        int statusCode;
        std::unordered_map<std::string, std::string> headers;
        std::vector<Cookie> cookies;
        std::string body;
        
        HttpResponse() : statusCode(200) {
            headers["Content-Type"] = "text/html; charset=utf-8";
            headers["Server"] = "Magolor/1.0";
            headers["X-Powered-By"] = "Magolor";
        }
        
        void setHeader(const std::string& name, const std::string& value) {
            headers[name] = value;
        }
        
        void setCookie(const Cookie& cookie) {
            cookies.push_back(cookie);
        }
        
        void setJson() {
            headers["Content-Type"] = "application/json; charset=utf-8";
        }
        
        void setText() {
            headers["Content-Type"] = "text/plain; charset=utf-8";
        }
        
        void setHtml() {
            headers["Content-Type"] = "text/html; charset=utf-8";
        }
        
        void setXml() {
            headers["Content-Type"] = "application/xml; charset=utf-8";
        }
        
        void setCors(const std::string& origin = "*") {
            headers["Access-Control-Allow-Origin"] = origin;
            headers["Access-Control-Allow-Methods"] = "GET, POST, PUT, DELETE, OPTIONS, PATCH";
            headers["Access-Control-Allow-Headers"] = "Content-Type, Authorization";
            headers["Access-Control-Allow-Credentials"] = "true";
        }
        
        void setCache(int maxAge) {
            headers["Cache-Control"] = "public, max-age=" + std::to_string(maxAge);
        }
        
        void setNoCache() {
            headers["Cache-Control"] = "no-cache, no-store, must-revalidate";
            headers["Pragma"] = "no-cache";
            headers["Expires"] = "0";
        }
        
        std::string serialize() const {
            std::ostringstream oss;
            oss << "HTTP/1.1 " << statusCode << " " << Status::toString(statusCode) << "\r\n";
            oss << "Content-Length: " << body.size() << "\r\n";
            
            for (const auto& [name, value] : headers) {
                oss << name << ": " << value << "\r\n";
            }
            
            for (const auto& cookie : cookies) {
                oss << "Set-Cookie: " << cookie.serialize() << "\r\n";
            }
            
            oss << "\r\n" << body;
            return oss.str();
        }
    };
    
    // ========================================================================
    // Submodule: Network.HTTP - HTTP-specific utilities
    // ========================================================================
    namespace HTTP {
        // HTTP Methods enum
        enum class Method {
            GET, POST, PUT, DELETE, PATCH, OPTIONS, HEAD, TRACE, CONNECT
        };
        
        inline std::string methodToString(Method m) {
            switch (m) {
                case Method::GET: return "GET";
                case Method::POST: return "POST";
                case Method::PUT: return "PUT";
                case Method::DELETE: return "DELETE";
                case Method::PATCH: return "PATCH";
                case Method::OPTIONS: return "OPTIONS";
                case Method::HEAD: return "HEAD";
                case Method::TRACE: return "TRACE";
                case Method::CONNECT: return "CONNECT";
                default: return "GET";
            }
        }
        
        inline Method stringToMethod(const std::string& s) {
            if (s == "POST") return Method::POST;
            if (s == "PUT") return Method::PUT;
            if (s == "DELETE") return Method::DELETE;
            if (s == "PATCH") return Method::PATCH;
            if (s == "OPTIONS") return Method::OPTIONS;
            if (s == "HEAD") return Method::HEAD;
            if (s == "TRACE") return Method::TRACE;
            if (s == "CONNECT") return Method::CONNECT;
            return Method::GET;
        }
        
        // HTTP Headers helper
        class Headers {
        private:
            std::unordered_map<std::string, std::string> data;
            
        public:
            void set(const std::string& name, const std::string& value) {
                data[name] = value;
            }
            
            std::string get(const std::string& name) const {
                auto it = data.find(name);
                return it != data.end() ? it->second : "";
            }
            
            bool has(const std::string& name) const {
                return data.find(name) != data.end();
            }
            
            void remove(const std::string& name) {
                data.erase(name);
            }
            
            const std::unordered_map<std::string, std::string>& getAll() const {
                return data;
            }
        };
        
        // Content-Type helpers
        namespace ContentType {
            constexpr const char* JSON = "application/json; charset=utf-8";
            constexpr const char* HTML = "text/html; charset=utf-8";
            constexpr const char* TEXT = "text/plain; charset=utf-8";
            constexpr const char* XML = "application/xml; charset=utf-8";
            constexpr const char* FORM = "application/x-www-form-urlencoded";
            constexpr const char* MULTIPART = "multipart/form-data";
            constexpr const char* CSS = "text/css";
            constexpr const char* JAVASCRIPT = "application/javascript";
            constexpr const char* PNG = "image/png";
            constexpr const char* JPEG = "image/jpeg";
            constexpr const char* GIF = "image/gif";
            constexpr const char* SVG = "image/svg+xml";
            constexpr const char* PDF = "application/pdf";
            constexpr const char* ZIP = "application/zip";
            constexpr const char* OCTET_STREAM = "application/octet-stream";
        }
    }
    
    // ========================================================================
    // Submodule: Network.WebSocket - WebSocket protocol
    // ========================================================================
    namespace WebSocket {
        enum class OpCode : uint8_t {
            CONTINUATION = 0x0,
            TEXT = 0x1,
            BINARY = 0x2,
            CLOSE = 0x8,
            PING = 0x9,
            PONG = 0xA
        };
        
        struct Frame {
            bool fin;
            OpCode opcode;
            bool masked;
            std::vector<uint8_t> payload;
        };
        
        class Connection {
        private:
            socket_t sock;
            bool connected = false;
            
            std::vector<uint8_t> encodeFrame(const std::string& message, OpCode opcode = OpCode::TEXT) {
                std::vector<uint8_t> frame;
                
                // FIN + OpCode
                frame.push_back(0x80 | static_cast<uint8_t>(opcode));
                
                // Payload length
                size_t len = message.size();
                if (len < 126) {
                    frame.push_back(static_cast<uint8_t>(len));
                } else if (len < 65536) {
                    frame.push_back(126);
                    frame.push_back((len >> 8) & 0xFF);
                    frame.push_back(len & 0xFF);
                } else {
                    frame.push_back(127);
                    for (int i = 7; i >= 0; i--) {
                        frame.push_back((len >> (i * 8)) & 0xFF);
                    }
                }
                
                // Payload
                for (char c : message) {
                    frame.push_back(static_cast<uint8_t>(c));
                }
                
                return frame;
            }
            
        public:
            Connection(socket_t s) : sock(s), connected(true) {}
            
            bool send(const std::string& message) {
                if (!connected) return false;
                
                auto frame = encodeFrame(message);
                return ::send(sock, (const char*)frame.data(), frame.size(), 0) > 0;
            }
            
            bool sendBinary(const std::vector<uint8_t>& data) {
                if (!connected) return false;
                
                std::string str(data.begin(), data.end());
                auto frame = encodeFrame(str, OpCode::BINARY);
                return ::send(sock, (const char*)frame.data(), frame.size(), 0) > 0;
            }
            
            bool ping() {
                if (!connected) return false;
                auto frame = encodeFrame("", OpCode::PING);
                return ::send(sock, (const char*)frame.data(), frame.size(), 0) > 0;
            }
            
            void closeSocket() {
                #ifdef PLATFORM_WINDOWS
                    closesocket(sock);
                #else
                    ::close(sock);
                #endif
                connected = false;
            }
            
            bool isConnected() const { return connected; }
        };
    }
    
    // ========================================================================
    // Submodule: Network.TCP - Raw TCP socket operations
    // ========================================================================
    namespace TCP {
        class Client {
        private:
            socket_t sock;
            bool connected = false;
            
        public:
            Client() : sock(INVALID_SOCKET) {
                initNetwork();
            }
            
            ~Client() {
                disconnect();
                cleanupNetwork();
            }
            
            bool connect(const std::string& host, int port) {
                sock = socket(AF_INET, SOCK_STREAM, 0);
                if (sock == INVALID_SOCKET) return false;
                
                sockaddr_in serverAddr;
                serverAddr.sin_family = AF_INET;
                serverAddr.sin_port = htons(port);
                
                #ifdef PLATFORM_WINDOWS
                    serverAddr.sin_addr.s_addr = inet_addr(host.c_str());
                #else
                    inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr);
                #endif
                
                if (::connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
                    #ifdef PLATFORM_WINDOWS
                        closesocket(sock);
                    #else
                        ::close(sock);
                    #endif
                    return false;
                }
                
                connected = true;
                return true;
            }
            
            bool send(const std::string& data) {
                if (!connected) return false;
                return ::send(sock, data.c_str(), data.size(), 0) > 0;
            }
            
            std::string receive(int maxBytes = 4096) {
                if (!connected) return "";
                
                char buffer[4096];
                int bytes = recv(sock, buffer, maxBytes, 0);
                if (bytes > 0) {
                    return std::string(buffer, bytes);
                }
                return "";
            }
            
            void disconnect() {
                if (connected) {
                    #ifdef PLATFORM_WINDOWS
                        closesocket(sock);
                    #else
                        ::close(sock);
                    #endif
                    connected = false;
                }
            }
            
            bool isConnected() const { return connected; }
        };
        
        class Server {
        private:
            socket_t serverSocket;
            int port;
            bool listening = false;
            
        public:
            Server(int p) : serverSocket(INVALID_SOCKET), port(p) {
                initNetwork();
            }
            
            ~Server() {
                stop();
                cleanupNetwork();
            }
            
            bool start() {
                serverSocket = socket(AF_INET, SOCK_STREAM, 0);
                if (serverSocket == INVALID_SOCKET) return false;
                
                int opt = 1;
                setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
                
                sockaddr_in serverAddr;
                serverAddr.sin_family = AF_INET;
                serverAddr.sin_addr.s_addr = INADDR_ANY;
                serverAddr.sin_port = htons(port);
                
                if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
                    #ifdef PLATFORM_WINDOWS
                        closesocket(serverSocket);
                    #else
                        ::close(serverSocket);
                    #endif
                    return false;
                }
                
                if (listen(serverSocket, 10) < 0) {
                    #ifdef PLATFORM_WINDOWS
                        closesocket(serverSocket);
                    #else
                        ::close(serverSocket);
                    #endif
                    return false;
                }
                
                listening = true;
                return true;
            }
            
            socket_t accept() {
                if (!listening) return INVALID_SOCKET;
                
                sockaddr_in clientAddr;
                socklen_t clientLen = sizeof(clientAddr);
                return ::accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
            }
            
            void stop() {
                if (listening) {
                    #ifdef PLATFORM_WINDOWS
                        closesocket(serverSocket);
                    #else
                        ::close(serverSocket);
                    #endif
                    listening = false;
                }
            }
        };
    }
    
    // ========================================================================
    // Submodule: Network.UDP - UDP socket operations
    // ========================================================================
    namespace UDP {
        class Socket {
        private:
            socket_t sock;
            bool bound = false;
            
        public:
            Socket() : sock(INVALID_SOCKET) {
                initNetwork();
            }
            
            ~Socket() {
                closeSocket();
                cleanupNetwork();
            }
            
            bool bind(int port) {
                sock = socket(AF_INET, SOCK_DGRAM, 0);
                if (sock == INVALID_SOCKET) return false;
                
                sockaddr_in addr;
                addr.sin_family = AF_INET;
                addr.sin_addr.s_addr = INADDR_ANY;
                addr.sin_port = htons(port);
                
                if (::bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
                    #ifdef PLATFORM_WINDOWS
                        closesocket(sock);
                    #else
                        ::close(sock);
                    #endif
                    return false;
                }
                
                bound = true;
                return true;
            }
            
            bool sendTo(const std::string& data, const std::string& host, int port) {
                if (sock == INVALID_SOCKET) {
                    sock = socket(AF_INET, SOCK_DGRAM, 0);
                    if (sock == INVALID_SOCKET) return false;
                }
                
                sockaddr_in addr;
                addr.sin_family = AF_INET;
                addr.sin_port = htons(port);
                
                #ifdef PLATFORM_WINDOWS
                    addr.sin_addr.s_addr = inet_addr(host.c_str());
                #else
                    inet_pton(AF_INET, host.c_str(), &addr.sin_addr);
                #endif
                
                return sendto(sock, data.c_str(), data.size(), 0, 
                             (struct sockaddr*)&addr, sizeof(addr)) > 0;
            }
            
            std::string receiveFrom(std::string& fromHost, int& fromPort) {
                if (!bound) return "";
                
                char buffer[4096];
                sockaddr_in fromAddr;
                socklen_t fromLen = sizeof(fromAddr);
                
                int bytes = recvfrom(sock, buffer, sizeof(buffer), 0,
                                    (struct sockaddr*)&fromAddr, &fromLen);
                
                if (bytes > 0) {
                    fromHost = inet_ntoa(fromAddr.sin_addr);
                    fromPort = ntohs(fromAddr.sin_port);
                    return std::string(buffer, bytes);
                }
                
                return "";
            }
            
            void closeSocket() {
                #ifdef PLATFORM_WINDOWS
                    closesocket(sock);
                #else
                    ::close(sock);
                #endif
                bound = false;
            }
        };
    }
    
    // ========================================================================
    // Submodule: Network.Security - Security utilities
    // ========================================================================
    namespace Security {
        // Basic XSS protection
        inline std::string escapeHtml(const std::string& str) {
            std::string result;
            for (char c : str) {
                switch (c) {
                    case '<': result += "&lt;"; break;
                    case '>': result += "&gt;"; break;
                    case '&': result += "&amp;"; break;
                    case '"': result += "&quot;"; break;
                    case '\'': result += "&#x27;"; break;
                    case '/': result += "&#x2F;"; break;
                    default: result += c;
                }
            }
            return result;
        }
        
        // Generate random token
        inline std::string generateToken(int length = 32) {
            static const char* chars = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
            std::string token;
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 61);
            
            for (int i = 0; i < length; i++) {
                token += chars[dis(gen)];
            }
            return token;
        }
        
        // Generate CSRF token
        inline std::string generateCsrfToken() {
            return generateToken(64);
        }
        
        // Basic rate limiter
        class RateLimiter {
        private:
            std::unordered_map<std::string, std::vector<std::chrono::steady_clock::time_point>> requests;
            int maxRequests;
            int windowSeconds;
            
        public:
            RateLimiter(int max, int window) : maxRequests(max), windowSeconds(window) {}
            
            bool allow(const std::string& clientId) {
                auto now = std::chrono::steady_clock::now();
                auto& times = requests[clientId];
                
                // Remove old entries
                times.erase(
                    std::remove_if(times.begin(), times.end(),
                        [now, this](const auto& t) {
                            return std::chrono::duration_cast<std::chrono::seconds>(now - t).count() > windowSeconds;
                        }),
                    times.end()
                );
                
                if (times.size() >= static_cast<size_t>(maxRequests)) {
                    return false;
                }
                
                times.push_back(now);
                return true;
            }
            
            void reset(const std::string& clientId) {
                requests.erase(clientId);
            }
        };
        
        // CORS configuration
        struct CorsConfig {
            std::vector<std::string> allowedOrigins;
            std::vector<std::string> allowedMethods;
            std::vector<std::string> allowedHeaders;
            bool allowCredentials = false;
            int maxAge = 86400; // 24 hours
            
            CorsConfig() {
                allowedMethods = {"GET", "POST", "PUT", "DELETE", "OPTIONS", "PATCH"};
                allowedHeaders = {"Content-Type", "Authorization"};
            }
        };
    }
    
    // ========================================================================
    // Submodule: Network.JSON - JSON utilities
    // ========================================================================
    namespace JSON {
        // JSON value types
        enum class Type {
            Null, Boolean, Number, String, Array, Object
        };
        
        // Simple JSON parser (basic implementation)
        class Parser {
        public:
            static std::string escape(const std::string& str) {
                std::string result;
                for (char c : str) {
                    switch (c) {
                        case '"': result += "\\\""; break;
                        case '\\': result += "\\\\"; break;
                        case '\n': result += "\\n"; break;
                        case '\r': result += "\\r"; break;
                        case '\t': result += "\\t"; break;
                        default: result += c;
                    }
                }
                return result;
            }
            
            static std::string unescape(const std::string& str) {
                std::string result;
                for (size_t i = 0; i < str.size(); i++) {
                    if (str[i] == '\\' && i + 1 < str.size()) {
                        switch (str[i + 1]) {
                            case '"': result += '"'; i++; break;
                            case '\\': result += '\\'; i++; break;
                            case 'n': result += '\n'; i++; break;
                            case 'r': result += '\r'; i++; break;
                            case 't': result += '\t'; i++; break;
                            default: result += str[i];
                        }
                    } else {
                        result += str[i];
                    }
                }
                return result;
            }
        };
        
        // Array builder
        class ArrayBuilder {
        private:
            std::ostringstream json;
            bool first = true;
            
        public:
            ArrayBuilder() { json << "["; }
            
            ArrayBuilder& add(const std::string& value) {
                if (!first) json << ",";
                json << "\"" << Parser::escape(value) << "\"";
                first = false;
                return *this;
            }
            
            ArrayBuilder& add(int value) {
                if (!first) json << ",";
                json << value;
                first = false;
                return *this;
            }
            
            ArrayBuilder& add(double value) {
                if (!first) json << ",";
                json << value;
                first = false;
                return *this;
            }
            
            ArrayBuilder& add(bool value) {
                if (!first) json << ",";
                json << (value ? "true" : "false");
                first = false;
                return *this;
            }
            
            std::string build() {
                return json.str() + "]";
            }
        };
    }
    
    // ========================================================================
    // Submodule: Network.Routing - Advanced routing
    // ========================================================================
    namespace Routing {
        // Route parameter extraction
        struct RouteMatch {
            bool matches = false;
            std::unordered_map<std::string, std::string> params;
        };
        
        inline RouteMatch matchRoute(const std::string& pattern, const std::string& path) {
            RouteMatch result;
            
            std::vector<std::string> patternParts;
            std::vector<std::string> pathParts;
            
            // Split pattern and path
            std::stringstream patternStream(pattern);
            std::stringstream pathStream(path);
            std::string part;
            
            while (std::getline(patternStream, part, '/')) {
                if (!part.empty()) patternParts.push_back(part);
            }
            
            while (std::getline(pathStream, part, '/')) {
                if (!part.empty()) pathParts.push_back(part);
            }
            
            if (patternParts.size() != pathParts.size()) {
                return result;
            }
            
            for (size_t i = 0; i < patternParts.size(); i++) {
                if (patternParts[i][0] == ':') {
                    // Parameter
                    std::string paramName = patternParts[i].substr(1);
                    result.params[paramName] = pathParts[i];
                } else if (patternParts[i] != pathParts[i]) {
                    return result;
                }
            }
            
            result.matches = true;
            return result;
        }
        
        // Router class
        class Router {
        private:
            struct Route {
                std::string pattern;
                std::string method;
                std::function<HttpResponse(const HttpRequest&, const std::unordered_map<std::string, std::string>&)> handler;
            };
            
            std::vector<Route> routes;
            
        public:
            void add(const std::string& method, const std::string& pattern,
                    std::function<HttpResponse(const HttpRequest&, const std::unordered_map<std::string, std::string>&)> handler) {
                routes.push_back({pattern, method, handler});
            }
            
            HttpResponse route(const HttpRequest& req) {
                for (const auto& route : routes) {
                    if (route.method != req.method) continue;
                    
                    auto match = matchRoute(route.pattern, req.path);
                    if (match.matches) {
                        return route.handler(req, match.params);
                    }
                }
                
                HttpResponse res;
                res.statusCode = 404;
                res.body = "Not Found";
                return res;
            }
        };
    }
    
    // ========================================================================
    // Session Management with Automatic Cleanup
    // ========================================================================
    class SessionStore {
    private:
        std::unordered_map<std::string, std::unordered_map<std::string, std::string>> sessions;
        std::unordered_map<std::string, std::chrono::steady_clock::time_point> expirations;
        int defaultTimeout = 3600; // 1 hour
        
    public:
        std::string create() {
            std::string id;
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 15);
            const char* hex = "0123456789abcdef";
            
            for (int i = 0; i < 32; i++) {
                id += hex[dis(gen)];
            }
            
            sessions[id] = {};
            expirations[id] = std::chrono::steady_clock::now() + std::chrono::seconds(defaultTimeout);
            return id;
        }
        
        bool exists(const std::string& id) {
            cleanup();
            return sessions.find(id) != sessions.end();
        }
        
        void set(const std::string& id, const std::string& key, const std::string& value) {
            if (exists(id)) {
                sessions[id][key] = value;
                expirations[id] = std::chrono::steady_clock::now() + std::chrono::seconds(defaultTimeout);
            }
        }
        
        std::string get(const std::string& id, const std::string& key) {
            if (exists(id)) {
                auto it = sessions[id].find(key);
                return it != sessions[id].end() ? it->second : "";
            }
            return "";
        }
        
        void destroy(const std::string& id) {
            sessions.erase(id);
            expirations.erase(id);
        }
        
        void cleanup() {
            auto now = std::chrono::steady_clock::now();
            std::vector<std::string> expired;
            for (const auto& [id, exp] : expirations) {
                if (now > exp) expired.push_back(id);
            }
            for (const auto& id : expired) destroy(id);
        }
        
        void setTimeout(int seconds) {
            defaultTimeout = seconds;
        }
    };
    
    // ========================================================================
    // URL Utilities
    // ========================================================================
    inline std::string urlDecode(const std::string& str) {
        std::string result;
        for (size_t i = 0; i < str.size(); i++) {
            if (str[i] == '%' && i + 2 < str.size()) {
                int value;
                std::istringstream is(str.substr(i + 1, 2));
                if (is >> std::hex >> value) {
                    result += static_cast<char>(value);
                    i += 2;
                } else {
                    result += str[i];
                }
            } else if (str[i] == '+') {
                result += ' ';
            } else {
                result += str[i];
            }
        }
        return result;
    }
    
    inline std::string urlEncode(const std::string& str) {
        std::ostringstream escaped;
        escaped.fill('0');
        escaped << std::hex;
        
        for (char c : str) {
            if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                escaped << c;
            } else {
                escaped << std::uppercase;
                escaped << '%' << std::setw(2) << int((unsigned char)c);
                escaped << std::nouppercase;
            }
        }
        
        return escaped.str();
    }
    
    inline std::unordered_map<std::string, std::string> parseQuery(const std::string& query) {
        std::unordered_map<std::string, std::string> result;
        std::istringstream iss(query);
        std::string pair;
        
        while (std::getline(iss, pair, '&')) {
            size_t eq = pair.find('=');
            if (eq != std::string::npos) {
                std::string key = urlDecode(pair.substr(0, eq));
                std::string value = urlDecode(pair.substr(eq + 1));
                result[key] = value;
            }
        }
        
        return result;
    }
    
    inline std::unordered_map<std::string, std::string> parseCookies(const std::string& cookieHeader) {
        std::unordered_map<std::string, std::string> cookies;
        std::istringstream iss(cookieHeader);
        std::string pair;
        
        while (std::getline(iss, pair, ';')) {
            pair.erase(0, pair.find_first_not_of(" \t"));
            size_t eq = pair.find('=');
            if (eq != std::string::npos) {
                std::string name = pair.substr(0, eq);
                std::string value = pair.substr(eq + 1);
                cookies[name] = value;
            }
        }
        
        return cookies;
    }
    
    // ========================================================================
    // Enhanced Request Parser
    // ========================================================================
    inline HttpRequest parseRequest(const std::string& raw) {
        HttpRequest req;
        std::istringstream stream(raw);
        std::string line;
        
        // Parse request line
        if (std::getline(stream, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            
            std::istringstream lineStream(line);
            lineStream >> req.method >> req.path >> req.version;
            
            // Parse query string
            size_t qPos = req.path.find('?');
            if (qPos != std::string::npos) {
                req.query = parseQuery(req.path.substr(qPos + 1));
                req.path = req.path.substr(0, qPos);
            }
        }
        
        // Parse headers
        while (std::getline(stream, line) && line != "\r" && !line.empty()) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            
            size_t colon = line.find(':');
            if (colon != std::string::npos) {
                std::string name = line.substr(0, colon);
                std::string value = line.substr(colon + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
                req.headers[name] = value;
                
                // Parse cookies
                if (name == "Cookie") {
                    req.cookies = parseCookies(value);
                }
            }
        }
        
        // Parse body
        std::ostringstream bodyStream;
        bodyStream << stream.rdbuf();
        req.body = bodyStream.str();
        
        // Parse form data if applicable
        if (req.isForm() && !req.body.empty()) {
            req.formData = parseQuery(req.body);
        }
        
        return req;
    }
    
    // ========================================================================
    // Middleware System
    // ========================================================================
    using RouteHandler = std::function<HttpResponse(const HttpRequest&)>;
    using Middleware = std::function<bool(HttpRequest&, HttpResponse&)>;
    
    // ========================================================================
    // JSON Builder Helper
    // ========================================================================
    class JsonBuilder {
    private:
        std::ostringstream json;
        bool first = true;
        
    public:
        JsonBuilder() { json << "{"; }
        
        JsonBuilder& add(const std::string& key, const std::string& value) {
            if (!first) json << ",";
            json << "\"" << key << "\":\"" << value << "\"";
            first = false;
            return *this;
        }
        
        JsonBuilder& add(const std::string& key, int value) {
            if (!first) json << ",";
            json << "\"" << key << "\":" << value;
            first = false;
            return *this;
        }
        
        JsonBuilder& add(const std::string& key, double value) {
            if (!first) json << ",";
            json << "\"" << key << "\":" << value;
            first = false;
            return *this;
        }
        
        JsonBuilder& add(const std::string& key, bool value) {
            if (!first) json << ",";
            json << "\"" << key << "\":" << (value ? "true" : "false");
            first = false;
            return *this;
        }
        
        std::string build() {
            return json.str() + "}";
        }
    };
    
    // ========================================================================
    // Cross-Platform HTTP Server
    // ========================================================================
    class HttpServer {
    private:
        int port;
        socket_t serverSocket;
        bool running;
        std::unordered_map<std::string, std::unordered_map<std::string, RouteHandler>> routes;
        std::vector<Middleware> middlewares;
        RouteHandler notFoundHandler;
        SessionStore sessions;
        
        void setupSocket() {
            initNetwork();
            
            serverSocket = socket(AF_INET, SOCK_STREAM, 0);
            if (serverSocket == INVALID_SOCKET) {
                throw std::runtime_error("Failed to create socket");
            }
            
            int opt = 1;
            setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
            
            sockaddr_in serverAddr;
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_addr.s_addr = INADDR_ANY;
            serverAddr.sin_port = htons(port);
            
            if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
                #ifdef PLATFORM_WINDOWS
                    closesocket(serverSocket);
                #else
                    ::close(serverSocket);
                #endif
                throw std::runtime_error("Failed to bind to port " + std::to_string(port));
            }
            
            if (listen(serverSocket, 10) < 0) {
                #ifdef PLATFORM_WINDOWS
                    closesocket(serverSocket);
                #else
                    ::close(serverSocket);
                #endif
                throw std::runtime_error("Failed to listen on socket");
            }
        }
        
        void handleClient(socket_t clientSocket, const std::string& clientAddr) {
            char buffer[8192];
            int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            
            if (bytesRead > 0) {
                buffer[bytesRead] = '\0';
                std::string rawRequest(buffer);
                
                HttpRequest request = parseRequest(rawRequest);
                request.remoteAddr = clientAddr;
                
                // Run middlewares
                HttpResponse response;
                bool continueProcessing = true;
                for (auto& middleware : middlewares) {
                    if (!middleware(request, response)) {
                        continueProcessing = false;
                        break;
                    }
                }
                
                if (continueProcessing) {
                    response = routeRequest(request);
                }
                
                std::string responseStr = response.serialize();
                ::send(clientSocket, responseStr.c_str(), responseStr.size(), 0);
            }
            
            #ifdef PLATFORM_WINDOWS
                closesocket(clientSocket);
            #else
                ::close(clientSocket);
            #endif
        }
        
        HttpResponse routeRequest(const HttpRequest& req) {
            auto methodIt = routes.find(req.method);
            if (methodIt != routes.end()) {
                auto pathIt = methodIt->second.find(req.path);
                if (pathIt != methodIt->second.end()) {
                    return pathIt->second(req);
                }
            }
            
            if (notFoundHandler) {
                return notFoundHandler(req);
            }
            
            HttpResponse response;
            response.statusCode = Status::NOT_FOUND;
            response.body = "<h1>404 Not Found</h1>";
            return response;
        }
        
    public:
        HttpServer(int p) : port(p), serverSocket(INVALID_SOCKET), running(false) {
            notFoundHandler = [](const HttpRequest& req) {
                HttpResponse res;
                res.statusCode = Status::NOT_FOUND;
                res.body = "<h1>404 Not Found</h1><p>Path: " + req.path + "</p>";
                return res;
            };
        }
        
        ~HttpServer() {
            stop();
            cleanupNetwork();
        }
        
        void use(Middleware middleware) {
            middlewares.push_back(middleware);
        }
        
        void get(const std::string& path, RouteHandler handler) {
            routes["GET"][path] = handler;
        }
        
        void post(const std::string& path, RouteHandler handler) {
            routes["POST"][path] = handler;
        }
        
        void put(const std::string& path, RouteHandler handler) {
            routes["PUT"][path] = handler;
        }
        
        void delete_(const std::string& path, RouteHandler handler) {
            routes["DELETE"][path] = handler;
        }
        
        void options(const std::string& path, RouteHandler handler) {
            routes["OPTIONS"][path] = handler;
        }
        
        void patch(const std::string& path, RouteHandler handler) {
            routes["PATCH"][path] = handler;
        }
        
        void setNotFound(RouteHandler handler) {
            notFoundHandler = handler;
        }
        
        SessionStore& getSessionStore() {
            return sessions;
        }
        
        void start() {
            setupSocket();
            running = true;
            
            std::cout << "Server listening on http://localhost:" << port << std::endl;
            
            while (running) {
                sockaddr_in clientAddr;
                socklen_t clientLen = sizeof(clientAddr);
                
                socket_t clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
                if (clientSocket == INVALID_SOCKET) {
                    if (running) {
                        std::cerr << "Failed to accept connection" << std::endl;
                    }
                    continue;
                }
                
                std::string clientIp = inet_ntoa(clientAddr.sin_addr);
                handleClient(clientSocket, clientIp);
            }
        }
        
        void stop() {
            running = false;
            if (serverSocket != INVALID_SOCKET) {
                #ifdef PLATFORM_WINDOWS
                    closesocket(serverSocket);
                #else
                    ::close(serverSocket);
                #endif
                serverSocket = INVALID_SOCKET;
            }
        }
    };
    
    // ========================================================================
    // Helper Functions
    // ========================================================================
    inline HttpResponse jsonResponse(const std::string& json, int status = Status::OK) {
        HttpResponse res;
        res.statusCode = status;
        res.setJson();
        res.body = json;
        return res;
    }
    
    inline HttpResponse htmlResponse(const std::string& html, int status = Status::OK) {
        HttpResponse res;
        res.statusCode = status;
        res.setHtml();
        res.body = html;
        return res;
    }
    
    inline HttpResponse textResponse(const std::string& text, int status = Status::OK) {
        HttpResponse res;
        res.statusCode = status;
        res.setText();
        res.body = text;
        return res;
    }
    
    inline HttpResponse redirectResponse(const std::string& url, int status = 302) {
        HttpResponse res;
        res.statusCode = status;
        res.setHeader("Location", url);
        res.body = "<html><body>Redirecting to <a href=\"" + url + "\">" + url + "</a></body></html>";
        return res;
    }
    
    inline HttpResponse errorResponse(int status, const std::string& message) {
        HttpResponse res;
        res.statusCode = status;
        res.body = "<h1>" + std::to_string(status) + " " + Status::toString(status) + "</h1>";
        res.body += "<p>" + message + "</p>";
        return res;
    }
    
    // ========================================================================
    // Built-in Middleware
    // ========================================================================
    inline Middleware corsMiddleware(const std::string& origin = "*") {
        return [origin](HttpRequest& req, HttpResponse& res) {
            res.setCors(origin);
            if (req.method == "OPTIONS") {
                res.statusCode = Status::NO_CONTENT;
                return false; // Stop processing
            }
            return true; // Continue
        };
    }
    
    inline Middleware loggerMiddleware() {
        return [](HttpRequest& req, HttpResponse& res) {
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            std::cout << "[" << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << "] "
                      << req.remoteAddr << " - " << req.method << " " << req.path << std::endl;
            return true;
        };
    }
    
    // ========================================================================
    // Static File Serving
    // ========================================================================
    inline HttpResponse serveFile(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file) {
            return errorResponse(Status::NOT_FOUND, "File not found");
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        
        HttpResponse res;
        res.body = buffer.str();
        
        // Set content type based on extension
        std::string ext = filepath.substr(filepath.find_last_of('.') + 1);
        if (ext == "html" || ext == "htm") res.setHtml();
        else if (ext == "css") res.setHeader("Content-Type", "text/css");
        else if (ext == "js") res.setHeader("Content-Type", "application/javascript");
        else if (ext == "json") res.setJson();
        else if (ext == "png") res.setHeader("Content-Type", "image/png");
        else if (ext == "jpg" || ext == "jpeg") res.setHeader("Content-Type", "image/jpeg");
        else if (ext == "gif") res.setHeader("Content-Type", "image/gif");
        else if (ext == "svg") res.setHeader("Content-Type", "image/svg+xml");
        else if (ext == "ico") res.setHeader("Content-Type", "image/x-icon");
        else if (ext == "pdf") res.setHeader("Content-Type", "application/pdf");
        else if (ext == "zip") res.setHeader("Content-Type", "application/zip");
        else res.setHeader("Content-Type", "application/octet-stream");
        
        return res;
    }
}

)";
  }
  static std::string generateTopLevel() {
    return R"(// ============================================================================
// Top-level convenience functions with universal print support
// ============================================================================
// Universal print - works with any type
template<typename T>
inline void print(const T& val) { 
    IO::print(mg_to_string(val)); 
}

template<typename T>
inline void println(const T& val) { 
    IO::println(mg_to_string(val)); 
}

// String overloads for efficiency
inline void print(const std::string& s) { IO::print(s); }
inline void println(const std::string& s) { IO::println(s); }
inline void print(const char* s) { IO::print(std::string(s)); }
inline void println(const char* s) { IO::println(std::string(s)); }
inline std::string toString(int value) {
    return std::to_string(value);
}

inline std::string toString(double value) {
    return std::to_string(value);
}

inline std::string toString(bool value) {
    return value ? "true" : "false";
}

inline std::string toString(const std::string& value) {
    return value;
}
inline std::string readLine() { return IO::readLine(); }
inline std::optional<int> parseInt(const std::string& s) { return Parse::parseInt(s); }
inline std::optional<double> parseFloat(const std::string& s) { return Parse::parseFloat(s); }

)";
  }

  static std::string generateTemplateHelpers() {
    return R"(// ============================================================================
// Template Helpers for String Conversion
// ============================================================================
template<typename T>
inline std::string mg_to_string(const T& val) { 
    // Try to use << operator if available
    std::ostringstream oss; 
    oss << val; 
    return oss.str(); 
}

template<>
inline std::string mg_to_string(const bool& val) {
    return val ? "true" : "false";
}

template<>
inline std::string mg_to_string(const std::string& val) {
    return val;
}

// Support for pointers to objects (for class instances)
template<typename T>
inline std::string mg_to_string(const T* val) {
    if (!val) return "null";
    std::ostringstream oss;
    oss << "<" << typeid(T).name() << " @ " << (void*)val << ">";
    return oss.str();
}

)";
  }
// Add to stdlib.hpp after generateTemplateHelpers()

};
