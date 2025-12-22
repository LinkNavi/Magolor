#pragma once
#include <string>
#include <vector>
#include <map>

// StdLib Module Manager
// Allows extracting, editing, and importing stdlib modules

struct StdLibFunction {
    std::string name;
    std::string signature;
    std::string body;
    bool isTemplate;
    std::string templateParams;
};

struct StdLibModule {
    std::string name;
    std::string comment;
    std::vector<std::string> constants;
    std::vector<StdLibFunction> functions;
    std::string rawCode;  // For custom/complex code
};

class StdLibManager {
public:
    // Extract a stdlib module to an editable file
    static bool extractModule(const std::string& moduleName, const std::string& outputFile);
    
    // Import a module file back into stdlib format
    static std::string importModule(const std::string& inputFile);
    
    // Create a new custom module template
    static bool createModuleTemplate(const std::string& moduleName, const std::string& outputFile);
    
    // Get all available stdlib modules
    static std::vector<std::string> getAvailableModules();
    
private:
    // Parse a module file into a StdLibModule struct
    static StdLibModule parseModuleFile(const std::string& content);
    
    // Convert a StdLibModule to C++ code
    static std::string generateModuleCode(const StdLibModule& module);
    
    // Get the raw source for a specific module
    static std::string getModuleSource(const std::string& moduleName);
    
    // Format C++ code with proper indentation
    static std::string formatCode(const std::string& code);
};
