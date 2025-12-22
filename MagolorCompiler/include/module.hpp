#pragma once
#include "ast.hpp"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <filesystem>

namespace fs = std::filesystem;

struct Module {
    std::string name;
    std::string filepath;
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
    static std::string filePathToModuleName(const std::string& filepath) {
        std::string path = filepath;
        
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
        // Try absolute path first
        if (fs::exists("src/" + importPath + ".mg")) {
            return importPath;
        }
        
        // Try relative to current module's directory
        // For example, if we're in "api.handlers" and import "types",
        // try "api.types"
        size_t lastDot = currentModulePath.rfind('.');
        if (lastDot != std::string::npos) {
            std::string parentPackage = currentModulePath.substr(0, lastDot);
            std::string candidatePath = parentPackage + "." + importPath;
            
            // Convert back to file path to check
            std::string filePath = "src/" + candidatePath;
            std::replace(filePath.begin(), filePath.end(), '.', '/');
            filePath += ".mg";
            
            if (fs::exists(filePath)) {
                return candidatePath;
            }
        }
        
        // Default to the import path as-is
        return importPath;
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
            
            // If looking for a member of this class
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
    
    // Check if a member of a class is publicly accessible
    static bool isMemberPublic(ModulePtr module,
                              const std::string& className,
                              const std::string& memberName) {
        if (!module) return false;
        
        for (const auto& cls : module->ast.classes) {
            if (cls.name == className) {
                // Check fields
                for (const auto& field : cls.fields) {
                    if (field.name == memberName) {
                        return field.isPublic;
                    }
                }
                
                // Check methods
                for (const auto& method : cls.methods) {
                    if (method.name == memberName) {
                        return method.isPublic;
                    }
                }
            }
        }
        
        return false;
    }
    
    // Get all public symbols from a module
    static std::vector<std::string> getPublicSymbols(ModulePtr module) {
        std::vector<std::string> symbols;
        if (!module) return symbols;
        
        // Public classes
        for (const auto& cls : module->ast.classes) {
            if (cls.isPublic) {
                symbols.push_back(cls.name);
            }
        }
        
        // Public functions
        for (const auto& fn : module->ast.functions) {
            if (fn.isPublic) {
                symbols.push_back(fn.name);
            }
        }
        
        return symbols;
    }
};

// Import resolver - checks that all imports can be satisfied
struct ImportResult {
    bool success = true;
    std::string error;
};

class ImportResolver {
public:
    ImportResult resolve(ModulePtr module) {
        ImportResult result;
        
        for (const auto& usingDecl : module->ast.usings) {
            std::string importPath = usingDecl.modulePath;
            
            // Resolve relative imports
            importPath = ModuleResolver::resolveImportPath(importPath, module->name);
            
            // Check if module exists
            ModulePtr importedModule = ModuleRegistry::instance().getModule(importPath);
            if (!importedModule) {
                result.success = false;
                result.error = "Cannot find module: " + usingDecl.modulePath + 
                              " (resolved to: " + importPath + ")";
                return result;
            }
            
            // Record the import
            module->importedModules.push_back(importPath);
            
            // If importing specific symbols, check visibility
            if (!usingDecl.symbols.empty()) {
                for (const auto& symbol : usingDecl.symbols) {
                    if (!ModuleResolver::isPublic(importedModule, symbol)) {
                        result.success = false;
                        result.error = "Cannot import private symbol: " + symbol + 
                                      " from module: " + importPath;
                        return result;
                    }
                }
            }
        }
        
        return result;
    }
};

// Name resolver - resolves all identifiers to their definitions
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
        
        // TODO: Walk the AST and verify all identifiers are defined
        // This is a simplified implementation
        
        return result;
    }
    
private:
    std::unordered_map<std::string, std::string> importedSymbols;
};
