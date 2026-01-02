// src/stdlib_parser.cpp
#include "stdlib_parser.hpp"
#include "stdlib.hpp"

std::vector<StdLibFunction> StdLibParser::parseStdLib() {
    std::vector<StdLibFunction> functions;
    
    // Get the raw source
    std::string source = StdLibGenerator::generateAll();
    
    // Parse each namespace
    parseNamespace(source, "IO", "", functions);
    parseNamespace(source, "Parse", "", functions);
    parseNamespace(source, "Option", "", functions);
    parseNamespace(source, "Math", "", functions);
    parseNamespace(source, "String", "", functions);
    parseNamespace(source, "Array", "", functions);
    parseNamespace(source, "Map", "", functions);
    parseNamespace(source, "Set", "", functions);
    parseNamespace(source, "File", "", functions);
    parseNamespace(source, "Time", "", functions);
    parseNamespace(source, "Random", "", functions);
    parseNamespace(source, "System", "", functions);
    
    // Parse Network and its submodules
    parseNamespace(source, "Network", "", functions);
    parseNetworkSubmodules(source, functions);
    
    return functions;
}

void StdLibParser::parseNamespace(const std::string& source, 
                           const std::string& namespaceName,
                           const std::string& submodule,
                           std::vector<StdLibFunction>& functions) {
    // Find the namespace block
    std::string nsPattern;
    if (submodule.empty()) {
        nsPattern = "namespace\\s+" + namespaceName + "\\s*\\{";
    } else {
        nsPattern = "namespace\\s+" + submodule + "\\s*\\{";
    }
    
    std::regex nsRegex(nsPattern);
    std::smatch nsMatch;
    
    auto searchStart = source.cbegin();
    while (std::regex_search(searchStart, source.cend(), nsMatch, nsRegex)) {
        // Find matching closing brace
        size_t startPos = nsMatch.position(0) + searchStart - source.cbegin();
        size_t braceStart = source.find('{', startPos);
        size_t braceEnd = findMatchingBrace(source, braceStart);
        
        if (braceEnd == std::string::npos) break;
        
        std::string namespaceBody = source.substr(braceStart + 1, braceEnd - braceStart - 1);
        
        // Parse inline functions
        parseInlineFunctions(namespaceBody, namespaceName, submodule, functions);
        
        // Parse constexpr constants
        parseConstants(namespaceBody, namespaceName, submodule, functions);
        
        searchStart = source.cbegin() + braceEnd + 1;
    }
}

void StdLibParser::parseNetworkSubmodules(const std::string& source, 
                                   std::vector<StdLibFunction>& functions) {
    // Network has submodules
    std::vector<std::string> submodules = {
        "HTTP", "WebSocket", "TCP", "UDP", "Security", "JSON", "Routing"
    };
    
    for (const auto& submodule : submodules) {
        parseNamespace(source, "Network", submodule, functions);
    }
}

void StdLibParser::parseInlineFunctions(const std::string& code,
                                 const std::string& module,
                                 const std::string& submodule,
                                 std::vector<StdLibFunction>& functions) {
    // Regex to match inline functions
    std::regex funcRegex(R"(inline\s+([a-zA-Z_][a-zA-Z0-9_<>,\s\*&:]*)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\(([^)]*)\))");
    
    auto searchStart = code.cbegin();
    std::smatch match;
    
    while (std::regex_search(searchStart, code.cend(), match, funcRegex)) {
        StdLibFunction func;
        func.name = match[2].str();
        func.module = module;
        func.submodule = submodule;
        func.isConstant = false;
        
        // Build signature
        std::string returnType = match[1].str();
        std::string params = match[3].str();
        
        // Convert C++ types to Magolor types
        returnType = cppTypeToMagolor(returnType);
        std::string magolorParams = convertParams(params);
        
        func.signature = func.name + "(" + magolorParams + ") -> " + returnType;
        
        functions.push_back(func);
        
        searchStart = match.suffix().first;
    }
}

void StdLibParser::parseConstants(const std::string& code,
                           const std::string& module,
                           const std::string& submodule,
                           std::vector<StdLibFunction>& functions) {
    // Regex to match constexpr constants
    std::regex constRegex(R"(constexpr\s+([a-zA-Z_][a-zA-Z0-9_]*)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*=)");
    
    auto searchStart = code.cbegin();
    std::smatch match;
    
    while (std::regex_search(searchStart, code.cend(), match, constRegex)) {
        StdLibFunction func;
        func.name = match[2].str();
        func.module = module;
        func.submodule = submodule;
        func.isConstant = true;
        
        std::string type = match[1].str();
        type = cppTypeToMagolor(type);
        func.signature = func.name + ": " + type;
        
        functions.push_back(func);
        
        searchStart = match.suffix().first;
    }
}

std::string StdLibParser::cppTypeToMagolor(const std::string& cppType) {
    std::string type = cppType;
    
    // Trim whitespace
    type.erase(0, type.find_first_not_of(" \t\n\r"));
    type.erase(type.find_last_not_of(" \t\n\r") + 1);
    
    // Convert common types
    if (type == "int") return "int";
    if (type == "double") return "float";
    if (type == "float") return "float";
    if (type == "bool") return "bool";
    if (type == "void") return "void";
    if (type == "char") return "char";
    if (type == "std::string") return "string";
    
    if (type.find("std::optional") != std::string::npos) {
        size_t start = type.find('<') + 1;
        size_t end = type.rfind('>');
        std::string inner = type.substr(start, end - start);
        return "Option<" + cppTypeToMagolor(inner) + ">";
    }
    
    if (type.find("std::vector") != std::string::npos) {
        size_t start = type.find('<') + 1;
        size_t end = type.rfind('>');
        std::string inner = type.substr(start, end - start);
        return "Array<" + cppTypeToMagolor(inner) + ">";
    }
    
    return type;
}

std::string StdLibParser::convertParams(const std::string& cppParams) {
    if (cppParams.empty()) return "";
    
    std::vector<std::string> params;
    std::stringstream ss(cppParams);
    std::string param;
    
    while (std::getline(ss, param, ',')) {
        param.erase(0, param.find_first_not_of(" \t\n\r"));
        param.erase(param.find_last_not_of(" \t\n\r") + 1);
        
        size_t lastSpace = param.rfind(' ');
        if (lastSpace != std::string::npos) {
            std::string type = param.substr(0, lastSpace);
            std::string name = param.substr(lastSpace + 1);
            
            size_t equalPos = name.find('=');
            if (equalPos != std::string::npos) {
                name = name.substr(0, equalPos);
                name.erase(name.find_last_not_of(" \t\n\r") + 1);
            }
            
            type.erase(std::remove_if(type.begin(), type.end(), 
                [](char c) { return c == '&' || c == '*'; }), type.end());
            
            if (type.find("const ") == 0) {
                type = type.substr(6);
            }
            
            type.erase(0, type.find_first_not_of(" \t\n\r"));
            type.erase(type.find_last_not_of(" \t\n\r") + 1);
            
            params.push_back(name + ": " + cppTypeToMagolor(type));
        }
    }
    
    std::string result;
    for (size_t i = 0; i < params.size(); i++) {
        if (i > 0) result += ", ";
        result += params[i];
    }
    
    return result;
}

size_t StdLibParser::findMatchingBrace(const std::string& str, size_t start) {
    int depth = 1;
    size_t pos = start + 1;
    
    while (pos < str.length() && depth > 0) {
        if (str[pos] == '{') depth++;
        else if (str[pos] == '}') depth--;
        pos++;
    }
    
    return depth == 0 ? pos - 1 : std::string::npos;
}
