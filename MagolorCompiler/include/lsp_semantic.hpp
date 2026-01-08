#pragma once

#include "position.hpp"
#include "stdlib_loader.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Symbol kinds (matching LSP spec)
enum class SymbolKind {
  File = 1,
  Module = 2,
  Namespace = 3,
  Package = 4,
  Class = 5,
  Method = 6,
  Property = 7,
  Field = 8,
  Constructor = 9,
  Enum = 10,
  Interface = 11,
  Function = 12,
  Variable = 13,
  Constant = 14,
  String = 15,
  Number = 16,
  Boolean = 17,
  Array = 18,
  Object = 19,
  Key = 20,
  Null = 21,
  EnumMember = 22,
  Struct = 23,
  Event = 24,
  Operator = 25,
  TypeParameter = 26,
  Parameter = 100 // Custom
};

// Represents a symbol in the code
struct Symbol {
  std::string name;
  SymbolKind kind = SymbolKind::Variable;
  std::string type;
  std::string detail;
  std::string documentation;
  std::string returnType;
  std::string containerName;
  std::string modulePath;
  std::vector<std::string> paramTypes;
  Location definition;
  std::vector<Location> references;
  bool isPublic = false;
  bool isStatic = false;
  bool isCallable = false;
};

using SymbolPtr = std::shared_ptr<Symbol>;

// Import tracking
struct ImportedModule {
  std::string fullPath;
  std::vector<std::string> importedSymbols;
  bool isStdLib = false;
};

// Scope for symbol resolution
class Scope {
public:
  Scope *parent = nullptr;
  std::unordered_map<std::string, SymbolPtr> symbols;
  std::vector<ImportedModule> imports;

  void define(SymbolPtr sym) { symbols[sym->name] = sym; }

  SymbolPtr lookup(const std::string &name) {
    auto it = symbols.find(name);
    if (it != symbols.end())
      return it->second;
    if (parent)
      return parent->lookup(name);
    return nullptr;
  }
};

// Main semantic analyzer
class SemanticAnalyzer {
public:
  SemanticAnalyzer();

  const std::unordered_map<std::string, std::vector<SymbolPtr>> &
  getFileSymbols() const {
    return fileSymbols;
  }
  // Analysis
  void analyze(const std::string &uri, const std::string &content);
  void reloadProject();

  // Symbol queries
  SymbolPtr getSymbolAt(const std::string &uri, Position pos);
  std::vector<SymbolPtr> getAllSymbolsInFile(const std::string &uri);
  std::vector<SymbolPtr> getCallableSymbols(const std::string &uri);
  std::vector<SymbolPtr> getVariablesInScope(const std::string &uri,
                                             Position pos);

  // Import resolution
  std::vector<std::string> getImportedModules(const std::string &uri);
  std::vector<SymbolPtr> resolveImportedSymbols(const std::string &uri);
  SymbolPtr findSymbolInImports(const std::string &uri,
                                const std::string &symbolName);

  // Stdlib
  bool isStdLibModule(const std::string &modulePath) const;
  std::vector<std::string> getAvailableStdModules() const;
  std::vector<SymbolPtr> getStdLibSymbols(const std::string &modulePath);

  // Module queries
  std::vector<SymbolPtr> getSymbolsFromModule(const std::string &modulePath);

  // CImport queries
  std::vector<std::string> getCImports(const std::string &uri) const;

  // Validation
  struct ImportError {
    std::string modulePath;
    std::string message;
    Range range;
  };
  std::vector<ImportError> validateImports(const std::string &uri);

private:
  void initStdLib();
  void loadProject(const std::string &startUri);
  void scanSourceDirectory(const std::string &srcDir);
  void extractSymbols(const std::string &uri, const std::string &content);
  void extractMetadataDirectives(const std::string &uri,
                                 const std::string &content, Scope *scope);

  // Parsing helpers
  void parseImport(const std::string &line, Scope *scope);
  SymbolPtr parseFunction(const std::string &line, int lineNum,
                          const std::string &uri);
  SymbolPtr parseClass(const std::string &line, int lineNum,
                       const std::string &uri);
  SymbolPtr parseVariable(const std::string &line, int lineNum,
                          const std::string &uri);
  SymbolPtr parseEnum(const std::string &line, int lineNum,
                      const std::string &uri);

  // Stdlib conversion
  SymbolPtr stdFunctionToSymbol(const StdFunction &func,
                                const std::string &modulePath);
  SymbolPtr stdClassToSymbol(const StdClass &cls,
                             const std::string &modulePath);
  SymbolPtr stdEnumToSymbol(const StdEnum &enumDef,
                            const std::string &modulePath);

  // State
  bool stdlibInitialized = false;
  bool projectLoaded = false;
  std::string projectRoot;

  std::unordered_map<std::string, std::vector<SymbolPtr>> fileSymbols;
  std::unordered_map<std::string, std::shared_ptr<Scope>> fileScopes;
  std::unordered_map<std::string, std::vector<SymbolPtr>> stdlibSymbolCache;
  std::unordered_map<std::string, std::vector<SymbolPtr>> moduleSymbolCache;
  std::unordered_map<std::string, std::vector<std::string>> fileCImports;
};
