// src/lsp_completion.cpp
#include "lsp_completion.hpp"
#include "lsp_semantic.hpp"
#include "stdlib_parser.hpp"
#include <algorithm>
#include <set>
#include "lsp_project.hpp"
// Cache the parsed stdlib functions (parse once at startup)
static std::vector<StdLibFunction> g_stdlibFunctions;
static bool g_stdlibParsed = false;
// ============================================================================
// CompletionProvider Implementation
// ============================================================================

std::vector<CompletionSnippet> CompletionProvider::getBuiltinSnippets() {
    return {
        {"fn", "fn ${1:name}(${2:params}) -> ${3:void} {\n\t${0}\n}", 
         "Function declaration", "Create a new function"},
        {"main", "fn main() {\n\t${0}\n}", "Main function", "Entry point"},
        {"class", "class ${1:Name} {\n\tpub ${2:field}: ${3:int};\n\t\n\tpub fn create() {\n\t\t${0}\n\t}\n}", 
         "Class definition", "Create a class"},
        {"if", "if (${1:condition}) {\n\t${0}\n}", "If statement", ""},
        {"ife", "if (${1:condition}) {\n\t${2}\n} else {\n\t${0}\n}", "If-else", ""},
        {"while", "while (${1:condition}) {\n\t${0}\n}", "While loop", ""},
        {"for", "for (${1:item} in ${2:array}) {\n\t${0}\n}", "For loop", ""},
        {"match", "match ${1:value} {\n\tSome(${2:v}) => {\n\t\t${3}\n\t},\n\tNone => {\n\t\t${0}\n\t}\n}", 
         "Match expression", "Pattern matching"},
        {"let", "let ${1:mut }${2:name} = ${0:value};", "Variable", ""},
        {"lett", "let ${1:mut }${2:name}: ${3:type} = ${0:value};", "Typed variable", ""},
        {"using", "using ${1:Std.IO};", "Import", ""},
        {"cpp", "@cpp {\n\t${0}\n}", "C++ block", "Inline C++"},
        {"pub", "pub fn ${1:name}(${2:params}) -> ${3:void} {\n\t${0}\n}", "Public function", ""},
        {"test", "test(\"${1:name}\", fn() -> bool {\n\t${0}\n\treturn true;\n});", "Test case", ""},
    };
}

std::vector<std::string> CompletionProvider::getKeywords() {
    return {
        "fn", "let", "mut", "return", "if", "else", "while", "for", "match",
        "class", "new", "this", "true", "false", "None", "Some", "using",
        "pub", "priv", "static", "cimport", "int", "float", "string", "bool", "void"
    };
}

bool CompletionProvider::matchesFilter(const std::string& name, const std::string& filter) {
    if (filter.empty()) return true;
    if (name.size() < filter.size()) return false;
    
    for (size_t i = 0; i < filter.size(); i++) {
        if (std::tolower(name[i]) != std::tolower(filter[i])) {
            return false;
        }
    }
    return true;
}

void CompletionProvider::addStdLibCompletions(JsonValue& items, const std::string& context) {
    std::string modulePath;
    
    if (context.find("Std.") != std::string::npos) {
        size_t stdPos = context.rfind("Std.");
        size_t endPos = context.find_first_of(".(: ", stdPos + 4);
        if (endPos == std::string::npos) endPos = context.length();
        
        modulePath = context.substr(stdPos, endPos - stdPos);
    }
    
    if (modulePath == "Std" || (context.find("Std.") != std::string::npos && 
                                 context.back() == '.')) {
        auto modules = analyzer.getAvailableStdModules();
        for (const auto& mod : modules) {
            std::string shortName = mod.substr(4);
            
            JsonValue item = JsonValue::object();
            item["label"] = shortName;
            item["kind"] = (int)CompletionItemKind::Module;
            item["detail"] = mod;
            item["sortText"] = "0_" + shortName;
            items.push(item);
        }
        return;
    }
    
    if (!modulePath.empty() && analyzer.isStdLibModule(modulePath)) {
        auto symbols = analyzer.getStdLibSymbols(modulePath);
        
        for (const auto& sym : symbols) {
            JsonValue item = JsonValue::object();
            item["label"] = sym->name;
            
            if (sym->kind == SymbolKind::Constant) {
                item["kind"] = (int)CompletionItemKind::Constant;
            } else if (sym->kind == SymbolKind::Function) {
                item["kind"] = (int)CompletionItemKind::Function;
            } else if (sym->kind == SymbolKind::Class) {
                item["kind"] = (int)CompletionItemKind::Class;
            }
            
            if (!sym->detail.empty()) {
                item["detail"] = sym->detail;
            }
            
            item["documentation"] = "From " + modulePath;
            item["sortText"] = "1_" + sym->name;
            
            items.push(item);
        }
    }
}

void CompletionProvider::addImportedSymbols(JsonValue& items, const std::string& uri, const std::string& filter) {
    auto symbols = analyzer.resolveImportedSymbols(uri);
    
    for (const auto& sym : symbols) {
        if (!matchesFilter(sym->name, filter)) continue;
        
        JsonValue item = JsonValue::object();
        item["label"] = sym->name;
        
        if (sym->isCallable) {
            item["kind"] = (int)CompletionItemKind::Function;
            item["insertText"] = sym->name + "($0)";
            item["insertTextFormat"] = 2;
        } else if (sym->kind == SymbolKind::Constant) {
            item["kind"] = (int)CompletionItemKind::Constant;
        } else if (sym->kind == SymbolKind::Class) {
            item["kind"] = (int)CompletionItemKind::Class;
        } else {
            item["kind"] = (int)CompletionItemKind::Variable;
        }
        
        if (!sym->detail.empty()) {
            item["detail"] = sym->detail;
        }
        
        if (!sym->modulePath.empty()) {
            item["documentation"] = "From " + sym->modulePath;
        }
        
        item["sortText"] = "2_" + sym->name;
        items.push(item);
    }
}

void CompletionProvider::addModuleCompletions(JsonValue& items, const std::string& uri) {
    auto modules = analyzer.getAvailableStdModules();
    
    for (const auto& mod : modules) {
        JsonValue item = JsonValue::object();
        item["label"] = mod;
        item["kind"] = (int)CompletionItemKind::Module;
        item["detail"] = "Standard library module";
        item["sortText"] = "0_" + mod;
        items.push(item);
    }
}

void CompletionProvider::addCallableSymbols(JsonValue& items, const std::string& uri, const std::string& filter) {
    auto symbols = analyzer.getCallableSymbols(uri);
    
    for (const auto& sym : symbols) {
        if (!matchesFilter(sym->name, filter)) continue;
        
        JsonValue item = JsonValue::object();
        item["label"] = sym->name;
        item["kind"] = (int)CompletionItemKind::Function;
        item["insertText"] = sym->name + "($0)";
        item["insertTextFormat"] = 2;
        
        if (!sym->detail.empty()) {
            item["detail"] = sym->detail;
        }
        
        item["sortText"] = "3_" + sym->name;
        items.push(item);
    }
}

void CompletionProvider::addVariableSymbols(JsonValue& items, const std::string& uri, Position pos, const std::string& filter) {
    auto symbols = analyzer.getVariablesInScope(uri, pos);
    
    for (const auto& sym : symbols) {
        if (!matchesFilter(sym->name, filter)) continue;
        
        JsonValue item = JsonValue::object();
        item["label"] = sym->name;
        item["kind"] = (int)CompletionItemKind::Variable;
        
        if (!sym->type.empty()) {
            item["detail"] = sym->type;
        }
        
        item["sortText"] = "4_" + sym->name;
        items.push(item);
    }
}

JsonValue CompletionProvider::provideCompletions(const std::string& uri, Position pos, const std::string& lineText) {
    JsonValue items = JsonValue::array();
    
    size_t cursorCol = (size_t)pos.character;
    std::string beforeCursor = (cursorCol <= lineText.size()) ? lineText.substr(0, cursorCol) : lineText;
    
    std::string prefix;
    for (int i = (int)beforeCursor.size() - 1; i >= 0; i--) {
        char c = beforeCursor[i];
        if (std::isalnum(c) || c == '_') {
            prefix = c + prefix;
        } else {
            break;
        }
    }
    
    if (beforeCursor.find("using ") != std::string::npos && 
        beforeCursor.find(';') == std::string::npos) {
        addModuleCompletions(items, uri);
        return items;
    }
    
    if (beforeCursor.find("Std.") != std::string::npos) {
        addStdLibCompletions(items, beforeCursor);
        return items;
    }
    
    auto keywords = getKeywords();
    for (const auto& kw : keywords) {
        if (!matchesFilter(kw, prefix)) continue;
        
        JsonValue item = JsonValue::object();
        item["label"] = kw;
        item["kind"] = (int)CompletionItemKind::Keyword;
        item["sortText"] = "9_" + kw;
        items.push(item);
    }
    
    auto snippets = getBuiltinSnippets();
    for (const auto& snip : snippets) {
        if (!matchesFilter(snip.label, prefix)) continue;
        
        JsonValue item = JsonValue::object();
        item["label"] = snip.label;
        item["kind"] = (int)CompletionItemKind::Snippet;
        item["insertText"] = snip.insertText;
        item["insertTextFormat"] = 2;
        item["detail"] = snip.detail;
        if (!snip.documentation.empty()) {
            item["documentation"] = snip.documentation;
        }
        item["sortText"] = "8_" + snip.label;
        items.push(item);
    }
    
    addImportedSymbols(items, uri, prefix);
    addCallableSymbols(items, uri, prefix);
    addVariableSymbols(items, uri, pos, prefix);
    
    return items;
}
