// include/stdlib_parser.hpp
#pragma once
#include <string>
#include <vector>
#include <map>
#include <set>
#include <regex>
#include <sstream>
#include <algorithm>

struct StdLibFunction {
    std::string name;
    std::string signature;
    std::string module;
    std::string submodule;
    bool isConstant;
};

class StdLibParser {
public:
    static std::vector<StdLibFunction> parseStdLib();
    
private:
    static void parseNamespace(const std::string& source, 
                               const std::string& namespaceName,
                               const std::string& submodule,
                               std::vector<StdLibFunction>& functions);
    
    static void parseNetworkSubmodules(const std::string& source, 
                                       std::vector<StdLibFunction>& functions);
    
    static void parseInlineFunctions(const std::string& code,
                                     const std::string& module,
                                     const std::string& submodule,
                                     std::vector<StdLibFunction>& functions);
    
    static void parseConstants(const std::string& code,
                               const std::string& module,
                               const std::string& submodule,
                               std::vector<StdLibFunction>& functions);
    
    static std::string cppTypeToMagolor(const std::string& cppType);
    
    static std::string convertParams(const std::string& cppParams);
    
    static size_t findMatchingBrace(const std::string& str, size_t start);
};
