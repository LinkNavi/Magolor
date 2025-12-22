#pragma once
#include "lsp_semantic.hpp"
#include "jsonrpc.hpp"
#include <vector>
#include <string>

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

class CompletionProvider {
public:
    CompletionProvider(SemanticAnalyzer& analyzer) : analyzer(analyzer) {}
    
    JsonValue provideCompletions(const std::string& uri, Position pos, const std::string& lineText);
    
private:
    SemanticAnalyzer& analyzer;
    
    std::vector<CompletionSnippet> getBuiltinSnippets();
    std::vector<std::string> getKeywords();
    void addStdLibCompletions(JsonValue& items, const std::string& context);
    void addCallableSymbols(JsonValue& items, const std::string& uri, const std::string& filter);
    void addVariableSymbols(JsonValue& items, const std::string& uri, Position pos, const std::string& filter);
    bool matchesFilter(const std::string& name, const std::string& filter);
};
