#pragma once
#include <sstream>
#include <string>

class StdLibGenerator {
public:
  static std::string generateAll() {
    std::stringstream ss;

    // Required includes
    ss << generateIncludes();
    ss << "\n";

    // Namespace opening
    ss << "namespace Std {\n\n";

    // Generate each module
    ss << generateIO();
    ss << generateParse();
    ss << generateOption();
    ss << generateMath();
    ss << generateString();
    ss << generateArray();
    ss << generateMap(); // NEW
    ss << generateSet(); // NEW
    ss << generateFile();
    ss << generateNetwork();
    ss << generateTime();
    ss << generateRandom();
    ss << generateSystem();

    // Top-level convenience functions
    ss << generateTopLevel();

    // Namespace closing
    ss << "} // namespace Std\n\n";

    // Template helpers
    ss << generateTemplateHelpers();

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

  static std::string generateArray() {
    return R"(// ============================================================================
// Std.Array - Array Operations
// ============================================================================
namespace Array {
    template<typename T>
    inline int length(const std::vector<T>& arr) { return arr.size(); }
    
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
    
    inline std::optional<int> size(const std::string& path) {
        try {
            return std::filesystem::file_size(path);
        } catch (...) { return std::nullopt; }
    }
}

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
// Std.Network - Network and HTTP Server Operations
// ============================================================================
namespace Network {
    // HTTP Status Codes
    namespace Status {
        constexpr int OK = 200;
        constexpr int CREATED = 201;
        constexpr int NO_CONTENT = 204;
        constexpr int BAD_REQUEST = 400;
        constexpr int UNAUTHORIZED = 401;
        constexpr int FORBIDDEN = 403;
        constexpr int NOT_FOUND = 404;
        constexpr int METHOD_NOT_ALLOWED = 405;
        constexpr int INTERNAL_SERVER_ERROR = 500;
        constexpr int NOT_IMPLEMENTED = 501;
        constexpr int SERVICE_UNAVAILABLE = 503;
        
        inline std::string toString(int code) {
            switch (code) {
                case 200: return "OK";
                case 201: return "Created";
                case 204: return "No Content";
                case 400: return "Bad Request";
                case 401: return "Unauthorized";
                case 403: return "Forbidden";
                case 404: return "Not Found";
                case 405: return "Method Not Allowed";
                case 500: return "Internal Server Error";
                case 501: return "Not Implemented";
                case 503: return "Service Unavailable";
                default: return "Unknown";
            }
        }
    }
    
    // HTTP Request Structure
    struct HttpRequest {
        std::string method;
        std::string path;
        std::string version;
        std::unordered_map<std::string, std::string> headers;
        std::unordered_map<std::string, std::string> query;
        std::string body;
        
        std::string getHeader(const std::string& name) const {
            auto it = headers.find(name);
            return it != headers.end() ? it->second : "";
        }
        
        std::string getQuery(const std::string& name) const {
            auto it = query.find(name);
            return it != query.end() ? it->second : "";
        }
    };
    
    // HTTP Response Structure
    struct HttpResponse {
        int statusCode;
        std::unordered_map<std::string, std::string> headers;
        std::string body;
        
        HttpResponse() : statusCode(200) {
            headers["Content-Type"] = "text/html; charset=utf-8";
            headers["Server"] = "Magolor/1.0";
        }
        
        void setHeader(const std::string& name, const std::string& value) {
            headers[name] = value;
        }
        
        void setJson() {
            headers["Content-Type"] = "application/json";
        }
        
        void setText() {
            headers["Content-Type"] = "text/plain";
        }
        
        void setHtml() {
            headers["Content-Type"] = "text/html; charset=utf-8";
        }
        
        std::string serialize() const {
            std::ostringstream oss;
            oss << "HTTP/1.1 " << statusCode << " " << Status::toString(statusCode) << "\r\n";
            oss << "Content-Length: " << body.size() << "\r\n";
            
            for (const auto& [name, value] : headers) {
                oss << name << ": " << value << "\r\n";
            }
            
            oss << "\r\n" << body;
            return oss.str();
        }
    };
    
    // URL Utilities
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
    
    // Request Parser
    inline HttpRequest parseRequest(const std::string& raw) {
        HttpRequest req;
        std::istringstream stream(raw);
        std::string line;
        
        if (std::getline(stream, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            
            std::istringstream lineStream(line);
            lineStream >> req.method >> req.path >> req.version;
            
            size_t qPos = req.path.find('?');
            if (qPos != std::string::npos) {
                req.query = parseQuery(req.path.substr(qPos + 1));
                req.path = req.path.substr(0, qPos);
            }
        }
        
        while (std::getline(stream, line) && line != "\r" && !line.empty()) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            
            size_t colon = line.find(':');
            if (colon != std::string::npos) {
                std::string name = line.substr(0, colon);
                std::string value = line.substr(colon + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
                req.headers[name] = value;
            }
        }
        
        std::ostringstream bodyStream;
        bodyStream << stream.rdbuf();
        req.body = bodyStream.str();
        
        return req;
    }
    
    // Route Handler Type
    using RouteHandler = std::function<HttpResponse(const HttpRequest&)>;
    
    // Simple HTTP Server
    class HttpServer {
    private:
        int port;
        int serverSocket;
        bool running;
        std::unordered_map<std::string, std::unordered_map<std::string, RouteHandler>> routes;
        RouteHandler notFoundHandler;
        
        void setupSocket() {
            serverSocket = socket(AF_INET, SOCK_STREAM, 0);
            if (serverSocket < 0) {
                throw std::runtime_error("Failed to create socket");
            }
            
            int opt = 1;
            setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
            
            sockaddr_in serverAddr;
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_addr.s_addr = INADDR_ANY;
            serverAddr.sin_port = htons(port);
            
            if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
                close(serverSocket);
                throw std::runtime_error("Failed to bind to port " + std::to_string(port));
            }
            
            if (listen(serverSocket, 10) < 0) {
                close(serverSocket);
                throw std::runtime_error("Failed to listen on socket");
            }
        }
        
        void handleClient(int clientSocket) {
            char buffer[4096];
            int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            
            if (bytesRead > 0) {
                buffer[bytesRead] = '\0';
                std::string rawRequest(buffer);
                
                HttpRequest request = parseRequest(rawRequest);
                HttpResponse response = routeRequest(request);
                
                std::string responseStr = response.serialize();
                send(clientSocket, responseStr.c_str(), responseStr.size(), 0);
            }
            
            close(clientSocket);
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
        HttpServer(int p) : port(p), serverSocket(-1), running(false) {
            notFoundHandler = [](const HttpRequest& req) {
                HttpResponse res;
                res.statusCode = Status::NOT_FOUND;
                res.body = "<h1>404 Not Found</h1><p>Path: " + req.path + "</p>";
                return res;
            };
        }
        
        ~HttpServer() {
            stop();
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
        
        void setNotFound(RouteHandler handler) {
            notFoundHandler = handler;
        }
        
        void start() {
            setupSocket();
            running = true;
            
            std::cout << "Server listening on http://localhost:" << port << std::endl;
            
            while (running) {
                sockaddr_in clientAddr;
                socklen_t clientLen = sizeof(clientAddr);
                
                int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
                if (clientSocket < 0) {
                    if (running) {
                        std::cerr << "Failed to accept connection" << std::endl;
                    }
                    continue;
                }
                
                handleClient(clientSocket);
            }
        }
        
        void stop() {
            running = false;
            if (serverSocket >= 0) {
                close(serverSocket);
                serverSocket = -1;
            }
        }
    };
    
    // Helper Functions
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
    
    inline bool ping(const std::string& host) {
        std::string cmd = "ping -c 1 -W 1 " + host + " > /dev/null 2>&1";
        return std::system(cmd.c_str()) == 0;
    }
    
    inline std::string getLocalIP() {
        FILE* pipe = popen("hostname -I | awk '{print $1}'", "r");
        if (!pipe) return "";
        
        char buffer[128];
        std::string result;
        if (fgets(buffer, sizeof(buffer), pipe)) {
            result = buffer;
            if (!result.empty() && result.back() == '\n') {
                result.pop_back();
            }
        }
        pclose(pipe);
        return result;
    }
    
    inline std::string httpGet(const std::string& url) {
        std::string cmd = "curl -s " + url;
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) return "";
        
        std::string result;
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe)) {
            result += buffer;
        }
        pclose(pipe);
        return result;
    }
}

)";
  }
  static std::string generateTopLevel() {
    return R"(// ============================================================================
// Top-level convenience functions
// ============================================================================
inline void print(const std::string& s) { IO::print(s); }
inline void println(const std::string& s) { IO::println(s); }
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

)";
  }
};
