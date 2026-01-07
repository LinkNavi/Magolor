// Force stdlib generation
#include <vector>
#include <unordered_map>
#include <optional>
#include <iostream>
#include <string>

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <algorithm>
#include <functional>
#include <sstream>
#include <fstream>
#include <chrono>
#include <thread>
#include <random>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <numeric>
#include <iomanip>
#include <cstdlib>
#include <unistd.h>

// Template helpers for string conversion
template<typename T>
inline ::std::string mg_to_string(const T& val) { 
    ::std::ostringstream oss; 
    oss << val; 
    return oss.str(); 
}

template<>
inline ::std::string mg_to_string(const bool& val) {
    return val ? "true" : "false";
}

template<>
inline ::std::string mg_to_string(const ::std::string& val) {
    return val;
}

// Global Option helpers
template<typename T>
inline bool isSome(const ::std::optional<T>& opt) { return opt.has_value(); }

template<typename T>
inline bool isNone(const ::std::optional<T>& opt) { return !opt.has_value(); }

template<typename T>
inline T unwrap(const ::std::optional<T>& opt) {
    if (!opt.has_value()) {
        throw ::std::runtime_error("Called unwrap on None value");
    }
    return opt.value();
}

template<typename T>
inline T unwrapOr(const ::std::optional<T>& opt, const T& defaultValue) {
    return opt.value_or(defaultValue);
}

namespace Std {

#include <iostream>
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
#include <numeric>
#include <regex>


// Template helpers for string conversion
template<typename T>
inline ::std::string mg_to_string(const T& val) { 
    ::std::ostringstream oss; 
    oss << val; 
    return oss.str(); 
}

template<>
inline ::std::string mg_to_string(const bool& val) {
    return val ? "true" : "false";
}

template<>
inline ::std::string mg_to_string(const ::std::string& val) {
    return val;
}

// Global Option helpers
template<typename T>
inline bool isSome(const ::std::optional<T>& opt) { return opt.has_value(); }

template<typename T>
inline bool isNone(const ::std::optional<T>& opt) { return !opt.has_value(); }

template<typename T>
inline T unwrap(const ::std::optional<T>& opt) {
    if (!opt.has_value()) {
        throw ::std::runtime_error("Called unwrap on None value");
    }
    return opt.value();
}

template<typename T>
inline T unwrapOr(const ::std::optional<T>& opt, const T& defaultValue) {
    return opt.value_or(defaultValue);
}

namespace Std {

// Module not found: Std.Core.Prelude
// Module not found: Std.Random
// Module not found: Std.File
// Module not found: Std.Math
// Module not found: Std.System
// Module not found: Std.Time
// Module not found: Std.Map
// Module not found: Std.Array
// Module not found: Std.String
// Module not found: Std.IO

// Convenience functions at Std level
template<typename T>
inline void print(const T& val) { ::std::cout << mg_to_string(val); }

template<typename T>
inline void println(const T& val) { ::std::cout << mg_to_string(val) << ::std::endl; }

inline void print(const ::std::string& s) { ::std::cout << s; }
inline void println(const ::std::string& s) { ::std::cout << s << ::std::endl; }

inline ::std::string readLine() { 
    ::std::string line; 
    ::std::getline(::std::cin, line); 
    return line; 
}

inline ::std::string toString(int v) { return ::std::to_string(v); }
inline ::std::string toString(double v) { return ::std::to_string(v); }
inline ::std::string toString(bool v) { return v ? "true" : "false"; }

} // namespace Std

using Std::println;
using Std::print;
using Std::readLine;

} // namespace Std

using Std::println;
using Std::print;
using Std::readLine;
using Std::toString;

// Import Std namespace for convenience
using Std::println;
using Std::print;
using Std::readLine;

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
    ::std::ifstream f(path); return f.good();
  }
}



int main() {
    Std::print(std::string("╔════════════════════════════════════════╗\n"));
    Std::print(std::string("║  Magolor Web Framework Demo           ║\n"));
    Std::print(std::string("║  Cross-Platform HTTP Server            ║\n"));
    Std::print(std::string("╚════════════════════════════════════════╝\n\n"));
    auto server = Std::Network::HttpServer(3000);
    server.use(Std::Network::corsMiddleware(std::string("*")));
    server.use(Std::Network::loggerMiddleware());
    server.use([=](auto req, auto res) {
        if (((req.path == std::string("/")) || (req.path == std::string("/login")))) {
            return true;
        }
        auto apiKey = req.getHeader(std::string("X-API-Key"));
        if ((apiKey == std::string("secret123"))) {
            return true;
        }
        res.statusCode = Std::Network::Status::UNAUTHORIZED;
        res.body = std::string("{\"error\": \"Unauthorized\"}");
        res.setJson();
        return false;
    });
    server.get(std::string("/"), [=](auto req) {
        auto html = std::string("<!DOCTYPE html>\n<html>\n<head>\n    <title>Magolor Web Demo</title>\n    <style>\n        body { \n            font-family: Arial, sans-serif; \n            max-width: 800px; \n            margin: 40px auto; \n            padding: 20px;\n            background: #f5f5f5;\n        }\n        .card {\n            background: white;\n            padding: 20px;\n            margin: 20px 0;\n            border-radius: 8px;\n            box-shadow: 0 2px 4px rgba(0,0,0,0.1);\n        }\n        h1 { color: #333; }\n        code { \n            background: #f0f0f0; \n            padding: 2px 6px; \n            border-radius: 3px;\n        }\n        .endpoint {\n            margin: 10px 0;\n            padding: 10px;\n            background: #f8f8f8;\n            border-left: 4px solid #4CAF50;\n        }\n    </style>\n</head>\n<body>\n    <h1>🚀 Magolor Web Framework</h1>\n    <div class='card'>\n        <h2>Available Endpoints</h2>\n        \n        <div class='endpoint'>\n            <strong>GET /</strong> - This page\n        </div>\n        \n        <div class='endpoint'>\n            <strong>GET /api/hello</strong> - JSON API example\n        </div>\n        \n        <div class='endpoint'>\n            <strong>GET /greet?name=YourName</strong> - Query parameters\n        </div>\n        \n        <div class='endpoint'>\n            <strong>POST /api/data</strong> - Form data handling\n        </div>\n        \n        <div class='endpoint'>\n            <strong>GET /cookie-demo</strong> - Cookie management\n        </div>\n        \n        <div class='endpoint'>\n            <strong>GET /session-demo</strong> - Session management\n        </div>\n        \n        <div class='endpoint'>\n            <strong>GET /json-builder</strong> - JSON builder demo\n        </div>\n        \n        <div class='endpoint'>\n            <strong>GET /redirect</strong> - Redirect example\n        </div>\n        \n        <div class='endpoint'>\n            <strong>GET /files/[path]</strong> - Static file serving\n        </div>\n    </div>\n    \n    <div class='card'>\n        <h2>Features</h2>\n        <ul>\n            <li>✅ Cross-platform (Windows, Linux, macOS)</li>\n            <li>✅ HTTP/1.1 with all methods</li>\n            <li>✅ Cookie management</li>\n            <li>✅ Session management</li>\n            <li>✅ Middleware system</li>\n            <li>✅ CORS support</li>\n            <li>✅ JSON builder</li>\n            <li>✅ Form data parsing</li>\n            <li>✅ Static file serving</li>\n            <li>✅ Query string parsing</li>\n            <li>✅ URL encoding/decoding</li>\n        </ul>\n    </div>\n</body>\n</html>");
        return Std::Network::htmlResponse(html);
    });
    server.get(std::string("/api/hello"), [=](auto req) {
        auto json = Std::Network::JsonBuilder();
        json.add(std::string("message"), std::string("Hello from Magolor!"));
        json.add(std::string("version"), std::string("1.0"));
        json.add(std::string("timestamp"), 1234567890);
        json.add(std::string("success"), true);
        return Std::Network::jsonResponse(json.build());
    });
    server.get(std::string("/greet"), [=](auto req) {
        auto name = req.getQuery(std::string("name"));
        if ((name == std::string(""))) {
            return Std::Network::htmlResponse(std::string("<h1>Hello, Stranger!</h1>"));
        }
        return Std::Network::htmlResponse((std::string("<h1>Hello, ") + mg_to_string(name) + std::string("!</h1>")));
    });
    server.post(std::string("/api/data"), [=](auto req) {
        if (req.isForm()) {
            auto username = req.getForm(std::string("username"));
            auto email = req.getForm(std::string("email"));
            auto json = Std::Network::JsonBuilder();
            json.add(std::string("received"), std::string("form data"));
            json.add(std::string("username"), username);
            json.add(std::string("email"), email);
            return Std::Network::jsonResponse(json.build(), Std::Network::Status::CREATED);
        }
        if (req.isJson()) {
            auto json = Std::Network::JsonBuilder();
            json.add(std::string("received"), std::string("json data"));
            json.add(std::string("bodyLength"), 100);
            return Std::Network::jsonResponse(json.build());
        }
        return Std::Network::errorResponse(Std::Network::Status::BAD_REQUEST, std::string("Expected form data or JSON"));
    });
    server.get(std::string("/cookie-demo"), [=](auto req) {
        auto res = Std::Network::HttpResponse();
        auto visits = req.getCookie(std::string("visits"));
        auto cookie = Std::Network::Cookie();
        cookie.name = std::string("visits");
        cookie.value = std::string("1");
        cookie.maxAge = 3600;
        cookie.httpOnly = true;
        cookie.secure = false;
        cookie.sameSite = std::string("Lax");
        res.setCookie(cookie);
        if ((visits == std::string(""))) {
            res.body = std::string("<h1>Welcome! This is your first visit.</h1>");
        }
        else {
            res.body = (std::string("<h1>Welcome back! Visit count: ") + mg_to_string(visits) + std::string("</h1>"));
        }
        return res;
    });
    server.get(std::string("/session-demo"), [=](auto req) {
        auto sessionStore = server.getSessionStore();
        auto res = Std::Network::HttpResponse();
        auto sessionId = req.getCookie(std::string("session_id"));
        if (((sessionId == std::string("")) || (!sessionStore.exists(sessionId)))) {
            sessionId = sessionStore.create();
            sessionStore.set(sessionId, std::string("user"), std::string("guest"));
            sessionStore.set(sessionId, std::string("login_time"), std::string("2024-01-01"));
            auto cookie = Std::Network::Cookie();
            cookie.name = std::string("session_id");
            cookie.value = sessionId;
            cookie.httpOnly = true;
            res.setCookie(cookie);
            res.body = std::string("<h1>New session created!</h1>");
        }
        else {
            auto user = sessionStore.get(sessionId, std::string("user"));
            auto loginTime = sessionStore.get(sessionId, std::string("login_time"));
            res.body = (std::string("<h1>Welcome back, ") + mg_to_string(user) + std::string("!</h1><p>Login time: ") + mg_to_string(loginTime) + std::string("</p>"));
        }
        return res;
    });
    server.get(std::string("/json-builder"), [=](auto req) {
        auto json = Std::Network::JsonBuilder();
        json.add(std::string("status"), std::string("success"));
        json.add(std::string("code"), 200);
        json.add(std::string("pi"), 3.141590);
        json.add(std::string("active"), true);
        json.add(std::string("message"), std::string("JsonBuilder makes JSON easy!"));
        return Std::Network::jsonResponse(json.build());
    });
    server.get(std::string("/redirect"), [=](auto req) {
        return Std::Network::redirectResponse(std::string("/"), Std::Network::Status::TEMPORARY_REDIRECT);
    });
    server.get(std::string("/files/test.html"), [=](auto req) {
        return Std::Network::serveFile(std::string("test.html"));
    });
    server.get(std::string("/data"), [=](auto req) {
        if (req.acceptsJson()) {
            auto json = Std::Network::JsonBuilder();
            json.add(std::string("format"), std::string("json"));
            json.add(std::string("value"), 42);
            return Std::Network::jsonResponse(json.build());
        }
        if (req.acceptsHtml()) {
            return Std::Network::htmlResponse(std::string("<h1>Data: 42</h1>"));
        }
        return Std::Network::textResponse(std::string("Data: 42"));
    });
    server.get(std::string("/error"), [=](auto req) {
        return Std::Network::errorResponse(Std::Network::Status::INTERNAL_SERVER_ERROR, std::string("This is a demo error"));
    });
    server.setNotFound([=](auto req) {
        auto html = ((std::string("<!DOCTYPE html>\n<html>\n<head>\n    <title>404 Not Found</title>\n    <style>\n        body { \n            font-family: Arial, sans-serif; \n            text-align: center; \n            padding: 50px;\n            background: #f5f5f5;\n        }\n        h1 { color: #e74c3c; }\n        a { color: #3498db; }\n    </style>\n</head>\n<body>\n    <h1>😕 404 - Page Not Found</h1>\n    <p>The page you're looking for doesn't exist.</p>\n    <p>Path: ") + req.path) + std::string("</p>\n    <p><a href='/'>← Go Home</a></p>\n</body>\n</html>"));
        return Std::Network::htmlResponse(html, Std::Network::Status::NOT_FOUND);
    });
    Std::print(std::string("\n🌐 Starting server...\n\n"));
    Std::print(std::string("Try these URLs:\n"));
    Std::print(std::string("  http://localhost:3000/\n"));
    Std::print(std::string("  http://localhost:3000/api/hello\n"));
    Std::print(std::string("  http://localhost:3000/greet?name=Alice\n"));
    Std::print(std::string("  http://localhost:3000/cookie-demo\n"));
    Std::print(std::string("  http://localhost:3000/session-demo\n"));
    Std::print(std::string("\nPress Ctrl+C to stop.\n\n"));
    server.start();
    return 0;
}

