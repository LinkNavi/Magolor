#pragma once
#include "stdlib_loader.hpp"
#include <string>
#include <sstream>
#include <unordered_set>

// Standard Library Code Generator
// Now integrated with stdlib_loader to use actual .mg files

class StdLibGenerator {
public:
    static std::string generateAll() {
        std::stringstream ss;
        
        // Include necessary C++ headers
        ss << "#include <iostream>\n";
        ss << "#include <string>\n";
        ss << "#include <vector>\n";
        ss << "#include <unordered_map>\n";
        ss << "#include <unordered_set>\n";
        ss << "#include <optional>\n";
        ss << "#include <algorithm>\n";
        ss << "#include <functional>\n";
        ss << "#include <sstream>\n";
        ss << "#include <fstream>\n";
        ss << "#include <chrono>\n";
        ss << "#include <thread>\n";
        ss << "#include <random>\n";
        ss << "#include <cmath>\n";
        ss << "#include <ctime>\n";
        ss << "#include <filesystem>\n";
        ss << "#include <numeric>\n";
        ss << "#include <iomanip>\n";
        ss << "#include <cstdlib>\n";
        ss << "#include <unistd.h>\n";
        ss << "\n";
        
        // Helper template for string conversion
        ss << "// Template helpers for string conversion\n";
        ss << "template<typename T>\n";
        ss << "inline std::string mg_to_string(const T& val) { \n";
        ss << "    std::ostringstream oss; \n";
        ss << "    oss << val; \n";
        ss << "    return oss.str(); \n";
        ss << "}\n\n";
        
        ss << "template<>\n";
        ss << "inline std::string mg_to_string(const bool& val) {\n";
        ss << "    return val ? \"true\" : \"false\";\n";
        ss << "}\n\n";
        
        ss << "template<>\n";
        ss << "inline std::string mg_to_string(const std::string& val) {\n";
        ss << "    return val;\n";
        ss << "}\n\n";
        
        // Global Option helpers
        ss << "// Global Option helpers\n";
        ss << "template<typename T>\n";
        ss << "inline bool isSome(const std::optional<T>& opt) { return opt.has_value(); }\n\n";
        
        ss << "template<typename T>\n";
        ss << "inline bool isNone(const std::optional<T>& opt) { return !opt.has_value(); }\n\n";
        
        ss << "template<typename T>\n";
        ss << "inline T unwrap(const std::optional<T>& opt) {\n";
        ss << "    if (!opt.has_value()) {\n";
        ss << "        throw std::runtime_error(\"Called unwrap on None value\");\n";
        ss << "    }\n";
        ss << "    return opt.value();\n";
        ss << "}\n\n";
        
        ss << "template<typename T>\n";
        ss << "inline T unwrapOr(const std::optional<T>& opt, const T& defaultValue) {\n";
        ss << "    return opt.value_or(defaultValue);\n";
        ss << "}\n\n";
        
        // Try to load stdlib from files
        auto& loader = StdLibLoader::instance();
        
        // Try common stdlib locations
        std::vector<std::string> searchPaths = {
            "./stdlib",
            "/usr/local/share/magolor/stdlib",
            "/usr/share/magolor/stdlib",
            std::string(getenv("HOME") ? getenv("HOME") : "") + "/.magolor/stdlib"
        };
        
        bool initialized = false;
        for (const auto& path : searchPaths) {
            if (std::filesystem::exists(path)) {
                loader.init(path);
                initialized = true;
                break;
            }
        }
        
        // Start Std namespace
        ss << "namespace Std {\n\n";
        
        if (initialized) {
            // Generate from loaded .mg files
            std::unordered_set<std::string> usedModules = {
                "Std.Core.Prelude",
                "Std.IO",
                "Std.String",
                "Std.Array",
                "Std.Map",
                "Std.Math",
                "Std.File",
                "Std.Time",
                "Std.Random",
                "Std.System"
            };
            
            ss << StdLibCodeGen::generateAll(usedModules);
        } else {
            // Fallback: minimal core functions if stdlib files not found
            ss << "// WARNING: stdlib files not found, using minimal fallback\n\n";
            
            ss << "// Core I/O functions\n";
            ss << "template<typename T>\n";
            ss << "inline void print(const T& val) { std::cout << mg_to_string(val); }\n\n";
            
            ss << "template<typename T>\n";
            ss << "inline void println(const T& val) { std::cout << mg_to_string(val) << std::endl; }\n\n";
            
            ss << "inline void print(const std::string& s) { std::cout << s; }\n";
            ss << "inline void println(const std::string& s) { std::cout << s << std::endl; }\n\n";
            
            ss << "inline std::string readLine() { \n";
            ss << "    std::string line; \n";
            ss << "    std::getline(std::cin, line); \n";
            ss << "    return line; \n";
            ss << "}\n\n";
            
            // Type conversion helpers
            ss << "inline std::string toString(int v) { return std::to_string(v); }\n";
            ss << "inline std::string toString(double v) { return std::to_string(v); }\n";
            ss << "inline std::string toString(bool v) { return v ? \"true\" : \"false\"; }\n";
            ss << "inline int toInt(double v) { return static_cast<int>(v); }\n";
            ss << "inline double toFloat(int v) { return static_cast<double>(v); }\n\n";
        }
        
        ss << "} // namespace Std\n\n";
        
        // Global using declarations
        ss << "using Std::println;\n";
        ss << "using Std::print;\n";
        ss << "using Std::readLine;\n";
        ss << "using Std::toString;\n\n";
        
        return ss.str();
    }
};
