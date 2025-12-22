#include "stdlib_manager.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <regex>
#include <cstring>

// Get all available stdlib modules
std::vector<std::string> StdLibManager::getAvailableModules() {
    return {
        "IO", "Parse", "Option", "Math", "String",
        "Array", "Map", "Set", "File", "Time",
        "Random", "System"
    };
}

// Get raw source for a specific module (from stdlib.hpp generators)
std::string StdLibManager::getModuleSource(const std::string& moduleName) {
    if (moduleName == "IO") {
        return R"(namespace IO {
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
})";
    } else if (moduleName == "Math") {
        return R"(namespace Math {
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
})";
    } else if (moduleName == "String") {
        return R"(namespace String {
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
})";
    }
    
    return "// Module not found\n";
}

// Extract a module to an editable file
bool StdLibManager::extractModule(const std::string& moduleName, const std::string& outputFile) {
    std::string source = getModuleSource(moduleName);
    
    if (source.find("not found") != std::string::npos) {
        std::cerr << "Module '" << moduleName << "' not found\n";
        return false;
    }
    
    std::ofstream out(outputFile);
    if (!out) {
        std::cerr << "Failed to create output file: " << outputFile << "\n";
        return false;
    }
    
    // Write header with instructions
    out << "# Magolor StdLib Module: Std." << moduleName << "\n";
    out << "# ============================================================================\n";
    out << "# This file contains the C++ implementation for the " << moduleName << " module.\n";
    out << "# \n";
    out << "# EDITING INSTRUCTIONS:\n";
    out << "# 1. Edit the C++ code below as needed\n";
    out << "# 2. Add new functions using 'inline' keyword\n";
    out << "# 3. Keep the namespace structure intact\n";
    out << "# 4. Run: magolor stdlib import " << outputFile << "\n";
    out << "# \n";
    out << "# IMPORTANT:\n";
    out << "# - All functions must be 'inline' or 'constexpr'\n";
    out << "# - Template functions should include full implementation\n";
    out << "# - Avoid dependencies outside the namespace\n";
    out << "# ============================================================================\n\n";
    
    out << "// BEGIN MODULE CODE\n\n";
    out << source;
    out << "\n\n// END MODULE CODE\n";
    
    out.close();
    
    std::cout << "✓ Extracted Std." << moduleName << " to " << outputFile << "\n";
    std::cout << "\nNext steps:\n";
    std::cout << "  1. Edit " << outputFile << "\n";
    std::cout << "  2. Run: magolor stdlib import " << outputFile << " > new_code.cpp\n";
    std::cout << "  3. Copy the output into stdlib.hpp\n";
    
    return true;
}

// Parse a module file
StdLibModule StdLibManager::parseModuleFile(const std::string& content) {
    StdLibModule module;
    
    // Extract module name from header
    std::regex nameRegex(R"(# Magolor StdLib Module: Std\.(\w+))");
    std::smatch match;
    if (std::regex_search(content, match, nameRegex)) {
        module.name = match[1].str();
    }
    
    // Extract code between markers
    size_t beginPos = content.find("// BEGIN MODULE CODE");
    size_t endPos = content.find("// END MODULE CODE");
    
    if (beginPos != std::string::npos && endPos != std::string::npos) {
        beginPos += strlen("// BEGIN MODULE CODE");
        module.rawCode = content.substr(beginPos, endPos - beginPos);
        
        // Trim whitespace
        module.rawCode.erase(0, module.rawCode.find_first_not_of(" \t\n\r"));
        module.rawCode.erase(module.rawCode.find_last_not_of(" \t\n\r") + 1);
    } else {
        // No markers, use entire content
        module.rawCode = content;
    }
    
    return module;
}

// Format code for stdlib.hpp inclusion
std::string StdLibManager::formatCode(const std::string& code) {
    std::stringstream ss(code);
    std::stringstream out;
    std::string line;
    
    while (std::getline(ss, line)) {
        // Skip comment lines at the start
        if (line.empty() || line[0] == '#') continue;
        
        out << line << "\n";
    }
    
    return out.str();
}

// Generate module code in stdlib.hpp format
std::string StdLibManager::generateModuleCode(const StdLibModule& module) {
    std::stringstream out;
    
    out << "// ============================================================================\n";
    out << "// Std." << module.name << " - " << module.comment << "\n";
    out << "// ============================================================================\n";
    
    out << module.rawCode;
    
    out << "\n";
    
    return out.str();
}

// Import a module file back into stdlib format
std::string StdLibManager::importModule(const std::string& inputFile) {
    std::ifstream in(inputFile);
    if (!in) {
        std::cerr << "Failed to open input file: " << inputFile << "\n";
        return "";
    }
    
    std::stringstream buffer;
    buffer << in.rdbuf();
    std::string content = buffer.str();
    
    StdLibModule module = parseModuleFile(content);
    
    if (module.name.empty()) {
        std::cerr << "Warning: Could not determine module name\n";
        module.name = "Custom";
    }
    
    return generateModuleCode(module);
}

// Create a new custom module template
bool StdLibManager::createModuleTemplate(const std::string& moduleName, const std::string& outputFile) {
    std::ofstream out(outputFile);
    if (!out) {
        std::cerr << "Failed to create output file: " << outputFile << "\n";
        return false;
    }
    
    out << "# Magolor StdLib Module: Std." << moduleName << "\n";
    out << "# ============================================================================\n";
    out << "# Custom module template\n";
    out << "# \n";
    out << "# USAGE:\n";
    out << "# 1. Add your functions below\n";
    out << "# 2. Run: magolor stdlib import " << outputFile << " > module_code.cpp\n";
    out << "# 3. Add the generated code to stdlib.hpp in the appropriate section\n";
    out << "# 4. Update stdlib.hpp's generateAll() to call your generator\n";
    out << "# 5. Update lsp_completion.cpp to add autocomplete support\n";
    out << "# \n";
    out << "# EXAMPLE FUNCTIONS:\n";
    out << "# inline int myFunction(int x) { return x * 2; }\n";
    out << "# inline std::string formatData(const std::string& s) { return \"[\" + s + \"]\"; }\n";
    out << "# ============================================================================\n\n";
    
    out << "// BEGIN MODULE CODE\n\n";
    out << "namespace " << moduleName << " {\n";
    out << "    // Add your functions here\n";
    out << "    \n";
    out << "    inline int example(int x) {\n";
    out << "        return x * 2;\n";
    out << "    }\n";
    out << "}\n";
    out << "\n// END MODULE CODE\n";
    
    out.close();
    
    std::cout << "✓ Created template for Std." << moduleName << " at " << outputFile << "\n";
    std::cout << "\nNext steps:\n";
    std::cout << "  1. Edit " << outputFile << " and add your functions\n";
    std::cout << "  2. Run: magolor stdlib import " << outputFile << " > module_code.cpp\n";
    std::cout << "  3. Add generated code to stdlib.hpp\n";
    std::cout << "  4. Update stdlib.hpp generateAll() method\n";
    std::cout << "  5. Add autocomplete support in lsp_completion.cpp\n";
    
    return true;
}
