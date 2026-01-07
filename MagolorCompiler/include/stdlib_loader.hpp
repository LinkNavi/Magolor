#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <filesystem>

namespace fs = std::filesystem;

// Represents a parsed function from stdlib .mg files
struct StdFunction {
    std::string name;
    std::string signature;
    std::string returnType;
    std::vector<std::string> paramNames;
    std::vector<std::string> paramTypes;
    std::string documentation;
    bool isConstant = false;
    bool isStatic = false;
};

// Represents a parsed class from stdlib .mg files
struct StdClass {
    std::string name;
    std::string documentation;
    std::vector<StdFunction> methods;
    std::vector<std::pair<std::string, std::string>> fields; // name, type
    std::vector<std::pair<std::string, std::string>> staticConstants; // name, type
};

// Represents a parsed enum from stdlib .mg files
struct StdEnum {
    std::string name;
    std::vector<std::string> variants;
    std::string documentation;
};

// C import symbol types
enum class CSymbolKind {
    Function,
    Struct,
    Enum,
    EnumValue,
    Typedef,
    Macro,
    Variable
};

// Represents a C symbol from header parsing
struct CSymbol {
    std::string name;
    CSymbolKind kind;
    std::string returnType;          // For functions
    std::vector<std::string> paramNames;
    std::vector<std::string> paramTypes;
    std::vector<std::string> enumValues;  // For enums
    std::vector<std::pair<std::string, std::string>> fields; // For structs
    std::string documentation;
    std::string headerFile;          // Source header
};

// Represents a parsed C header
struct CHeader {
    std::string path;                // Header path (e.g., <openssl/evp.h>)
    std::string absolutePath;        // Resolved filesystem path
    std::vector<CSymbol> symbols;
    bool parsed = false;
};

// Module metadata from @link and @include directives
struct ModuleMetadata {
    std::vector<std::string> linkFlags;      // e.g., {"-lssl", "-lcrypto"}
    std::vector<std::string> cppIncludes;    // e.g., {"<openssl/evp.h>"}
    std::vector<std::string> cimports;       // e.g., {"openssl/evp.h"}
};

// Represents a loaded stdlib module
struct StdModule {
    std::string name;           // Short name (e.g., "IO")
    std::string fullPath;       // Full path (e.g., "Std.IO")
    std::string filePath;       // Filesystem path to .mg file
    std::vector<StdFunction> functions;
    std::vector<StdClass> classes;
    std::vector<StdEnum> enums;
    std::vector<std::string> constants;
    ModuleMetadata metadata;    // Link flags, includes, etc.
    bool loaded = false;
    bool parsed = false;
};

// C Header parser for cimport support
class CHeaderParser {
public:
    static CHeaderParser& instance() {
        static CHeaderParser inst;
        return inst;
    }
    
    // Initialize with system include paths
    void init();
    void addIncludePath(const std::string& path);
    
    // Parse a header file
    CHeader* parseHeader(const std::string& headerPath);
    
    // Get symbols from a header
    std::vector<CSymbol> getSymbols(const std::string& headerPath);
    std::vector<CSymbol> getFunctions(const std::string& headerPath);
    std::vector<CSymbol> getStructs(const std::string& headerPath);
    std::vector<CSymbol> getEnums(const std::string& headerPath);
    std::vector<CSymbol> getMacros(const std::string& headerPath);
    
    // Symbol lookup
    CSymbol* findSymbol(const std::string& headerPath, const std::string& name);
    
    // Get all parsed headers
    std::vector<std::string> getParsedHeaders() const;
    
private:
    CHeaderParser() = default;
    
    std::string resolveHeaderPath(const std::string& headerPath);
    void parseHeaderContent(const std::string& content, CHeader& header);
    void extractFunctions(const std::string& content, CHeader& header);
    void extractStructs(const std::string& content, CHeader& header);
    void extractEnums(const std::string& content, CHeader& header);
    void extractTypedefs(const std::string& content, CHeader& header);
    void extractMacros(const std::string& content, CHeader& header);
    
    std::vector<std::string> includePaths;
    std::unordered_map<std::string, CHeader> headers;
    bool initialized = false;
};

// Main stdlib loader - singleton pattern
class StdLibLoader {
public:
    static StdLibLoader& instance() {
        static StdLibLoader inst;
        return inst;
    }
    
    // Initialize with path to stdlib directory
    void init(const std::string& stdlibPath);
    
    // Check if initialized
    bool isInitialized() const { return initialized; }
    
    // Module queries
    bool hasModule(const std::string& modulePath) const;
    std::vector<std::string> getAvailableModules() const;
    StdModule* loadModule(const std::string& modulePath);
    
    // Symbol queries
    std::vector<StdFunction> getFunctions(const std::string& modulePath);
    std::vector<StdClass> getClasses(const std::string& modulePath);
    std::vector<StdEnum> getEnums(const std::string& modulePath);
    
    // Metadata queries
    ModuleMetadata getModuleMetadata(const std::string& modulePath);
    std::vector<std::string> getLinkFlags(const std::string& modulePath);
    std::vector<std::string> getCppIncludes(const std::string& modulePath);
    
    // Collect all link flags for a set of modules
    std::vector<std::string> collectLinkFlags(const std::unordered_set<std::string>& modules);
    std::vector<std::string> collectIncludes(const std::unordered_set<std::string>& modules);
    
    // Function lookup
    bool isStdFunction(const std::string& funcName) const;
    std::string getModuleForFunction(const std::string& funcName) const;
    std::string getFunctionSignature(const std::string& funcName) const;
    std::string getReturnType(const std::string& funcName) const;
    
    // Get all symbols for completion
    std::vector<std::string> getAllFunctionNames() const;
    std::vector<std::string> getAllClassNames() const;
    
private:
    StdLibLoader() = default;
    
    void discoverModules();
    void parseModuleFile(StdModule& module);
    void extractMetadata(const std::string& source, StdModule& module);
    void extractFunctions(const std::string& source, StdModule& module);
    void extractClasses(const std::string& source, StdModule& module);
    void extractEnums(const std::string& source, StdModule& module);
    void extractConstants(const std::string& source, StdModule& module);
    
    // Parse helpers
    std::string parseType(const std::string& typeStr);
    std::vector<std::pair<std::string, std::string>> parseParams(const std::string& paramStr);
    std::string extractDocComment(const std::string& source, size_t pos);
    
    std::string stdlibPath;
    bool initialized = false;
    
    std::unordered_map<std::string, StdModule> modules;
    std::unordered_map<std::string, std::string> functionToModule;
    std::unordered_map<std::string, StdFunction> allFunctions;
    std::unordered_map<std::string, StdClass> allClasses;
};

// Code generation helper
class StdLibCodeGen {
public:
    static std::string generateIncludes();
    static std::string generateHelpers();
    static std::string generateAll(const std::unordered_set<std::string>& usedModules = {});
    static std::string generateNamespace(const std::unordered_set<std::string>& usedModules);
    static std::string generateModuleCode(const std::string& modulePath);
    static std::string extractCppCode(const std::string& source, const std::string& funcName);
};
