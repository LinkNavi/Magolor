#pragma once
#include "ast.hpp"
#include <string>
#include <algorithm>
#include <vector>
#include <memory>
#include <unordered_map>
#include <filesystem>

namespace fs = std::filesystem;

struct Module {
    std::string name;
    std::string filepath;
    std::string packageName;  // NEW: Track which package this module belongs to
    Program ast;
    std::vector<std::string> importedModules;
};

using ModulePtr = std::shared_ptr<Module>;

class ModuleRegistry {
public:
    static ModuleRegistry& instance() {
        static ModuleRegistry inst;
        return inst;
    }
    
    void registerModule(ModulePtr module) {
        modules[module->name] = module;
    }
    
    void registerModule(const std::string& name, ModulePtr module) {
        modules[name] = module;
    }
    
    ModulePtr getModule(const std::string& name) {
        auto it = modules.find(name);
        return it != modules.end() ? it->second : nullptr;
    }
    
    const std::unordered_map<std::string, ModulePtr>& getModules() const {
        return modules;
    }
    
    void clear() {
        modules.clear();
    }
    
private:
    std::unordered_map<std::string, ModulePtr> modules;
    ModuleRegistry() = default;
};

class ModuleResolver {
public:
    // Convert file path to module name
    // Example: src/api/handlers.mg -> "api.handlers"
    static std::string filePathToModuleName(const std::string& filepath, 
                                            const std::string& packageName = "") {
        std::string path = filepath;
        
        // Remove package-specific paths
        if (!packageName.empty()) {
            // Remove .magolor/packages/<package>/src/ prefix if present
            std::string packagePrefix = ".magolor/packages/" + packageName + "/src/";
            if (path.find(packagePrefix) == 0) {
                path = path.substr(packagePrefix.length());
            }
        }
        
        // Remove src/ prefix if present
        if (path.find("src/") == 0) {
            path = path.substr(4);
        }
        
        // Remove .mg extension
        if (path.size() >= 3 && path.substr(path.size() - 3) == ".mg") {
            path = path.substr(0, path.size() - 3);
        }
        
        // Replace path separators with dots
        std::string result;
        if (!packageName.empty()) {
            result = packageName + ".";
        }
        
        for (char c : path) {
            if (c == '/' || c == '\\') {
                result += '.';
            } else {
                result += c;
            }
        }
        
        return result;
    }
    
    // Resolve import path relative to current module
    static std::string resolveImportPath(const std::string& importPath,
                                        const std::string& currentModulePath) {
        // Convert path (e.g., ["api", "handlers"]) to string
        std::string pathStr = importPath;
        
        // Try absolute path first
        std::string srcPath = "src/" + pathStr;
        std::replace(srcPath.begin(), srcPath.end(), '.', '/');
        srcPath += ".mg";
        
        if (fs::exists(srcPath)) {
            return pathStr;
        }
        
        // Try relative to current module's directory
        size_t lastDot = currentModulePath.rfind('.');
        if (lastDot != std::string::npos) {
            std::string parentPackage = currentModulePath.substr(0, lastDot);
            std::string candidatePath = parentPackage + "." + pathStr;
            
            std::string filePath = "src/" + candidatePath;
            std::replace(filePath.begin(), filePath.end(), '.', '/');
            filePath += ".mg";
            
            if (fs::exists(filePath)) {
                return candidatePath;
            }
        }
        
        // Check in .magolor/packages
        for (const auto& entry : fs::directory_iterator(".magolor/packages")) {
            if (entry.is_directory()) {
                std::string packagePath = entry.path().string() + "/src/" + pathStr;
                std::replace(packagePath.begin(), packagePath.end(), '.', '/');
                packagePath += ".mg";
                
                if (fs::exists(packagePath)) {
                    return entry.path().filename().string() + "." + pathStr;
                }
            }
        }
        
        return pathStr;
    }
    
    // Check if a symbol is publicly accessible
    static bool isPublic(ModulePtr module, 
                        const std::string& symbolName,
                        bool isClassName = false) {
        if (!module) return false;
        
        // Check classes
        for (const auto& cls : module->ast.classes) {
            if (cls.name == symbolName) {
                return cls.isPublic;
            }
            
            if (isClassName) {
                for (const auto& field : cls.fields) {
                    if (field.name == symbolName) {
                        return field.isPublic;
                    }
                }
                for (const auto& method : cls.methods) {
                    if (method.name == symbolName) {
                        return method.isPublic;
                    }
                }
            }
        }
        
        // Check functions
        for (const auto& fn : module->ast.functions) {
            if (fn.name == symbolName) {
                return fn.isPublic;
            }
        }
        
        return false;
    }
    
    // Get all public symbols from a module
    static std::vector<std::string> getPublicSymbols(ModulePtr module) {
        std::vector<std::string> symbols;
        if (!module) return symbols;
        
        for (const auto& cls : module->ast.classes) {
            if (cls.isPublic) {
                symbols.push_back(cls.name);
            }
        }
        
        for (const auto& fn : module->ast.functions) {
            if (fn.isPublic) {
                symbols.push_back(fn.name);
            }
        }
        
        return symbols;
    }
};

// Import resolver
struct ImportResult {
    bool success = true;
    std::string error;
};

class ImportResolver {
public:
    ImportResult resolve(ModulePtr module) {
        ImportResult result;
        
        for (const auto& usingDecl : module->ast.usings) {
            // Convert path vector to string
            std::string importPath;
            for (size_t i = 0; i < usingDecl.path.size(); i++) {
                if (i > 0) importPath += ".";
                importPath += usingDecl.path[i];
            }
            
            // Resolve relative imports
            importPath = ModuleResolver::resolveImportPath(importPath, module->name);
            
            // Check if module exists
            ModulePtr importedModule = ModuleRegistry::instance().getModule(importPath);
            if (!importedModule) {
                result.success = false;
                result.error = "Cannot find module: " + importPath;
                return result;
            }
            
            module->importedModules.push_back(importPath);
        }
        
        return result;
    }
};

// Name resolver
struct NameResolutionResult {
    bool success = true;
    std::vector<std::string> errors;
};

class NameResolver {
public:
    NameResolutionResult resolve(ModulePtr module) {
        NameResolutionResult result;
        
        // Build symbol table from imports
        for (const auto& importedModuleName : module->importedModules) {
            ModulePtr importedModule = ModuleRegistry::instance().getModule(importedModuleName);
            if (importedModule) {
                auto symbols = ModuleResolver::getPublicSymbols(importedModule);
                for (const auto& symbol : symbols) {
                    importedSymbols[symbol] = importedModuleName;
                }
            }
        }
        
        return result;
    }
    
private:
    std::unordered_map<std::string, std::string> importedSymbols;
};
