// Auto-generated C++ code from Magolor
// Module-required includes
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <iostream>
#include <string>
#include <algorithm>
#include <functional>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <random>
#include <chrono>
#include <thread>
#include <cmath>
#include <stdexcept>
#include <cstring>
#include <unistd.h>

// =======================================================================
// Auto-generated Standard Library Implementations
// =======================================================================

// =======================================================================
// Std.Network.Http (Auto-generated from stdlib)
// =======================================================================
namespace Http {

    struct Status {
    };

    struct Request {
        std::string method;
        std::string path;
        std::unordered_map<std::string, std::string> headers;
        std::string body;

        inline void create() {
            this->method = "GET";
                        this->path = "/";
                        this->headers = std::unordered_map<std::string, std::string>();
                        this->body = "";
        }
    };

    struct Response {
        int64_t status = 0;
        std::unordered_map<std::string, std::string> headers;
        std::string body;

        inline void create() {
            this->status = 200;
                        this->headers = std::unordered_map<std::string, std::string>();
                        this->headers["Content-Type"] = "text/html; charset=utf-8";
                        this->body = "";
        }

        inline std::string toWire() {
            std::ostringstream ss;
                        
                        // Status line
                        ss << "HTTP/1.1 " << this->status << " ";
                        switch (this->status) {
                            case 200: ss << "OK"; break;
                            case 201: ss << "Created"; break;
                            case 400: ss << "Bad Request"; break;
                            case 404: ss << "Not Found"; break;
                            case 500: ss << "Internal Server Error"; break;
                            default: ss << "Unknown"; break;
                        }
                        ss << "\r\n";
                        
                        // Headers
                        this->headers["Content-Length"] = std::to_string(this->body.length());
                        for (const auto& [key, value] : this->headers) {
                            ss << key << ": " << value << "\r\n";
                        }
                        ss << "\r\n";
                        
                        // Body
                        ss << this->body;
                        
                        return ss.str();
        }
    };

    struct HttpServer {
        int64_t port = 0;

        inline void create(int64_t serverPort) {
            this->port = serverPort;
        }

        inline void start() {
            // Create socket
                        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
                        if (server_fd < 0) {
                            std::cerr << "Failed to create socket\n";
                            return;
                        }
                        
                        // Set socket options
                        int opt = 1;
                        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
                        
                        // Bind
                        struct sockaddr_in addr;
                        addr.sin_family = AF_INET;
                        addr.sin_addr.s_addr = INADDR_ANY;
                        addr.sin_port = htons(this->port);
                        
                        if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
                            std::cerr << "Failed to bind to port " << this->port << "\n";
                            close(server_fd);
                            return;
                        }
                        
                        // Listen
                        if (listen(server_fd, 128) < 0) {
                            std::cerr << "Failed to listen\n";
                            close(server_fd);
                            return;
                        }
                        
                        std::cout << "Server listening on http://localhost:" << this->port << "\n";
                        
                        // Accept loop
                        while (true) {
                            struct sockaddr_in client_addr;
                            socklen_t client_len = sizeof(client_addr);
                            
                            int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
                            if (client_fd < 0) {
                                continue;
                            }
                            
                            // Read request
                            char buffer[8192];
                            ssize_t n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
                            if (n > 0) {
                                buffer[n] = '\0';
                                std::string requestData(buffer, n);
                                
                                // Parse request
                                auto reqOpt = HTTP::parseRequest(requestData);
                                if (reqOpt.has_value()) {
                                    Request req = reqOpt.value();
                                    
                                    // Generate response
                                    Response res;
                                    res.status = 200;
                                    res.body = "<h1>Hello from Magolor!</h1><p>Path: " + req.path + "</p>";
                                    res.headers["Content-Type"] = "text/html";
                                    
                                    // Send response
                                    std::string wire = res.toWire();
                                    send(client_fd, wire.c_str(), wire.length(), 0);
                                }
                            }
                            
                            close(client_fd);
                        }
                        
                        close(server_fd);
        }
    };

    inline Response htmlResponse(const std::string& html, int64_t statusCode) {
        Response res;
                res.status = statusCode;
                res.headers["Content-Type"] = "text/html; charset=utf-8";
                res.body = html;
                return res;
    }

    inline Response jsonResponse(const std::string& json, int64_t statusCode) {
        Response res;
                res.status = statusCode;
                res.headers["Content-Type"] = "application/json";
                res.body = json;
                return res;
    }

    inline Response serveFile(const std::string& path) {
        Response res;
                
                // Read file
                std::ifstream file(path, std::ios::binary);
                if (!file) {
                    res.status = 404;
                    res.headers["Content-Type"] = "text/html";
                    res.body = "<h1>404 - File Not Found</h1>";
                    return res;
                }
                
                std::stringstream buffer;
                buffer << file.rdbuf();
                res.body = buffer.str();
                
                // Determine content type
                std::string ext = path.substr(path.find_last_of('.') + 1);
                if (ext == "html" || ext == "htm") {
                    res.headers["Content-Type"] = "text/html; charset=utf-8";
                } else if (ext == "css") {
                    res.headers["Content-Type"] = "text/css";
                } else if (ext == "js") {
                    res.headers["Content-Type"] = "application/javascript";
                } else if (ext == "json") {
                    res.headers["Content-Type"] = "application/json";
                } else {
                    res.headers["Content-Type"] = "application/octet-stream";
                }
                
                res.status = 200;
                return res;
    }

    inline std::optional<Request> parseRequest(const std::string& data) {
        Request req;
                
                // Split into lines
                std::istringstream stream(data);
                std::string line;
                
                // Parse request line
                if (!std::getline(stream, line)) return std::nullopt;
                if (!line.empty() && line.back() == '\r') line.pop_back();
                
                std::istringstream reqLine(line);
                std::string version;
                reqLine >> req.method >> req.path >> version;
                
                // Parse headers
                while (std::getline(stream, line) && line != "\r" && !line.empty()) {
                    if (line.back() == '\r') line.pop_back();
                    
                    size_t colon = line.find(':');
                    if (colon != std::string::npos) {
                        std::string key = line.substr(0, colon);
                        std::string value = line.substr(colon + 1);
                        
                        // Trim whitespace
                        value.erase(0, value.find_first_not_of(" \t"));
                        value.erase(value.find_last_not_of(" \t") + 1);
                        
                        req.headers[key] = value;
                    }
                }
                
                // Parse body
                std::string bodyData;
                while (std::getline(stream, line)) {
                    bodyData += line + "\n";
                }
                if (!bodyData.empty() && bodyData.back() == '\n') {
                    bodyData.pop_back();
                }
                req.body = bodyData;
                
                return req;
    }

} // namespace Http

// =======================================================================
// Std.IO (Auto-generated from stdlib)
// =======================================================================
namespace IO {

    struct Writer {
        std::string buffer;
        bool autoFlush = false;

        inline void create() {
            this->buffer = "";
                        this->autoFlush = false;
        }

        inline void write(const std::string& s) {
            this->buffer = this->buffer + s;
                        if (this->autoFlush) {
                            std::cout << this->buffer;
                            this->buffer = "";
                        }
        }

        inline void writeLine(const std::string& s) {
            this->buffer = this->buffer + s + "\n";
                        if (this->autoFlush) {
                            std::cout << this->buffer;
                            this->buffer = "";
                        }
        }

        inline void flush() {
            std::cout << this->buffer;
                        this->buffer = "";
        }

        inline std::string toString() {
            return this->buffer;
        }

        inline void clear() {
            this->buffer = "";
        }
    };

    inline void print(const std::string& s) {
        std::cout << s;
    }

    inline void println(const std::string& s) {
        std::cout << s << std::endl;
    }

    inline void printlnEmpty() {
        std::cout << std::endl;
    }

    inline void eprint(const std::string& s) {
        std::cerr << s;
    }

    inline void eprintln(const std::string& s) {
        std::cerr << s << std::endl;
    }

    inline void printInt(int64_t n) {
        std::cout << n;
    }

    inline void printlnInt(int64_t n) {
        std::cout << n << std::endl;
    }

    inline void printFloat(double n) {
        std::cout << n;
    }

    inline void printlnFloat(double n) {
        std::cout << n << std::endl;
    }

    inline void printBool(bool b) {
        std::cout << (b ? "true" : "false");
    }

    inline void printlnBool(bool b) {
        std::cout << (b ? "true" : "false") << std::endl;
    }

    inline std::string readLine() {
        std::string line;
                std::getline(std::cin, line);
                return line;
    }

    inline std::string prompt(const std::string& message) {
        std::cout << message;
                std::string line;
                std::getline(std::cin, line);
                return line;
    }

    inline std::string readAll() {
        std::string content, line;
                while (std::getline(std::cin, line)) {
                    content += line + "\n";
                }
                return content;
    }

    inline std::string readChar() {
        char c;
                std::cin >> c;
                return std::string(1, c);
    }

    inline bool hasInput() {
        return std::cin.peek() != EOF;
    }

    inline std::optional<int64_t> readInt() {
        std::string line;
                std::getline(std::cin, line);
                try {
                    return std::stoll(line);
                } catch (...) {
                    return std::nullopt;
                }
    }

    inline std::optional<double> readFloat() {
        std::string line;
                std::getline(std::cin, line);
                try {
                    return std::stod(line);
                } catch (...) {
                    return std::nullopt;
                }
    }

    inline std::string padLeft(const std::string& s, int64_t width, const std::string& padChar) {
        if (static_cast<int64_t>(s.length()) >= width || padChar.empty()) return s;
                std::string padding;
                int64_t needed = width - s.length();
                for (int64_t i = 0; i < needed; i++) {
                    padding += padChar[0];
                }
                return padding + s;
    }

    inline std::string padRight(const std::string& s, int64_t width, const std::string& padChar) {
        if (static_cast<int64_t>(s.length()) >= width || padChar.empty()) return s;
                std::string result = s;
                int64_t needed = width - s.length();
                for (int64_t i = 0; i < needed; i++) {
                    result += padChar[0];
                }
                return result;
    }

    inline std::string center(const std::string& s, int64_t width, const std::string& padChar) {
        if (static_cast<int64_t>(s.length()) >= width || padChar.empty()) return s;
                int64_t totalPad = width - s.length();
                int64_t leftPad = totalPad / 2;
                int64_t rightPad = totalPad - leftPad;
                std::string left, right;
                for (int64_t i = 0; i < leftPad; i++) left += padChar[0];
                for (int64_t i = 0; i < rightPad; i++) right += padChar[0];
                return left + s + right;
    }

    inline Writer newWriter() {
        Writer w;
                w.buffer = "";
                w.autoFlush = false;
                return w;
    }

} // namespace IO

// =======================================================================
// Standard Library Helpers (generated once)
// =======================================================================
#ifndef MAGOLOR_STDLIB_HELPERS_H
#define MAGOLOR_STDLIB_HELPERS_H

// Template helpers for string conversion
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

// Global Option helpers
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

#endif // MAGOLOR_STDLIB_HELPERS_H

// Array helper wrappers
template<typename T> int length(const ::std::vector<T>& arr) { return arr.size(); }
template<typename T> void push(::std::vector<T>& arr, const T& val) { arr.push_back(val); }
template<typename T> T pop(::std::vector<T>& arr) { auto v = arr.back(); arr.pop_back(); return v; }

// Map helper wrappers
namespace Map {
  template<typename K, typename V> ::std::unordered_map<K,V> create() { return {}; }
  template<typename K, typename V> void insert(::std::unordered_map<K,V>& m, const K& k, const V& v) { m[k] = v; }
  template<typename K, typename V> ::std::optional<V> get(const ::std::unordered_map<K,V>& m, const K& k) {
    auto it = m.find(k); return it != m.end() ? ::std::optional<V>(it->second) : ::std::nullopt;
  }
  template<typename K, typename V> ::std::vector<V> values(const ::std::unordered_map<K,V>& m) {
    ::std::vector<V> r; for(auto& p : m) r.push_back(p.second); return r;
  }
}

// File helper
namespace File {
  inline bool exists(const ::std::string& path) {
    return std::filesystem::exists(path);
  }
  inline bool isFile(const ::std::string& path) {
    return std::filesystem::is_regular_file(path);
  }
  inline bool isDirectory(const ::std::string& path) {
    return std::filesystem::is_directory(path);
  }
  inline std::string absolutePath(const ::std::string& path) {
    return std::filesystem::absolute(path).string();
  }
  inline std::string parentDir(const ::std::string& path) {
    return std::filesystem::path(path).parent_path().string();
  }
  inline std::string fileName(const ::std::string& path) {
    return std::filesystem::path(path).filename().string();
  }
  inline std::string extension(const ::std::string& path) {
    return std::filesystem::path(path).extension().string();
  }
  inline std::string tempDir() {
    return std::filesystem::temp_directory_path().string();
  }
  inline std::optional<std::string> createTempFile(const ::std::string& prefix) {
    std::string dir = std::filesystem::temp_directory_path().string();
    std::string path = dir + "/" + prefix + "_XXXXXX";
    std::vector<char> buf(path.begin(), path.end());
    buf.push_back('\0');
    int fd = mkstemp(buf.data());
    if (fd == -1) return std::nullopt;
    close(fd);
    return std::string(buf.data());
  }
  inline std::string cwd() {
    return std::filesystem::current_path().string();
  }
}

// Global I/O functions
inline void print(const std::string& s) { std::cout << s; }
inline void println(const std::string& s) { std::cout << s << std::endl; }
inline void println() { std::cout << std::endl; }
inline void eprint(const std::string& s) { std::cerr << s; }
inline void eprintln(const std::string& s) { std::cerr << s << std::endl; }
inline std::string readLine() { std::string line; std::getline(std::cin, line); return line; }

// Overloads for common types
template<typename T>
inline void print(const T& val) { std::cout << val; }
template<typename T>
inline void println(const T& val) { std::cout << val << std::endl; }



int main() {
    println(std::string("Starting simple HTTP server..."));
    auto server = HttpServer(8080);
    println(std::string("Server starting on http://localhost:8080"));
    println(std::string("Press Ctrl+C to stop"));
    server.start();
    return 0;
}

