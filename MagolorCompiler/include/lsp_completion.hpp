#pragma once

#include <string>
#include <vector>
#include "lsp_semantic.hpp"

// Forward declaration for JSON
class JsonValue;

// Completion item kinds (matching LSP spec)
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

// Builtin snippets
struct CompletionSnippet {
    std::string label;
    std::string insertText;
    std::string detail;
    std::string documentation;
};

// Completion provider
class CompletionProvider {
public:
    CompletionProvider(SemanticAnalyzer& analyzer) : analyzer(analyzer) {}
    
    // Main entry point
    JsonValue provideCompletions(const std::string& uri, Position pos, const std::string& lineText);
    
private:
    // Snippet helpers
    static std::vector<CompletionSnippet> getBuiltinSnippets();
    static std::vector<std::string> getKeywords();
    
    // Completion generators
    void addStdLibCompletions(JsonValue& items, const std::string& context);
    void addModuleCompletions(JsonValue& items, const std::string& uri);
    void addImportedSymbols(JsonValue& items, const std::string& uri, const std::string& filter);
    void addCallableSymbols(JsonValue& items, const std::string& uri, const std::string& filter);
    void addVariableSymbols(JsonValue& items, const std::string& uri, Position pos, const std::string& filter);
    void addEnumCompletions(JsonValue& items, const std::string& enumName, const std::string& uri);
    void addClassMemberCompletions(JsonValue& items, const std::string& className, const std::string& uri);
    
    // CImport completions
    void addCImportCompletions(JsonValue& items, const std::string& uri, const std::string& filter);
    void addCHeaderCompletions(JsonValue& items, const std::string& headerPath, const std::string& filter);
    void addCFunctionCompletions(JsonValue& items, const std::string& headerPath, const std::string& filter);
    void addCStructCompletions(JsonValue& items, const std::string& headerPath, const std::string& filter);
    void addCEnumCompletions(JsonValue& items, const std::string& headerPath, const std::string& filter);
    void addCMacroCompletions(JsonValue& items, const std::string& headerPath, const std::string& filter);
    
    // Helpers
    static bool matchesFilter(const std::string& name, const std::string& filter);
    std::vector<std::string> getCImportsFromFile(const std::string& uri);
    
    SemanticAnalyzer& analyzer;
};
