#pragma once

#include <unordered_set>
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
    std::string packageName;
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
    // Check if this is a built-in standard library module
    static bool isBuiltinModule(const std::string& modulePath) {
        // List of all built-in Std modules
        static const std::unordered_set<std::string> builtins = {
            "Std", "Std.IO", "Std.Parse", "Std.Option", "Std.Math",
            "Std.String", "Std.Array", "Std.Map", "Std.Set", "Std.File",
            "Std.Network", "Std.Time", "Std.Random", "Std.System",
            // Network submodules
            "Std.Network.HTTP", "Std.Network.WebSocket", "Std.Network.TCP",
            "Std.Network.UDP", "Std.Network.Security", "Std.Network.JSON",
            "Std.Network.Routing"
        };
        
        return builtins.count(modulePath) > 0;
    }
    
    // Convert file path to module name
    static std::string filePathToModuleName(const std::string& filepath, 
                                            const std::string& packageName = "") {
        std::string path = filepath;
        
        if (!packageName.empty()) {
            std::string packagePrefix = ".magolor/packages/" + packageName + "/src/";
            if (path.find(packagePrefix) == 0) {
                path = path.substr(packagePrefix.length());
            }
        }
        
        if (path.find("src/") == 0) {
            path = path.substr(4);
        }
        
        if (path.size() >= 3 && path.substr(path.size() - 3) == ".mg") {
            path = path.substr(0, path.size() - 3);
        }
        
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
        // Check if it's a built-in module first
        if (isBuiltinModule(importPath)) {
            return importPath;
        }
        
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
        if (fs::exists(".magolor/packages")) {
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
        }
        
        return pathStr;
    }
    
    static bool isPublic(ModulePtr module, 
                        const std::string& symbolName,
                        bool isClassName = false) {
        if (!module) return false;
        
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
        
        for (const auto& fn : module->ast.functions) {
            if (fn.name == symbolName) {
                return fn.isPublic;
            }
        }
        
        return false;
    }
    
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

struct ImportResult {
    bool success = true;
    std::string error;
};

class ImportResolver {
public:
    ImportResult resolve(ModulePtr module) {
        ImportResult result;
        
        for (const auto& usingDecl : module->ast.usings) {
            std::string importPath;
            for (size_t i = 0; i < usingDecl.path.size(); i++) {
                if (i > 0) importPath += ".";
                importPath += usingDecl.path[i];
            }
            
            // Check if it's a built-in module
            if (ModuleResolver::isBuiltinModule(importPath)) {
                // Built-in modules don't need resolution
                module->importedModules.push_back(importPath);
                continue;
            }
            
            // Resolve relative imports for user modules
            importPath = ModuleResolver::resolveImportPath(importPath, module->name);
            
            // Check if module exists
            ModulePtr importedModule = ModuleRegistry::instance().getModule(importPath);
            if (!importedModule) {
                // Only error if it's not a built-in module
                if (!ModuleResolver::isBuiltinModule(importPath)) {
                    result.success = false;
                    result.error = "Cannot find module: " + importPath;
                    return result;
                }
            }
            
            module->importedModules.push_back(importPath);
        }
        
        return result;
    }
};

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
            // Skip built-in modules - they're handled by the C++ compiler
            if (ModuleResolver::isBuiltinModule(importedModuleName)) {
                continue;
            }
            
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

struct Callable {
    std::string name;
    std::string module;
    std::string className;
};

class CallableCollector {
public:
    static std::vector<Callable> getAllCallablesRecursive(ModulePtr module) {
        std::vector<Callable> callables;
        std::unordered_set<std::string> visitedModules;
        collect(module, callables, visitedModules);
        return callables;
    }

private:
    static void collect(ModulePtr module, std::vector<Callable>& callables,
                        std::unordered_set<std::string>& visitedModules) {
        if (!module || visitedModules.count(module->name)) return;

        visitedModules.insert(module->name);

        // Top-level functions
        for (const auto& fn : module->ast.functions) {
            if (fn.isPublic) {
                callables.push_back({fn.name, module->name, ""});
            }
        }

        // Methods in public classes
        for (const auto& cls : module->ast.classes) {
            if (!cls.isPublic) continue;
            for (const auto& method : cls.methods) {
                if (method.isPublic) {
                    callables.push_back({method.name, module->name, cls.name});
                }
            }
        }

        // Recursively process imported modules (skip built-ins)
        for (const auto& importedName : module->importedModules) {
            if (ModuleResolver::isBuiltinModule(importedName)) {
                continue;
            }
            ModulePtr imported = ModuleRegistry::instance().getModule(importedName);
            collect(imported, callables, visitedModules);
        }
    }
};
