#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

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

struct Position {
    int line = 0;
    int character = 0;
};

struct Range {
    Position start;
    Position end;
};

struct Location {
    std::string uri;
    Range range;
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
    bool isCallable = false;  // NEW: Track if this is actually callable
    std::string containerName;
    std::vector<std::string> paramTypes;  // NEW: For functions
    std::string returnType;  // NEW: For functions
};

struct ImportedModule {
    std::string fullPath;  // e.g., "Std.IO"
    std::vector<std::string> importedSymbols;  // Symbols available from this import
};

struct Scope {
    std::unordered_map<std::string, SymbolPtr> symbols;
    std::vector<ImportedModule> imports;  // Track what's been imported
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
    std::vector<std::string> getImportedModules(const std::string& uri);  // NEW
    
private:
    std::unordered_map<std::string, std::vector<SymbolPtr>> fileSymbols;
    std::unordered_map<std::string, std::shared_ptr<Scope>> fileScopes;
    
    void extractSymbols(const std::string& uri, const std::string& content);
    SymbolPtr parseFunction(const std::string& line, int lineNum, const std::string& uri);
    SymbolPtr parseClass(const std::string& line, int lineNum, const std::string& uri);
    SymbolPtr parseVariable(const std::string& line, int lineNum, const std::string& uri);
    std::string extractType(const std::string& declaration);
    void parseImport(const std::string& line, Scope* scope);  // NEW
};
