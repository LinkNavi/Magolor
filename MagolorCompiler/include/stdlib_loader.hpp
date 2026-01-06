#pragma once
#include "ast.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>

namespace fs = std::filesystem;

// ============================================================================
// StdLib Module metadata
// ============================================================================

struct StdFunction {
    std::string name;
    std::string signature;      // e.g., "(s: string) -> int"
    std::string returnType;
    std::vector<std::string> paramTypes;
    std::vector<std::string> paramNames;
    bool isConstant = false;
    bool isGeneric = false;
    std::string documentation;
};

struct StdClass {
    std::string name;
    std::vector<std::string> fields;
    std::vector<std::string> methods;
    std::string documentation;
};

struct StdModule {
    std::string name;           // e.g., "IO", "Math"
    std::string fullPath;       // e.g., "Std.IO"
    std::string filePath;       // Path to .mg file
    std::vector<StdFunction> functions;
    std::vector<StdClass> classes;
    std::vector<std::string> constants;
    std::vector<std::string> dependencies;  // Other modules this depends on
    bool loaded = false;
    bool parsed = false;
};

// ============================================================================
// StdLib Loader - discovers and parses stdlib modules
// ============================================================================

class StdLibLoader {
public:
    static StdLibLoader& instance() {
        static StdLibLoader inst;
        return inst;
    }
    
    // Initialize with stdlib path
    void init(const std::string& stdlibPath);
    
    // Discover all modules in stdlib directory
    void discoverModules();
    
    // Load a specific module by path (e.g., "Std.IO")
    StdModule* loadModule(const std::string& modulePath);
    
    // Check if a module exists
    bool hasModule(const std::string& modulePath) const;
    
    // Get all available module paths
    std::vector<std::string> getAvailableModules() const;
    
    // Get function info for a module
    std::vector<StdFunction> getFunctions(const std::string& modulePath);
    
    // Get class info for a module
    std::vector<StdClass> getClasses(const std::string& modulePath);
    
    // Check if a function exists in any loaded module
    bool isStdFunction(const std::string& funcName) const;
    
    // Get the module containing a function
    std::string getModuleForFunction(const std::string& funcName) const;
    
    // Get function signature
    std::string getFunctionSignature(const std::string& funcName) const;
    
    // Get return type for a function
    TypePtr getReturnType(const std::string& funcName) const;
    
    // Generate C++ code for used modules
    std::string generateCppForModules(const std::unordered_set<std::string>& usedModules);
    
    // Get stdlib root path
    const std::string& getStdLibPath() const { return stdlibPath; }
    
private:
    StdLibLoader() = default;
    
    std::string stdlibPath;
    std::unordered_map<std::string, StdModule> modules;
    std::unordered_map<std::string, std::string> functionToModule;
    std::unordered_map<std::string, StdFunction> allFunctions;
    bool initialized = false;
    
    // Parse a .mg file to extract module info
    void parseModuleFile(StdModule& module);
    
    // Extract function signatures from source
    void extractFunctions(const std::string& source, StdModule& module);
    
    // Extract class definitions from source
    void extractClasses(const std::string& source, StdModule& module);
    
    // Convert Magolor type to C++ type
    std::string toCppType(const std::string& mgType);
    
    // Generate C++ code for a single function
    std::string generateFunctionCpp(const StdFunction& func, const std::string& source);
};

// ============================================================================
// StdLib Code Generator - generates C++ from stdlib modules
// ============================================================================

class StdLibCodeGen {
public:
    // Generate the includes needed
    static std::string generateIncludes();
    
    // Generate the Std namespace with all used modules
    static std::string generateNamespace(const std::unordered_set<std::string>& usedModules);
    
    // Generate helper templates
    static std::string generateHelpers();
    
    // Generate complete stdlib code
    static std::string generateAll(const std::unordered_set<std::string>& usedModules);
    
private:
    // Generate code for a specific module
    static std::string generateModuleCode(const std::string& modulePath);
    
    // Parse @cpp blocks from source
    static std::string extractCppCode(const std::string& source, const std::string& funcName);
};
