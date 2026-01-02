#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "position.hpp"

// Forward declarations
struct Symbol;
struct Scope;
using SymbolPtr = std::shared_ptr<Symbol>;

enum class SymbolKind {
    Function = 12,
    Variable = 13,
    Class = 5,
    Field = 8,
    Method = 6,
    Parameter = 25,
    Module = 2
};

struct Symbol {
    std::string name;
    SymbolKind kind;
    std::string type;
    Location definition;
    std::vector<Location> references;
    std::string documentation;
    std::string detail;
    bool isPublic = true;
    bool isStatic = false;
    bool isCallable = false;
    std::string containerName;
    std::vector<std::string> paramTypes;
    std::string returnType;
};

struct ImportedModule {
    std::string fullPath;  // e.g., "Std.IO" or "MagolorDotDev.models.Package"
    std::vector<std::string> importedSymbols;  // Symbols available from this import
};

struct Scope {
    std::unordered_map<std::string, SymbolPtr> symbols;
    std::vector<ImportedModule> imports;
    Scope* parent = nullptr;
    
    SymbolPtr lookup(const std::string& name) {
        auto it = symbols.find(name);
        if (it != symbols.end()) return it->second;
        if (parent) return parent->lookup(name);
        return nullptr;
    }
    
    void define(SymbolPtr sym) {
        symbols[sym->name] = sym;
    }
    
    bool hasImport(const std::string& modulePath) {
        for (const auto& imp : imports) {
            if (imp.fullPath == modulePath) return true;
        }
        if (parent) return parent->hasImport(modulePath);
        return false;
    }
};

class SemanticAnalyzer {
public:
    void analyze(const std::string& uri, const std::string& content);
    std::vector<SymbolPtr> getCallableSymbols(const std::string& uri);
    std::vector<SymbolPtr> getVariablesInScope(const std::string& uri, Position pos);
    SymbolPtr getSymbolAt(const std::string& uri, Position pos);
    std::vector<SymbolPtr> getAllSymbolsInFile(const std::string& uri);
    std::vector<std::string> getImportedModules(const std::string& uri);
    std::vector<SymbolPtr> getSymbolsFromModule(const std::string& modulePath);
    std::vector<SymbolPtr> resolveImportedSymbols(const std::string& uri);
    SymbolPtr findSymbolInImports(const std::string& uri, const std::string& symbolName);
    
    // Project-wide loading
    void loadProject(const std::string& startUri);
    void scanSourceDirectory(const std::string& srcDir);
    void reloadProject();
    
    // Import validation
    struct ImportError {
        std::string modulePath;
        std::string message;
        Range range;
    };
    std::vector<ImportError> validateImports(const std::string& uri);

private:
    std::unordered_map<std::string, std::vector<SymbolPtr>> fileSymbols;
    std::unordered_map<std::string, std::shared_ptr<Scope>> fileScopes;
    std::unordered_map<std::string, std::vector<SymbolPtr>> moduleSymbols;
    
    // Project state
    bool projectLoaded = false;
    std::string projectRoot;
    
    void extractSymbols(const std::string& uri, const std::string& content);
    SymbolPtr parseFunction(const std::string& line, int lineNum, const std::string& uri);
    SymbolPtr parseClass(const std::string& line, int lineNum, const std::string& uri);
    SymbolPtr parseVariable(const std::string& line, int lineNum, const std::string& uri);
    std::string extractType(const std::string& declaration);
    void parseImport(const std::string& line, Scope* scope);
};
