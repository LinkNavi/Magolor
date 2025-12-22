#pragma once
#include "ast.hpp"
#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>

// Module represents a single source file or package
struct Module {
    std::string name;           // e.g., "Std.IO" or "mypackage.utils"
    std::string filepath;       // Source file path
    std::string packageName;    // Package this module belongs to
    Program ast;                // Parsed AST
    
    // Exported symbols
    std::map<std::string, FnDecl*> functions;
    std::map<std::string, ClassDecl*> classes;
    
    // Imported modules
    std::vector<std::string> imports;
};

using ModulePtr = std::shared_ptr<Module>;

// Symbol represents an exported item
struct Symbol {
    enum Kind { FUNCTION, CLASS, VARIABLE };
    Kind kind;
    std::string name;
    std::string modulePath;     // Full module path
    void* declaration;          // Pointer to FnDecl or ClassDecl
};

class ModuleRegistry {
public:
    static ModuleRegistry& instance() {
        static ModuleRegistry registry;
        return registry;
    }
    
    // Register built-in Std modules
    void registerStdModules() {
        // Create Std.IO module
        auto stdIO = std::make_shared<Module>();
        stdIO->name = "Std.IO";
        stdIO->filepath = "<builtin>";
        stdIO->packageName = "Std";
        
        // Register Std.Parse module
        auto stdParse = std::make_shared<Module>();
        stdParse->name = "Std.Parse";
        stdParse->filepath = "<builtin>";
        stdParse->packageName = "Std";
        
        // Register Std.Option module
        auto stdOption = std::make_shared<Module>();
        stdOption->name = "Std.Option";
        stdOption->filepath = "<builtin>";
        stdOption->packageName = "Std";
        
        // Register Std (base) module
        auto std = std::make_shared<Module>();
        std->name = "Std";
        std->filepath = "<builtin>";
        std->packageName = "Std";
        
        modules["Std.IO"] = stdIO;
        modules["Std.Parse"] = stdParse;
        modules["Std.Option"] = stdOption;
        modules["Std"] = std;
    }
    
    // Register a module
    void registerModule(ModulePtr module) {
        // Check if a module with this name already exists (for package merging)
        auto existing = modules.find(module->name);
        if (existing != modules.end() && existing->second->packageName == module->packageName) {
            // Merge this module into the existing one
            auto& existingModule = existing->second;
            
            // Merge functions
            for (auto& fn : module->ast.functions) {
                existingModule->ast.functions.push_back(fn);
            }
            
            // Merge classes
            for (auto& cls : module->ast.classes) {
                existingModule->ast.classes.push_back(cls);
            }
            
            // Merge usings (avoiding duplicates)
            for (auto& u : module->ast.usings) {
                bool found = false;
                for (auto& existing_u : existingModule->ast.usings) {
                    if (existing_u.path == u.path) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    existingModule->ast.usings.push_back(u);
                }
            }
            
            // Re-register symbols from the merged functions
            for (auto& fn : module->ast.functions) {
                if (fn.isPublic) {
                    Symbol sym;
                    sym.kind = Symbol::FUNCTION;
                    sym.name = fn.name;
                    sym.modulePath = existingModule->name;
                    sym.declaration = &fn;
                    symbols[existingModule->name + "::" + fn.name] = sym;
                }
            }
            
            return;
        }
        
        modules[module->name] = module;
        
        // Register exported functions
        for (auto& fn : module->ast.functions) {
            if (fn.isPublic) {
                Symbol sym;
                sym.kind = Symbol::FUNCTION;
                sym.name = fn.name;
                sym.modulePath = module->name;
                sym.declaration = &fn;
                symbols[module->name + "::" + fn.name] = sym;
            }
        }
        
        // Register exported classes
        for (auto& cls : module->ast.classes) {
            Symbol sym;
            sym.kind = Symbol::CLASS;
            sym.name = cls.name;
            sym.modulePath = module->name;
            sym.declaration = &cls;
            symbols[module->name + "::" + cls.name] = sym;
        }
    }
    
    // Find a module by name
    ModulePtr findModule(const std::string& name) {
        auto it = modules.find(name);
        return it != modules.end() ? it->second : nullptr;
    }
    
    // Resolve a symbol (function or class)
    Symbol* resolveSymbol(const std::string& modulePath, const std::string& symbolName) {
        std::string fullPath = modulePath + "::" + symbolName;
        auto it = symbols.find(fullPath);
        return it != symbols.end() ? &it->second : nullptr;
    }
    
    // Get all modules
    const std::map<std::string, ModulePtr>& getModules() const {
        return modules;
    }
    
    void clear() {
        modules.clear();
        symbols.clear();
        // Re-register built-in modules after clearing
        registerStdModules();
    }
    
private:
    std::map<std::string, ModulePtr> modules;
    std::map<std::string, Symbol> symbols;
    
    ModuleRegistry() {
        registerStdModules();
    }
};

class ModuleResolver {
public:
    // Convert file path to module name
    // src/main.mg -> main
    // src/utils/helpers.mg -> utils.helpers
    // .magolor/packages/http/src/lib.mg -> http
    static std::string filePathToModuleName(const std::string& filepath, 
                                           const std::string& packageName = "") {
        std::string name;
        
        // If it's from a package, use package name as prefix
        if (!packageName.empty() && filepath.find(".magolor/packages") != std::string::npos) {
            // For lib.mg, just use the package name
            if (filepath.find("/lib.mg") != std::string::npos) {
                return packageName;
            }
            // For other files in a package, use packageName.filename
            size_t lastSlash = filepath.rfind('/');
            if (lastSlash != std::string::npos) {
                std::string filename = filepath.substr(lastSlash + 1);
                if (filename.size() > 3 && filename.substr(filename.size() - 3) == ".mg") {
                    filename = filename.substr(0, filename.size() - 3);
                }
                if (filename != "lib") {
                    return packageName + "." + filename;
                }
            }
            return packageName;
        }
        
        // Extract relative path from src/
        size_t srcPos = filepath.find("/src/");
        if (srcPos != std::string::npos) {
            name = filepath.substr(srcPos + 5); // Skip "/src/"
        } else {
            name = filepath;
        }
        
        // Remove .mg extension
        if (name.size() > 3 && name.substr(name.size() - 3) == ".mg") {
            name = name.substr(0, name.size() - 3);
        }
        
        // Convert path separators to dots
        for (char& c : name) {
            if (c == '/' || c == '\\') c = '.';
        }
        
        // Handle lib.mg -> just use package name
        if (name == "lib" && !packageName.empty()) {
            return packageName;
        }
        
        // Add "src." prefix for local modules
        if (filepath.find("/src/") != std::string::npos && 
            filepath.find(".magolor/packages") == std::string::npos) {
            return "src." + name;
        }
        
        return name;
    }
    
    // Resolve using path from a module
    // If we're in module "myapp.utils" and import "helpers"
    // Try: myapp.utils.helpers, myapp.helpers, helpers
    static std::vector<std::string> resolveImportPath(const std::string& currentModule,
                                                       const std::vector<std::string>& importPath) {
        std::vector<std::string> candidates;
        
        // Full import path
        std::string fullImport;
        for (size_t i = 0; i < importPath.size(); i++) {
            if (i > 0) fullImport += ".";
            fullImport += importPath[i];
        }
        
        // Try absolute path first
        candidates.push_back(fullImport);
        
        // Try relative to current module
        if (!currentModule.empty()) {
            std::vector<std::string> parts;
            size_t start = 0;
            size_t end = currentModule.find('.');
            
            while (end != std::string::npos) {
                parts.push_back(currentModule.substr(start, end - start));
                start = end + 1;
                end = currentModule.find('.', start);
            }
            parts.push_back(currentModule.substr(start));
            
            // Try each level up
            while (!parts.empty()) {
                std::string base;
                for (size_t i = 0; i < parts.size(); i++) {
                    if (i > 0) base += ".";
                    base += parts[i];
                }
                candidates.push_back(base + "." + fullImport);
                parts.pop_back();
            }
        }
        
        return candidates;
    }
    
    // Check if a symbol is accessible
    static bool canAccess(const Symbol& symbol, const std::string& fromModule) {
        // For now, all pub symbols are accessible
        // Could add package-private, etc. later
        return true;
    }
};

// Import resolver - resolves all imports for a module
class ImportResolver {
public:
    struct ResolveResult {
        bool success;
        std::string error;
        std::map<std::string, ModulePtr> resolvedModules;
    };
    
    ResolveResult resolve(ModulePtr module) {
        ResolveResult result;
        result.success = true;
        
        auto& registry = ModuleRegistry::instance();
        
        for (const auto& usingDecl : module->ast.usings) {
            // Convert using path to module name
            std::string importPath;
            for (size_t i = 0; i < usingDecl.path.size(); i++) {
                if (i > 0) importPath += ".";
                importPath += usingDecl.path[i];
            }
            
            // Skip built-in Std modules - they're always available
            if (usingDecl.path[0] == "Std") {
                auto found = registry.findModule(importPath);
                if (found) {
                    result.resolvedModules[importPath] = found;
                    module->imports.push_back(found->name);
                }
                continue;
            }
            
            // Try to resolve
            auto candidates = ModuleResolver::resolveImportPath(module->name, usingDecl.path);
            
            ModulePtr found = nullptr;
            for (const auto& candidate : candidates) {
                found = registry.findModule(candidate);
                if (found) break;
            }
            
            if (!found) {
                result.success = false;
                result.error = "Cannot find module '" + importPath + "' imported in " + module->name;
                return result;
            }
            
            result.resolvedModules[importPath] = found;
            module->imports.push_back(found->name);
        }
        
        return result;
    }
};

// Scope for name resolution
class Scope {
public:
    Scope(Scope* parent = nullptr) : parent(parent) {}
    
    void define(const std::string& name, const std::string& type, void* decl = nullptr) {
        symbols[name] = {type, decl};
    }
    
    bool lookup(const std::string& name, std::string* outType = nullptr, void** outDecl = nullptr) {
        auto it = symbols.find(name);
        if (it != symbols.end()) {
            if (outType) *outType = it->second.type;
            if (outDecl) *outDecl = it->second.declaration;
            return true;
        }
        
        if (parent) {
            return parent->lookup(name, outType, outDecl);
        }
        
        return false;
    }
    
    Scope* getParent() { return parent; }
    
private:
    struct ScopeSymbol {
        std::string type;
        void* declaration;
    };
    
    Scope* parent;
    std::map<std::string, ScopeSymbol> symbols;
};

// Name resolver - resolves all names in a module
class NameResolver {
public:
    struct ResolveResult {
        bool success;
        std::vector<std::string> errors;
    };
    
    ResolveResult resolve(ModulePtr module) {
        ResolveResult result;
        result.success = true;
        
        currentModule = module;
        globalScope = std::make_unique<Scope>();
        
        // Register imported symbols
        auto& registry = ModuleRegistry::instance();
        for (const auto& importName : module->imports) {
            auto imported = registry.findModule(importName);
            if (!imported) {
                // DEBUG: Module not found
                result.errors.push_back("DEBUG: Could not find imported module: " + importName);
                continue;
            }
            
            // DEBUG: Show what we're importing
            // std::cerr << "  DEBUG: Importing from " << importName 
            //           << " (" << imported->ast.functions.size() << " functions)\n";
            
            // Add all public functions from imported module
            for (auto& fn : imported->ast.functions) {
                if (fn.isPublic) {
                    globalScope->define(fn.name, "function", &fn);
                    // std::cerr << "    Added function: " << fn.name << "\n";
                }
            }
            
            // Add all classes from imported module
            for (auto& cls : imported->ast.classes) {
                globalScope->define(cls.name, "class", &cls);
            }
            
            // Also register the module name itself as a namespace
            // Extract the last part of the module name (e.g., "testlib" from "testlib")
            size_t lastDot = importName.rfind('.');
            std::string shortName = (lastDot != std::string::npos) 
                ? importName.substr(lastDot + 1) 
                : importName;
            globalScope->define(shortName, "module", imported.get());
        }
        
        // Add built-in Std functions (from codegen)
        defineStdFunctions();
        
        // Register module's own functions and classes
        for (auto& fn : module->ast.functions) {
            globalScope->define(fn.name, "function", &fn);
        }
        
        for (auto& cls : module->ast.classes) {
            globalScope->define(cls.name, "class", &cls);
        }
        
        // Resolve all function bodies
        for (auto& fn : module->ast.functions) {
            auto fnScope = std::make_unique<Scope>(globalScope.get());
            
            // Add parameters to scope
            for (const auto& param : fn.params) {
                fnScope->define(param.name, "parameter");
            }
            
            for (const auto& stmt : fn.body) {
                resolveStmt(stmt, fnScope.get(), result);
            }
        }
        
        return result;
    }
    
private:
    ModulePtr currentModule;
    std::unique_ptr<Scope> globalScope;
    
    void defineStdFunctions() {
        // Built-in functions from Std library
        globalScope->define("print", "function");
        globalScope->define("println", "function");
        globalScope->define("readLine", "function");
        globalScope->define("parseInt", "function");
        globalScope->define("parseFloat", "function");
        
        // Register Std as a namespace
        globalScope->define("Std", "namespace");
    }
    
    void resolveStmt(const StmtPtr& stmt, Scope* scope, ResolveResult& result) {
        std::visit([&](auto&& s) {
            using T = std::decay_t<decltype(s)>;
            
            if constexpr (std::is_same_v<T, LetStmt>) {
                resolveExpr(s.init, scope, result);
                scope->define(s.name, "variable");
            } else if constexpr (std::is_same_v<T, ReturnStmt>) {
                if (s.value) resolveExpr(s.value, scope, result);
            } else if constexpr (std::is_same_v<T, ExprStmt>) {
                resolveExpr(s.expr, scope, result);
            } else if constexpr (std::is_same_v<T, IfStmt>) {
                resolveExpr(s.cond, scope, result);
                auto thenScope = std::make_unique<Scope>(scope);
                for (const auto& st : s.thenBody) resolveStmt(st, thenScope.get(), result);
                if (!s.elseBody.empty()) {
                    auto elseScope = std::make_unique<Scope>(scope);
                    for (const auto& st : s.elseBody) resolveStmt(st, elseScope.get(), result);
                }
            } else if constexpr (std::is_same_v<T, WhileStmt>) {
                resolveExpr(s.cond, scope, result);
                auto bodyScope = std::make_unique<Scope>(scope);
                for (const auto& st : s.body) resolveStmt(st, bodyScope.get(), result);
            } else if constexpr (std::is_same_v<T, ForStmt>) {
                resolveExpr(s.iterable, scope, result);
                auto bodyScope = std::make_unique<Scope>(scope);
                bodyScope->define(s.var, "variable");
                for (const auto& st : s.body) resolveStmt(st, bodyScope.get(), result);
            } else if constexpr (std::is_same_v<T, MatchStmt>) {
                resolveExpr(s.expr, scope, result);
                for (const auto& arm : s.arms) {
                    auto armScope = std::make_unique<Scope>(scope);
                    if (!arm.bindVar.empty()) {
                        armScope->define(arm.bindVar, "variable");
                    }
                    for (const auto& st : arm.body) resolveStmt(st, armScope.get(), result);
                }
            } else if constexpr (std::is_same_v<T, BlockStmt>) {
                auto blockScope = std::make_unique<Scope>(scope);
                for (const auto& st : s.stmts) resolveStmt(st, blockScope.get(), result);
            }
        }, stmt->data);
    }
    
    void resolveExpr(const ExprPtr& expr, Scope* scope, ResolveResult& result) {
        std::visit([&](auto&& e) {
            using T = std::decay_t<decltype(e)>;
            
            if constexpr (std::is_same_v<T, IdentExpr>) {
                if (!scope->lookup(e.name)) {
                    result.errors.push_back("Undefined identifier: " + e.name);
                    result.success = false;
                }
            } else if constexpr (std::is_same_v<T, BinaryExpr>) {
                resolveExpr(e.left, scope, result);
                resolveExpr(e.right, scope, result);
            } else if constexpr (std::is_same_v<T, UnaryExpr>) {
                resolveExpr(e.operand, scope, result);
            } else if constexpr (std::is_same_v<T, CallExpr>) {
                resolveExpr(e.callee, scope, result);
                for (const auto& arg : e.args) resolveExpr(arg, scope, result);
            } else if constexpr (std::is_same_v<T, MemberExpr>) {
                // For member access, check if object is a namespace (like Std)
                if (auto* ident = std::get_if<IdentExpr>(&e.object->data)) {
                    if (ident->name == "Std") {
                        // Std.print, Std.readLine, etc. are always valid
                        // Don't check further - the codegen handles this
                        return;
                    }
                }
                resolveExpr(e.object, scope, result);
                // Member access is checked at runtime/codegen
            } else if constexpr (std::is_same_v<T, IndexExpr>) {
                resolveExpr(e.object, scope, result);
                resolveExpr(e.index, scope, result);
            } else if constexpr (std::is_same_v<T, LambdaExpr>) {
                auto lambdaScope = std::make_unique<Scope>(scope);
                for (const auto& param : e.params) {
                    lambdaScope->define(param.name, "parameter");
                }
                for (const auto& st : e.body) resolveStmt(st, lambdaScope.get(), result);
            } else if constexpr (std::is_same_v<T, NewExpr>) {
                if (!scope->lookup(e.className)) {
                    result.errors.push_back("Undefined class: " + e.className);
                    result.success = false;
                }
                for (const auto& arg : e.args) resolveExpr(arg, scope, result);
            } else if constexpr (std::is_same_v<T, SomeExpr>) {
                resolveExpr(e.value, scope, result);
            } else if constexpr (std::is_same_v<T, ArrayExpr>) {
                for (const auto& elem : e.elements) resolveExpr(elem, scope, result);
            }
            // Literals don't need resolution
        }, expr->data);
    }
};
