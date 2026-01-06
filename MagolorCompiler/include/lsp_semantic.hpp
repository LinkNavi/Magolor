#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include "position.hpp"
#include "stdlib_loader.hpp"

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
    Module = 2,
    Constant = 21
};

struct Symbol {
    std::string name;
    SymbolKind kind;
    std::string type;
    Location definition;
    std::vector<Location> references;
    std::string documentation;
    std::string detail;          // Signature for functions
    bool isPublic = true;
    bool isStatic = false;
    bool isCallable = false;
    std::string containerName;   // Class name for methods
    std::vector<std::string> paramTypes;
    std::string returnType;
    std::string modulePath;      // For stdlib symbols
};

struct ImportedModule {
    std::string fullPath;
    std::vector<std::string> importedSymbols;
    bool isStdLib = false;
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
    SemanticAnalyzer();
    
    void analyze(const std::string& uri, const std::string& content);
    
    // Symbol queries
    std::vector<SymbolPtr> getCallableSymbols(const std::string& uri);
    std::vector<SymbolPtr> getVariablesInScope(const std::string& uri, Position pos);
    SymbolPtr getSymbolAt(const std::string& uri, Position pos);
    std::vector<SymbolPtr> getAllSymbolsInFile(const std::string& uri);
    
    // Import handling
    std::vector<std::string> getImportedModules(const std::string& uri);
    std::vector<SymbolPtr> getSymbolsFromModule(const std::string& modulePath);
    std::vector<SymbolPtr> resolveImportedSymbols(const std::string& uri);
    SymbolPtr findSymbolInImports(const std::string& uri, const std::string& symbolName);
    
    // Stdlib queries (dynamic)
    std::vector<SymbolPtr> getStdLibSymbols(const std::string& modulePath);
    bool isStdLibModule(const std::string& modulePath) const;
    std::vector<std::string> getAvailableStdModules() const;
    
    // Project-wide
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
    std::unordered_map<std::string, std::vector<SymbolPtr>> moduleSymbolCache;
    std::unordered_map<std::string, std::vector<SymbolPtr>> stdlibSymbolCache;
    
    bool projectLoaded = false;
    std::string projectRoot;
    bool stdlibInitialized = false;
    
    void initStdLib();
    void extractSymbols(const std::string& uri, const std::string& content);
    SymbolPtr parseFunction(const std::string& line, int lineNum, const std::string& uri);
    SymbolPtr parseClass(const std::string& line, int lineNum, const std::string& uri);
    SymbolPtr parseVariable(const std::string& line, int lineNum, const std::string& uri);
    void parseImport(const std::string& line, Scope* scope);
    
    // Convert StdFunction to Symbol
    SymbolPtr stdFunctionToSymbol(const StdFunction& func, const std::string& modulePath);
};

// ============================================================================
// Completion Provider with dynamic stdlib
// ============================================================================

enum class CompletionItemKind {
    Text = 1,
    Method = 2,
    Function = 3,
    Constructor = 4,
    Field = 5,
    Variable = 6,
    Class = 7,
    Interface = 8,
    Module = 9,
    Property = 10,
    Unit = 11,
    Value = 12,
    Enum = 13,
    Keyword = 14,
    Snippet = 15,
    Color = 16,
    File = 17,
    Reference = 18,
    Folder = 19,
    EnumMember = 20,
    Constant = 21,
    Struct = 22,
    Event = 23,
    Operator = 24,
    TypeParameter = 25
};

struct CompletionSnippet {
    std::string label;
    std::string insertText;
    std::string detail;
    std::string documentation;
};

class JsonValue;  // Forward declare

class CompletionProvider {
public:
    CompletionProvider(SemanticAnalyzer& analyzer) : analyzer(analyzer) {}
    
    JsonValue provideCompletions(const std::string& uri, Position pos, const std::string& lineText);
    
private:
    SemanticAnalyzer& analyzer;
    
    std::vector<CompletionSnippet> getBuiltinSnippets();
    std::vector<std::string> getKeywords();
    
    void addStdLibCompletions(JsonValue& items, const std::string& context);
    void addImportedSymbols(JsonValue& items, const std::string& uri, const std::string& filter);
    void addModuleCompletions(JsonValue& items, const std::string& uri);
    void addCallableSymbols(JsonValue& items, const std::string& uri, const std::string& filter);
    void addVariableSymbols(JsonValue& items, const std::string& uri, Position pos, const std::string& filter);
    
    bool matchesFilter(const std::string& name, const std::string& filter);
};
