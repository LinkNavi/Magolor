#include "stdlib_loader.hpp"
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>

void StdLibLoader::init(const std::string& path) {
    if (initialized) return;
    
    stdlibPath = path;
    
    // Try multiple possible locations
    std::vector<std::string> searchPaths = {
        path,
        "/usr/local/share/magolor/stdlib",
        "/usr/share/magolor/stdlib",
        std::string(getenv("HOME") ? getenv("HOME") : "") + "/.magolor/stdlib",
        "./stdlib"
    };
    
    for (const auto& searchPath : searchPaths) {
        if (fs::exists(searchPath) && fs::is_directory(searchPath)) {
            stdlibPath = searchPath;
            break;
        }
    }
    
    discoverModules();
    initialized = true;
}

void StdLibLoader::discoverModules() {
    if (!fs::exists(stdlibPath)) {
        return;
    }
    
    // Scan stdlib directories
    for (const auto& entry : fs::recursive_directory_iterator(stdlibPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".mg") {
            std::string filePath = entry.path().string();
            std::string relPath = fs::relative(entry.path(), stdlibPath).string();
            
            // Convert path to module name: core/prelude.mg -> Std.Core.Prelude
            std::string moduleName = "Std";
            std::string pathCopy = relPath;
            
            // Remove .mg extension
            if (pathCopy.size() > 3) {
                pathCopy = pathCopy.substr(0, pathCopy.size() - 3);
            }
            
            // Replace path separators with dots and capitalize
            for (size_t i = 0; i < pathCopy.size(); i++) {
                if (pathCopy[i] == '/' || pathCopy[i] == '\\') {
                    moduleName += ".";
                    if (i + 1 < pathCopy.size()) {
                        pathCopy[i + 1] = std::toupper(pathCopy[i + 1]);
                    }
                } else if (i == 0 || pathCopy[i - 1] == '/' || pathCopy[i - 1] == '\\') {
                    moduleName += std::toupper(pathCopy[i]);
                } else {
                    moduleName += pathCopy[i];
                }
            }
            
            StdModule module;
            module.name = moduleName.substr(moduleName.rfind('.') + 1);
            module.fullPath = moduleName;
            module.filePath = filePath;
            
            modules[moduleName] = module;
            
            // Also register short names (e.g., "Std.IO" as well as full path)
            if (moduleName.find('.') != std::string::npos) {
                std::string shortName = "Std." + module.name;
                if (modules.find(shortName) == modules.end()) {
                    modules[shortName] = module;
                }
            }
        }
    }
}

StdModule* StdLibLoader::loadModule(const std::string& modulePath) {
    auto it = modules.find(modulePath);
    if (it == modules.end()) {
        return nullptr;
    }
    
    StdModule& module = it->second;
    
    if (!module.loaded) {
        parseModuleFile(module);
        module.loaded = true;
    }
    
    return &module;
}

bool StdLibLoader::hasModule(const std::string& modulePath) const {
    return modules.find(modulePath) != modules.end();
}

std::vector<std::string> StdLibLoader::getAvailableModules() const {
    std::vector<std::string> result;
    std::unordered_set<std::string> seen;
    
    for (const auto& [path, _] : modules) {
        // Only return Std.X style paths
        if (std::count(path.begin(), path.end(), '.') == 1) {
            if (seen.insert(path).second) {
                result.push_back(path);
            }
        }
    }
    
    std::sort(result.begin(), result.end());
    return result;
}

void StdLibLoader::parseModuleFile(StdModule& module) {
    std::ifstream file(module.filePath);
    if (!file) return;
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    
    extractFunctions(source, module);
    extractClasses(source, module);
    
    module.parsed = true;
}

void StdLibLoader::extractFunctions(const std::string& source, StdModule& module) {
    // Regex for pub fn name(params) -> returnType
    std::regex funcRegex(R"(pub\s+fn\s+(\w+)\s*\(([^)]*)\)\s*(?:->\s*([^\{]+))?)");
    
    auto begin = std::sregex_iterator(source.begin(), source.end(), funcRegex);
    auto end = std::sregex_iterator();
    
    for (auto it = begin; it != end; ++it) {
        StdFunction func;
        func.name = (*it)[1].str();
        std::string params = (*it)[2].str();
        func.returnType = (*it)[3].matched ? (*it)[3].str() : "void";
        
        // Trim return type
        func.returnType.erase(0, func.returnType.find_first_not_of(" \t\n"));
        func.returnType.erase(func.returnType.find_last_not_of(" \t\n") + 1);
        
        // Parse parameters
        if (!params.empty()) {
            std::regex paramRegex(R"((\w+)\s*:\s*([^,]+))");
            auto pBegin = std::sregex_iterator(params.begin(), params.end(), paramRegex);
            auto pEnd = std::sregex_iterator();
            
            std::string sig = "(";
            bool first = true;
            
            for (auto pit = pBegin; pit != pEnd; ++pit) {
                func.paramNames.push_back((*pit)[1].str());
                std::string ptype = (*pit)[2].str();
                ptype.erase(0, ptype.find_first_not_of(" \t"));
                ptype.erase(ptype.find_last_not_of(" \t") + 1);
                func.paramTypes.push_back(ptype);
                
                if (!first) sig += ", ";
                first = false;
                sig += (*pit)[1].str() + ": " + ptype;
            }
            
            sig += ") -> " + func.returnType;
            func.signature = sig;
        } else {
            func.signature = "() -> " + func.returnType;
        }
        
        module.functions.push_back(func);
        functionToModule[func.name] = module.fullPath;
        allFunctions[func.name] = func;
    }
    
    // Also look for pub static constants
    std::regex constRegex(R"(pub\s+static\s+(\w+)\s*:\s*(\w+)\s*=)");
    begin = std::sregex_iterator(source.begin(), source.end(), constRegex);
    
    for (auto it = begin; it != end; ++it) {
        StdFunction func;
        func.name = (*it)[1].str();
        func.returnType = (*it)[2].str();
        func.isConstant = true;
        func.signature = func.name + ": " + func.returnType;
        
        module.functions.push_back(func);
        module.constants.push_back(func.name);
        functionToModule[func.name] = module.fullPath;
        allFunctions[func.name] = func;
    }
}

void StdLibLoader::extractClasses(const std::string& source, StdModule& module) {
    std::regex classRegex(R"(pub\s+class\s+(\w+)\s*\{)");
    
    auto begin = std::sregex_iterator(source.begin(), source.end(), classRegex);
    auto end = std::sregex_iterator();
    
    for (auto it = begin; it != end; ++it) {
        StdClass cls;
        cls.name = (*it)[1].str();
        module.classes.push_back(cls);
    }
}

bool StdLibLoader::isStdFunction(const std::string& funcName) const {
    return allFunctions.find(funcName) != allFunctions.end();
}

std::string StdLibLoader::getModuleForFunction(const std::string& funcName) const {
    auto it = functionToModule.find(funcName);
    return it != functionToModule.end() ? it->second : "";
}

std::string StdLibLoader::getFunctionSignature(const std::string& funcName) const {
    auto it = allFunctions.find(funcName);
    return it != allFunctions.end() ? it->second.signature : "";
}

std::vector<StdFunction> StdLibLoader::getFunctions(const std::string& modulePath) {
    auto* module = loadModule(modulePath);
    if (!module) return {};
    return module->functions;
}

std::vector<StdClass> StdLibLoader::getClasses(const std::string& modulePath) {
    auto* module = loadModule(modulePath);
    if (!module) return {};
    return module->classes;
}

TypePtr StdLibLoader::getReturnType(const std::string& funcName) const {
    auto it = allFunctions.find(funcName);
    if (it == allFunctions.end()) return nullptr;
    
    const std::string& rt = it->second.returnType;
    auto type = std::make_shared<Type>();
    
    if (rt == "int") type->kind = Type::INT;
    else if (rt == "float") type->kind = Type::FLOAT;
    else if (rt == "string") type->kind = Type::STRING;
    else if (rt == "bool") type->kind = Type::BOOL;
    else if (rt == "void") type->kind = Type::VOID;
    else if (rt.find("Option<") == 0) {
        type->kind = Type::OPTION;
        type->innerType = std::make_shared<Type>();
        // Parse inner type (simplified)
        std::string inner = rt.substr(7, rt.size() - 8);
        if (inner == "int") type->innerType->kind = Type::INT;
        else if (inner == "string") type->innerType->kind = Type::STRING;
        else type->innerType->kind = Type::VOID;
    }
    else if (rt.find("Array<") == 0) {
        type->kind = Type::ARRAY;
        type->innerType = std::make_shared<Type>();
        type->innerType->kind = Type::VOID;
    }
    else {
        type->kind = Type::CLASS;
        type->className = rt;
    }
    
    return type;
}

// ============================================================================
// Code Generation
// ============================================================================

std::string StdLibCodeGen::generateIncludes() {
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
#include <numeric>
#include <regex>

)";
}

std::string StdLibCodeGen::generateHelpers() {
    return R"(
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

)";
}

std::string StdLibCodeGen::generateAll(const std::unordered_set<std::string>& usedModules) {
    std::stringstream ss;
    
    ss << generateIncludes();
    ss << generateHelpers();
    ss << generateNamespace(usedModules);
    
    return ss.str();
}

std::string StdLibCodeGen::generateNamespace(const std::unordered_set<std::string>& usedModules) {
    std::stringstream ss;
    
    ss << "namespace Std {\n\n";
    
    // Always include core
    ss << generateModuleCode("Std.Core.Prelude");
    
    // Generate code for each used module
    for (const auto& mod : usedModules) {
        if (mod != "Std.Core.Prelude") {
            ss << generateModuleCode(mod);
        }
    }
    
    // Add convenience using declarations
    ss << R"(
// Convenience functions at Std level
template<typename T>
inline void print(const T& val) { std::cout << mg_to_string(val); }

template<typename T>
inline void println(const T& val) { std::cout << mg_to_string(val) << std::endl; }

inline void print(const std::string& s) { std::cout << s; }
inline void println(const std::string& s) { std::cout << s << std::endl; }

inline std::string readLine() { 
    std::string line; 
    std::getline(std::cin, line); 
    return line; 
}

inline std::string toString(int v) { return std::to_string(v); }
inline std::string toString(double v) { return std::to_string(v); }
inline std::string toString(bool v) { return v ? "true" : "false"; }

)";
    
    ss << "} // namespace Std\n\n";
    
    // Add global using declarations for common functions
    ss << "using Std::println;\n";
    ss << "using Std::print;\n";
    ss << "using Std::readLine;\n\n";
    
    return ss.str();
}

std::string StdLibCodeGen::generateModuleCode(const std::string& modulePath) {
    auto& loader = StdLibLoader::instance();
    auto* module = loader.loadModule(modulePath);
    
    if (!module || module->filePath.empty()) {
        return "// Module not found: " + modulePath + "\n";
    }
    
    // Read the source file
    std::ifstream file(module->filePath);
    if (!file) {
        return "// Could not read: " + module->filePath + "\n";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    
    std::stringstream ss;
    ss << "// " << std::string(70, '=') << "\n";
    ss << "// " << modulePath << "\n";
    ss << "// " << std::string(70, '=') << "\n";
    ss << "namespace " << module->name << " {\n\n";
    
    // Extract and generate each function's @cpp block
    for (const auto& func : module->functions) {
        std::string cppCode = extractCppCode(source, func.name);
        
        if (func.isConstant) {
            // Generate constant
            ss << "    constexpr ";
            if (func.returnType == "int") ss << "int";
            else if (func.returnType == "float") ss << "double";
            else ss << func.returnType;
            ss << " " << func.name << " = ";
            
            // Find the value from source
            std::regex constRegex(func.name + R"(\s*:\s*\w+\s*=\s*([^;]+))");
            std::smatch match;
            if (std::regex_search(source, match, constRegex)) {
                ss << match[1].str();
            } else {
                ss << "0";
            }
            ss << ";\n";
        } else if (!cppCode.empty()) {
            // Generate function with @cpp block
            ss << "    inline ";
            
            // Return type
            if (func.returnType == "int") ss << "int";
            else if (func.returnType == "float") ss << "double";
            else if (func.returnType == "string") ss << "std::string";
            else if (func.returnType == "bool") ss << "bool";
            else if (func.returnType == "void") ss << "void";
            else if (func.returnType.find("Option<") == 0) {
                std::string inner = func.returnType.substr(7, func.returnType.size() - 8);
                ss << "std::optional<";
                if (inner == "int") ss << "int";
                else if (inner == "string") ss << "std::string";
                else if (inner == "float") ss << "double";
                else ss << inner;
                ss << ">";
            }
            else if (func.returnType.find("Array<") == 0) {
                std::string inner = func.returnType.substr(6, func.returnType.size() - 7);
                ss << "std::vector<";
                if (inner == "int") ss << "int";
                else if (inner == "string") ss << "std::string";
                else ss << inner;
                ss << ">";
            }
            else ss << func.returnType;
            
            ss << " " << func.name << "(";
            
            // Parameters
            for (size_t i = 0; i < func.paramNames.size(); i++) {
                if (i > 0) ss << ", ";
                
                const std::string& ptype = func.paramTypes[i];
                if (ptype == "int") ss << "int";
                else if (ptype == "float") ss << "double";
                else if (ptype == "string") ss << "const std::string&";
                else if (ptype == "bool") ss << "bool";
                else if (ptype.find("Array<") == 0) {
                    std::string inner = ptype.substr(6, ptype.size() - 7);
                    ss << "std::vector<";
                    if (inner == "int") ss << "int";
                    else if (inner == "string") ss << "std::string";
                    else ss << inner;
                    ss << ">&";
                }
                else if (ptype.find("Option<") == 0) {
                    std::string inner = ptype.substr(7, ptype.size() - 8);
                    ss << "const std::optional<";
                    if (inner == "int") ss << "int";
                    else if (inner == "string") ss << "std::string";
                    else ss << inner;
                    ss << ">&";
                }
                else ss << "const " << ptype << "&";
                
                ss << " " << func.paramNames[i];
            }
            
            ss << ") {\n";
            ss << "        " << cppCode << "\n";
            ss << "    }\n\n";
        }
    }
    
    ss << "}\n\n";
    return ss.str();
}

std::string StdLibCodeGen::extractCppCode(const std::string& source, const std::string& funcName) {
    // Find the function
    std::regex funcStartRegex("pub\\s+fn\\s+" + funcName + "\\s*\\(");
    std::smatch match;
    
    if (!std::regex_search(source, match, funcStartRegex)) {
        return "";
    }
    
    size_t funcStart = match.position();
    
    // Find @cpp block after this function
    size_t cppStart = source.find("@cpp", funcStart);
    if (cppStart == std::string::npos) return "";
    
    // Make sure this @cpp is within this function (before next pub fn)
    size_t nextFunc = source.find("pub fn", funcStart + 10);
    if (nextFunc != std::string::npos && cppStart > nextFunc) {
        return "";
    }
    
    // Find opening brace after @cpp
    size_t braceStart = source.find('{', cppStart);
    if (braceStart == std::string::npos) return "";
    
    // Find matching closing brace
    int depth = 1;
    size_t pos = braceStart + 1;
    while (pos < source.size() && depth > 0) {
        if (source[pos] == '{') depth++;
        else if (source[pos] == '}') depth--;
        pos++;
    }
    
    if (depth != 0) return "";
    
    std::string code = source.substr(braceStart + 1, pos - braceStart - 2);
    
    // Trim leading/trailing whitespace
    size_t start = code.find_first_not_of(" \t\n\r");
    size_t end = code.find_last_not_of(" \t\n\r");
    
    if (start != std::string::npos && end != std::string::npos) {
        return code.substr(start, end - start + 1);
    }
    
    return code;
}
