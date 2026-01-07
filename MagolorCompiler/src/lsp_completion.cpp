#include "lsp_completion.hpp"
#include "stdlib_loader.hpp"
#include <algorithm>
#include <sstream>

// Simple JSON builder (assumes JsonValue is defined elsewhere, or inline here)
struct JsonValue {
    enum Type { Null, Bool, Int, Double, String, Array, Object };
    Type type = Null;
    bool boolVal = false;
    int64_t intVal = 0;
    double doubleVal = 0;
    std::string strVal;
    std::vector<JsonValue> arrayVal;
    std::vector<std::pair<std::string, JsonValue>> objectVal;

    JsonValue() : type(Null) {}
    JsonValue(bool b) : type(Bool), boolVal(b) {}
    JsonValue(int i) : type(Int), intVal(i) {}
    JsonValue(int64_t i) : type(Int), intVal(i) {}
    JsonValue(double d) : type(Double), doubleVal(d) {}
    JsonValue(const char* s) : type(String), strVal(s) {}
    JsonValue(const std::string& s) : type(String), strVal(s) {}

    static JsonValue array() { JsonValue v; v.type = Array; return v; }
    static JsonValue object() { JsonValue v; v.type = Object; return v; }

    void push(const JsonValue& v) { if (type == Array) arrayVal.push_back(v); }
    void set(const std::string& k, const JsonValue& v) { 
        if (type == Object) objectVal.emplace_back(k, v); 
    }

    std::string dump() const {
        switch (type) {
            case Null: return "null";
            case Bool: return boolVal ? "true" : "false";
            case Int: return std::to_string(intVal);
            case Double: return std::to_string(doubleVal);
            case String: {
                std::string out = "\"";
                for (char c : strVal) {
                    if (c == '"') out += "\\\"";
                    else if (c == '\\') out += "\\\\";
                    else if (c == '\n') out += "\\n";
                    else if (c == '\t') out += "\\t";
                    else out += c;
                }
                return out + "\"";
            }
            case Array: {
                std::string out = "[";
                for (size_t i = 0; i < arrayVal.size(); ++i) {
                    if (i > 0) out += ",";
                    out += arrayVal[i].dump();
                }
                return out + "]";
            }
            case Object: {
                std::string out = "{";
                for (size_t i = 0; i < objectVal.size(); ++i) {
                    if (i > 0) out += ",";
                    out += "\"" + objectVal[i].first + "\":" + objectVal[i].second.dump();
                }
                return out + "}";
            }
        }
        return "null";
    }
};

static JsonValue makeCompletionItem(
    const std::string& label,
    CompletionItemKind kind,
    const std::string& detail = "",
    const std::string& doc = "",
    const std::string& insertText = "",
    const std::string& sortText = ""
) {
    JsonValue item = JsonValue::object();
    item.set("label", label);
    item.set("kind", static_cast<int>(kind));
    if (!detail.empty()) item.set("detail", detail);
    if (!doc.empty()) {
        JsonValue docObj = JsonValue::object();
        docObj.set("kind", "markdown");
        docObj.set("value", doc);
        item.set("documentation", docObj);
    }
    if (!insertText.empty()) {
        item.set("insertText", insertText);
        item.set("insertTextFormat", 2); // Snippet format
    }
    if (!sortText.empty()) item.set("sortText", sortText);
    return item;
}

std::vector<CompletionSnippet> CompletionProvider::getBuiltinSnippets() {
    return {
        {"fn", "fn ${1:name}(${2:params}) -> ${3:void} {\n\t$0\n}", "Function", "Define a function"},
        {"pub fn", "pub fn ${1:name}(${2:params}) -> ${3:void} {\n\t$0\n}", "Public function", "Define a public function"},
        {"class", "class ${1:Name} {\n\t$0\n}", "Class", "Define a class"},
        {"pub class", "pub class ${1:Name} {\n\t$0\n}", "Public class", "Define a public class"},
        {"enum", "enum ${1:Name} {\n\t${2:Variant1},\n\t${3:Variant2}\n}", "Enum", "Define an enum"},
        {"if", "if ${1:condition} {\n\t$0\n}", "If statement", "Conditional branch"},
        {"if else", "if ${1:condition} {\n\t$2\n} else {\n\t$0\n}", "If-else", "Conditional with else"},
        {"match", "match ${1:expr} {\n\t${2:pattern} => $0\n}", "Match", "Pattern matching"},
        {"for", "for ${1:item} in ${2:iter} {\n\t$0\n}", "For loop", "Iterate over collection"},
        {"while", "while ${1:condition} {\n\t$0\n}", "While loop", "Loop while condition"},
        {"loop", "loop {\n\t$0\n}", "Infinite loop", "Loop forever"},
        {"let", "let ${1:name}: ${2:Type} = $0;", "Variable", "Declare variable"},
        {"mut", "mut ${1:name}: ${2:Type} = $0;", "Mutable variable", "Declare mutable variable"},
        {"using", "using ${1:Std.Module};", "Import", "Import module"},
        {"return", "return $0;", "Return", "Return from function"},
    };
}

std::vector<std::string> CompletionProvider::getKeywords() {
    return {
        "fn", "pub", "class", "enum", "if", "else", "match", "for", "in",
        "while", "loop", "let", "mut", "return", "break", "continue",
        "using", "true", "false", "null", "self", "static", "const",
        "int", "float", "bool", "string", "void", "char"
    };
}

bool CompletionProvider::matchesFilter(const std::string& name, const std::string& filter) {
    if (filter.empty()) return true;
    std::string lowerName = name, lowerFilter = filter;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), ::tolower);
    return lowerName.find(lowerFilter) == 0;
}

void CompletionProvider::addStdLibCompletions(JsonValue& items, const std::string& context) {
    auto& loader = StdLibLoader::instance();
    auto modules = loader.getAvailableModules();
    
    for (const auto& mod : modules) {
        // Extract short name (e.g., "IO" from "Std.IO")
        std::string shortName = mod;
        auto dot = mod.rfind('.');
        if (dot != std::string::npos) shortName = mod.substr(dot + 1);
        
        if (matchesFilter(shortName, context) || matchesFilter(mod, context)) {
            items.push(makeCompletionItem(
                mod, CompletionItemKind::Module,
                "Standard library module",
                "", "", "0" + mod
            ));
        }
    }
}

void CompletionProvider::addModuleCompletions(JsonValue& items, const std::string& uri) {
    auto& loader = StdLibLoader::instance();
    auto modules = loader.getAvailableModules();
    
    for (const auto& mod : modules) {
        items.push(makeCompletionItem(
            mod, CompletionItemKind::Module,
            "Standard library module",
            "", "", "0" + mod
        ));
    }
}

void CompletionProvider::addImportedSymbols(JsonValue& items, const std::string& uri, const std::string& filter) {
    auto symbols = analyzer.resolveImportedSymbols(uri);
    
    for (const auto& sym : symbols) {
        if (!matchesFilter(sym->name, filter)) continue;
        
        CompletionItemKind kind;
        std::string detail;
        
        switch (sym->kind) {
            case SymbolKind::Function:
                kind = CompletionItemKind::Function;
                detail = sym->type.empty() ? "function" : "fn() -> " + sym->type;
                break;
            case SymbolKind::Class:
                kind = CompletionItemKind::Class;
                detail = "class";
                break;
            case SymbolKind::Enum:
                kind = CompletionItemKind::Enum;
                detail = "enum";
                break;
            case SymbolKind::Variable:
                kind = CompletionItemKind::Variable;
                detail = sym->type;
                break;
            case SymbolKind::Constant:
                kind = CompletionItemKind::Constant;
                detail = sym->type;
                break;
            case SymbolKind::Method:
                kind = CompletionItemKind::Method;
                detail = sym->type;
                break;
            case SymbolKind::Field:
                kind = CompletionItemKind::Field;
                detail = sym->type;
                break;
            default:
                kind = CompletionItemKind::Text;
                detail = "";
        }
        
        items.push(makeCompletionItem(
            sym->name, kind, detail, sym->documentation,
            "", "1" + sym->name
        ));
    }
}

void CompletionProvider::addCallableSymbols(JsonValue& items, const std::string& uri, const std::string& filter) {
    // Add functions from stdlib
    auto& loader = StdLibLoader::instance();
    auto modules = loader.getAvailableModules();
    
    for (const auto& mod : modules) {
        auto funcs = loader.getFunctions(mod);
        for (const auto& fn : funcs) {
            if (!matchesFilter(fn.name, filter)) continue;
            
            std::string sig = fn.name + "(";
            std::string snippet = fn.name + "(";
            int idx = 1;
            for (size_t i = 0; i < fn.paramNames.size(); ++i) {
                if (i > 0) { sig += ", "; snippet += ", "; }
                sig += fn.paramNames[i] + ": " + fn.paramTypes[i];
                snippet += "${" + std::to_string(idx++) + ":" + fn.paramNames[i] + "}";
            }
            sig += ") -> " + fn.returnType;
            snippet += ")$0";
            
            items.push(makeCompletionItem(
                fn.name, CompletionItemKind::Function,
                sig, fn.documentation, snippet, "2" + fn.name
            ));
        }
    }
}

void CompletionProvider::addVariableSymbols(JsonValue& items, const std::string& uri, Position pos, const std::string& filter) {
    auto symbols = analyzer.getVariablesInScope(uri, pos);
    
    for (const auto& sym : symbols) {
        if (!matchesFilter(sym->name, filter)) continue;
        if (sym->kind != SymbolKind::Variable && sym->kind != SymbolKind::Constant) continue;
        
        CompletionItemKind kind = sym->kind == SymbolKind::Constant 
            ? CompletionItemKind::Constant 
            : CompletionItemKind::Variable;
        
        items.push(makeCompletionItem(
            sym->name, kind, sym->type, sym->documentation,
            "", "3" + sym->name
        ));
    }
}

void CompletionProvider::addEnumCompletions(JsonValue& items, const std::string& enumName, const std::string& uri) {
    auto& loader = StdLibLoader::instance();
    auto modules = loader.getAvailableModules();
    
    for (const auto& mod : modules) {
        auto enums = loader.getEnums(mod);
        for (const auto& e : enums) {
            if (e.name != enumName) continue;
            
            for (const auto& variant : e.variants) {
                items.push(makeCompletionItem(
                    variant, CompletionItemKind::EnumMember,
                    enumName + "::" + variant, "",
                    "", "0" + variant
                ));
            }
            return;
        }
    }
}

void CompletionProvider::addClassMemberCompletions(JsonValue& items, const std::string& className, const std::string& uri) {
    auto& loader = StdLibLoader::instance();
    auto modules = loader.getAvailableModules();
    
    for (const auto& mod : modules) {
        auto classes = loader.getClasses(mod);
        for (const auto& cls : classes) {
            if (cls.name != className) continue;
            
            // Add methods
            for (const auto& method : cls.methods) {
                std::string sig = method.name + "(";
                std::string snippet = method.name + "(";
                int idx = 1;
                for (size_t i = 0; i < method.paramNames.size(); ++i) {
                    if (i > 0) { sig += ", "; snippet += ", "; }
                    sig += method.paramNames[i] + ": " + method.paramTypes[i];
                    snippet += "${" + std::to_string(idx++) + ":" + method.paramNames[i] + "}";
                }
                sig += ") -> " + method.returnType;
                snippet += ")$0";
                
                items.push(makeCompletionItem(
                    method.name, CompletionItemKind::Method,
                    sig, method.documentation, snippet, "0" + method.name
                ));
            }
            
            // Add fields (pair<name, type>)
            for (const auto& field : cls.fields) {
                items.push(makeCompletionItem(
                    field.first, CompletionItemKind::Field,
                    field.second, "", "", "1" + field.first
                ));
            }
            return;
        }
    }
}

JsonValue CompletionProvider::provideCompletions(const std::string& uri, Position pos, const std::string& lineText) {
    JsonValue items = JsonValue::array();
    
    // Determine completion context from line
    std::string line = lineText.substr(0, pos.character);
    std::string filter;
    
    // Extract word being typed
    size_t wordStart = line.size();
    while (wordStart > 0 && (std::isalnum(line[wordStart-1]) || line[wordStart-1] == '_')) {
        --wordStart;
    }
    filter = line.substr(wordStart);
    
    // Check for specific contexts
    bool afterDot = wordStart > 0 && line[wordStart-1] == '.';
    bool afterDoubleColon = wordStart > 1 && line.substr(wordStart-2, 2) == "::";
    bool inUsing = line.find("using ") != std::string::npos;
    
    if (inUsing) {
        // Module completions for imports
        addStdLibCompletions(items, filter);
    } else if (afterDoubleColon) {
        // Enum variant completions
        size_t enumStart = wordStart - 2;
        while (enumStart > 0 && (std::isalnum(line[enumStart-1]) || line[enumStart-1] == '_')) {
            --enumStart;
        }
        std::string enumName = line.substr(enumStart, wordStart - 2 - enumStart);
        addEnumCompletions(items, enumName, uri);
    } else if (afterDot) {
        // Member access completions
        size_t objStart = wordStart - 1;
        while (objStart > 0 && (std::isalnum(line[objStart-1]) || line[objStart-1] == '_')) {
            --objStart;
        }
        std::string objName = line.substr(objStart, wordStart - 1 - objStart);
        
        // Try to resolve object type and add members
        auto sym = analyzer.findSymbolInImports(uri, objName);
        if (sym && !sym->type.empty()) {
            addClassMemberCompletions(items, sym->type, uri);
        }
    } else {
        // General completions
        
        // Keywords
        for (const auto& kw : getKeywords()) {
            if (matchesFilter(kw, filter)) {
                items.push(makeCompletionItem(
                    kw, CompletionItemKind::Keyword,
                    "keyword", "", "", "9" + kw
                ));
            }
        }
        
        // Snippets
        for (const auto& snip : getBuiltinSnippets()) {
            if (matchesFilter(snip.label, filter)) {
                items.push(makeCompletionItem(
                    snip.label, CompletionItemKind::Snippet,
                    snip.detail, snip.documentation, snip.insertText, "8" + snip.label
                ));
            }
        }
        
        // Imported symbols
        addImportedSymbols(items, uri, filter);
        
        // Variables in scope
        addVariableSymbols(items, uri, pos, filter);
        
        // Callable functions
        addCallableSymbols(items, uri, filter);
        
        // CImport symbols
        addCImportCompletions(items, uri, filter);
    }
    
    return items;
}

// ============================================================================
// CImport Completions
// ============================================================================

std::vector<std::string> CompletionProvider::getCImportsFromFile(const std::string& uri) {
    std::vector<std::string> cimports;
    
    // Get direct cimports from file
    auto directCImports = analyzer.getCImports(uri);
    for (const auto& h : directCImports) {
        cimports.push_back(h);
    }
    
    // Also check imported modules for their cimports
    auto modules = analyzer.getImportedModules(uri);
    auto& loader = StdLibLoader::instance();
    
    for (const auto& mod : modules) {
        auto metadata = loader.getModuleMetadata(mod);
        for (const auto& cimport : metadata.cimports) {
            cimports.push_back(cimport);
        }
    }
    
    return cimports;
}

void CompletionProvider::addCImportCompletions(JsonValue& items, const std::string& uri, const std::string& filter) {
    auto cimports = getCImportsFromFile(uri);
    auto& parser = CHeaderParser::instance();
    
    for (const auto& headerPath : cimports) {
        addCHeaderCompletions(items, headerPath, filter);
    }
}

void CompletionProvider::addCHeaderCompletions(JsonValue& items, const std::string& headerPath, const std::string& filter) {
    addCFunctionCompletions(items, headerPath, filter);
    addCStructCompletions(items, headerPath, filter);
    addCEnumCompletions(items, headerPath, filter);
    addCMacroCompletions(items, headerPath, filter);
}

void CompletionProvider::addCFunctionCompletions(JsonValue& items, const std::string& headerPath, const std::string& filter) {
    auto& parser = CHeaderParser::instance();
    auto funcs = parser.getFunctions(headerPath);
    
    for (const auto& func : funcs) {
        if (!matchesFilter(func.name, filter)) continue;
        
        // Build signature
        std::string sig = func.name + "(";
        std::string snippet = func.name + "(";
        int idx = 1;
        
        for (size_t i = 0; i < func.paramTypes.size(); ++i) {
            if (i > 0) { sig += ", "; snippet += ", "; }
            
            std::string paramName = i < func.paramNames.size() && !func.paramNames[i].empty() 
                ? func.paramNames[i] 
                : "arg" + std::to_string(i);
            
            sig += func.paramTypes[i];
            if (!paramName.empty()) {
                sig += " " + paramName;
            }
            snippet += "${" + std::to_string(idx++) + ":" + paramName + "}";
        }
        sig += ") -> " + func.returnType;
        snippet += ")$0";
        
        items.push(makeCompletionItem(
            func.name, CompletionItemKind::Function,
            sig, "C function from " + func.headerFile,
            snippet, "4" + func.name
        ));
    }
}

void CompletionProvider::addCStructCompletions(JsonValue& items, const std::string& headerPath, const std::string& filter) {
    auto& parser = CHeaderParser::instance();
    auto structs = parser.getStructs(headerPath);
    
    for (const auto& st : structs) {
        if (!matchesFilter(st.name, filter)) continue;
        
        std::string doc = "C struct from " + st.headerFile + "\n\nFields:\n";
        for (const auto& field : st.fields) {
            doc += "  " + field.second + " " + field.first + "\n";
        }
        
        items.push(makeCompletionItem(
            st.name, CompletionItemKind::Struct,
            "struct " + st.name, doc,
            "", "4" + st.name
        ));
    }
}

void CompletionProvider::addCEnumCompletions(JsonValue& items, const std::string& headerPath, const std::string& filter) {
    auto& parser = CHeaderParser::instance();
    auto enums = parser.getEnums(headerPath);
    
    for (const auto& en : enums) {
        // Add enum type
        if (matchesFilter(en.name, filter)) {
            std::string doc = "C enum from " + en.headerFile + "\n\nValues:\n";
            for (const auto& val : en.enumValues) {
                doc += "  " + val + "\n";
            }
            
            items.push(makeCompletionItem(
                en.name, CompletionItemKind::Enum,
                "enum " + en.name, doc,
                "", "4" + en.name
            ));
        }
        
        // Add enum values
        for (const auto& val : en.enumValues) {
            if (matchesFilter(val, filter)) {
                items.push(makeCompletionItem(
                    val, CompletionItemKind::EnumMember,
                    en.name + "::" + val, "C enum value from " + en.headerFile,
                    "", "5" + val
                ));
            }
        }
    }
}

void CompletionProvider::addCMacroCompletions(JsonValue& items, const std::string& headerPath, const std::string& filter) {
    auto& parser = CHeaderParser::instance();
    auto macros = parser.getMacros(headerPath);
    
    for (const auto& macro : macros) {
        if (!matchesFilter(macro.name, filter)) continue;
        
        std::string detail = "#define " + macro.name;
        std::string snippet = macro.name;
        
        if (!macro.paramNames.empty()) {
            detail += "(";
            snippet += "(";
            int idx = 1;
            for (size_t i = 0; i < macro.paramNames.size(); ++i) {
                if (i > 0) { detail += ", "; snippet += ", "; }
                detail += macro.paramNames[i];
                snippet += "${" + std::to_string(idx++) + ":" + macro.paramNames[i] + "}";
            }
            detail += ")";
            snippet += ")$0";
        }
        
        items.push(makeCompletionItem(
            macro.name, CompletionItemKind::Constant,
            detail, "C macro from " + macro.headerFile,
            macro.paramNames.empty() ? "" : snippet, "6" + macro.name
        ));
    }
}
