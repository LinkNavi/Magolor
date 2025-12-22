#include "lsp_completion.hpp"
#include <algorithm>

std::vector<CompletionSnippet> CompletionProvider::getBuiltinSnippets() {
    return {
        {
            "fn",
            "fn ${1:name}(${2:params}) -> ${3:void} {\n\t${0}\n}",
            "Function declaration",
            "Create a new function with parameters and return type"
        },
        {
            "fnr",
            "fn ${1:name}(${2:params}) -> ${3:int} {\n\treturn ${0:0};\n}",
            "Function with return",
            "Create a function that returns a value"
        },
        {
            "main",
            "fn main() {\n\t${0}\n}",
            "Main function",
            "Entry point of the program"
        },
        {
            "class",
            "class ${1:Name} {\n\tpub ${2:field}: ${3:int};\n\t\n\tpub fn ${4:method}() {\n\t\t${0}\n\t}\n}",
            "Class definition",
            "Create a class with fields and methods"
        },
        {
            "if",
            "if (${1:condition}) {\n\t${0}\n}",
            "If statement",
            "Conditional execution"
        },
        {
            "ife",
            "if (${1:condition}) {\n\t${2}\n} else {\n\t${0}\n}",
            "If-else statement",
            "Conditional with alternative"
        },
        {
            "while",
            "while (${1:condition}) {\n\t${0}\n}",
            "While loop",
            "Loop while condition is true"
        },
        {
            "for",
            "for (${1:item} in ${2:array}) {\n\t${0}\n}",
            "For loop",
            "Iterate over collection"
        },
        {
            "match",
            "match ${1:value} {\n\tSome(${2:v}) => {\n\t\t${3}\n\t},\n\tNone => {\n\t\t${0}\n\t}\n}",
            "Match expression",
            "Pattern matching for Option types"
        },
        {
            "let",
            "let ${1:mut }${2:name} = ${0:value};",
            "Variable declaration",
            "Declare a variable (optionally mutable)"
        },
        {
            "lett",
            "let ${1:mut }${2:name}: ${3:type} = ${0:value};",
            "Variable with type",
            "Declare a typed variable"
        },
        {
            "print",
            "Std.print(${1:\"message\"});",
            "Print statement",
            "Print text to stdout"
        },
        {
            "printf",
            "Std.print($\"${1:text}{${2:var}}${0}\");",
            "Print with interpolation",
            "Print with string interpolation"
        },
        {
            "using",
            "using ${1:Std.IO};",
            "Import statement",
            "Import a module"
        },
        {
            "cimport",
            "cimport <${1:header.h}>${2: as ${3:Name}};",
            "C/C++ import",
            "Import C/C++ header"
        },
        {
            "cpp",
            "@cpp {\n\t${0}\n}",
            "C++ block",
            "Inline C++ code"
        },
        {
            "pubfn",
            "pub fn ${1:name}(${2:params}) -> ${3:void} {\n\t${0}\n}",
            "Public function",
            "Public function declaration"
        },
        {
            "staticfn",
            "pub static fn ${1:name}(${2:params}) -> ${3:void} {\n\t${0}\n}",
            "Static function",
            "Static function declaration"
        },
        {
            "lambda",
            "fn(${1:x}: ${2:int}) -> ${3:int} {\n\treturn ${0:x};\n}",
            "Lambda function",
            "Anonymous function/closure"
        },
        {
            "ret",
            "return ${0:value};",
            "Return statement",
            "Return from function"
        },
        {
            "new",
            "let ${1:var} = new ${2:Class}();",
            "New instance",
            "Create class instance"
        },
    };
}

std::vector<std::string> CompletionProvider::getKeywords() {
    return {
        "fn", "let", "mut", "return", "if", "else", "while", "for",
        "match", "class", "new", "this", "true", "false", "None", "Some",
        "using", "pub", "priv", "static", "cimport", "int", "float",
        "string", "bool", "void"
    };
}

bool CompletionProvider::matchesFilter(const std::string& name, const std::string& filter) {
    if (filter.empty()) return true;
    
    // Case-insensitive prefix match
    if (name.size() < filter.size()) return false;
    
    for (size_t i = 0; i < filter.size(); i++) {
        if (std::tolower(name[i]) != std::tolower(filter[i])) {
            return false;
        }
    }
    return true;
}

void CompletionProvider::addCallableSymbols(JsonValue& items, const std::string& uri, const std::string& filter) {
    auto callables = analyzer.getCallableSymbols(uri);
    
    for (const auto& sym : callables) {
        if (!matchesFilter(sym->name, filter)) continue;
        
        JsonValue item = JsonValue::object();
        item["label"] = sym->name;
        item["kind"] = (int)(sym->kind == SymbolKind::Method ? 
                            CompletionItemKind::Method : 
                            CompletionItemKind::Function);
        
        // Show signature
        if (!sym->detail.empty()) {
            item["detail"] = sym->name + sym->detail;
        } else {
            item["detail"] = sym->name + "()";
        }
        
        if (!sym->documentation.empty()) {
            item["documentation"] = sym->documentation;
        }
        
        item["sortText"] = "1_" + sym->name;
        items.push(item);
    }
}

void CompletionProvider::addVariableSymbols(JsonValue& items, const std::string& uri, Position pos, const std::string& filter) {
    auto variables = analyzer.getVariablesInScope(uri, pos);
    
    for (const auto& sym : variables) {
        if (!matchesFilter(sym->name, filter)) continue;
        
        JsonValue item = JsonValue::object();
        item["label"] = sym->name;
        item["kind"] = (int)CompletionItemKind::Variable;
        
        if (!sym->type.empty()) {
            item["detail"] = sym->type;
        }
        
        item["sortText"] = "1_" + sym->name;
        items.push(item);
    }
}

void CompletionProvider::addStdLibCompletions(JsonValue& items, const std::string& context) {
    if (context.find("Std.") != std::string::npos) {
        std::vector<std::string> stdModules = {
            "IO", "Parse", "Option", "Math", "String", 
            "Array", "Map", "Set", "File", "Time", "Random", "System"
        };
        for (const auto& mod : stdModules) {
            JsonValue item = JsonValue::object();
            item["label"] = mod;
            item["kind"] = (int)CompletionItemKind::Module;
            item["detail"] = "Std." + mod;
            item["sortText"] = "1_" + mod;
            items.push(item);
        }
    }
    
    if (context.find("Std.IO.") != std::string::npos) {
        std::vector<std::pair<std::string, std::string>> ioFuncs = {
            {"print", "print(s: string) -> void"},
            {"println", "println(s: string) -> void"},
            {"eprint", "eprint(s: string) -> void"},
            {"eprintln", "eprintln(s: string) -> void"},
            {"readLine", "readLine() -> string"},
            {"read", "read() -> string"},
        };
        for (const auto& [name, sig] : ioFuncs) {
            JsonValue item = JsonValue::object();
            item["label"] = name;
            item["kind"] = (int)CompletionItemKind::Function;
            item["detail"] = sig;
            item["sortText"] = "1_" + name;
            items.push(item);
        }
    }
    
    if (context.find("Std.Math.") != std::string::npos) {
        std::vector<std::pair<std::string, std::string>> mathFuncs = {
            {"sqrt", "sqrt(x: float) -> float"},
            {"sin", "sin(x: float) -> float"},
            {"cos", "cos(x: float) -> float"},
            {"tan", "tan(x: float) -> float"},
            {"abs", "abs(x: float) -> float"},
            {"pow", "pow(base: float, exp: float) -> float"},
            {"PI", "PI: float = 3.14159..."},
            {"E", "E: float = 2.71828..."},
        };
        for (const auto& [name, sig] : mathFuncs) {
            JsonValue item = JsonValue::object();
            item["label"] = name;
            item["kind"] = (name == "PI" || name == "E") ? 
                          (int)CompletionItemKind::Constant : 
                          (int)CompletionItemKind::Function;
            item["detail"] = sig;
            item["sortText"] = "1_" + name;
            items.push(item);
        }
    }
    
    if (context.find("Std.String.") != std::string::npos) {
        std::vector<std::pair<std::string, std::string>> stringFuncs = {
            {"length", "length(s: string) -> int"},
            {"isEmpty", "isEmpty(s: string) -> bool"},
            {"trim", "trim(s: string) -> string"},
            {"toLower", "toLower(s: string) -> string"},
            {"toUpper", "toUpper(s: string) -> string"},
            {"contains", "contains(s: string, substr: string) -> bool"},
            {"split", "split(s: string, delim: char) -> Array<string>"},
        };
        for (const auto& [name, sig] : stringFuncs) {
            JsonValue item = JsonValue::object();
            item["label"] = name;
            item["kind"] = (int)CompletionItemKind::Function;
            item["detail"] = sig;
            item["sortText"] = "1_" + name;
            items.push(item);
        }
    }
    
    if (context.find("Std.Array.") != std::string::npos) {
        std::vector<std::pair<std::string, std::string>> arrayFuncs = {
            {"length", "length(arr: Array<T>) -> int"},
            {"isEmpty", "isEmpty(arr: Array<T>) -> bool"},
            {"push", "push(arr: Array<T>, item: T) -> void"},
            {"pop", "pop(arr: Array<T>) -> Option<T>"},
            {"contains", "contains(arr: Array<T>, item: T) -> bool"},
            {"reverse", "reverse(arr: Array<T>) -> void"},
            {"sort", "sort(arr: Array<T>) -> void"},
        };
        for (const auto& [name, sig] : arrayFuncs) {
            JsonValue item = JsonValue::object();
            item["label"] = name;
            item["kind"] = (int)CompletionItemKind::Function;
            item["detail"] = sig;
            item["sortText"] = "1_" + name;
            items.push(item);
        }
    }
}

JsonValue CompletionProvider::provideCompletions(const std::string& uri, Position pos, const std::string& lineText) {
    JsonValue items = JsonValue::array();
    
    // Extract the word being typed
    std::string word;
    int charPos = pos.character - 1;
    while (charPos >= 0 && (std::isalnum(lineText[charPos]) || lineText[charPos] == '_')) {
        word = lineText[charPos] + word;
        charPos--;
    }
    
    // Check context for standard library completions
    std::string context = lineText.substr(0, pos.character);
    addStdLibCompletions(items, context);
    
    // Add snippets (lower priority than actual symbols)
    for (const auto& snippet : getBuiltinSnippets()) {
        if (matchesFilter(snippet.label, word)) {
            JsonValue item = JsonValue::object();
            item["label"] = snippet.label;
            item["kind"] = (int)CompletionItemKind::Snippet;
            item["insertText"] = snippet.insertText;
            item["insertTextFormat"] = 2; // Snippet format
            item["detail"] = snippet.detail;
            item["documentation"] = snippet.documentation;
            item["sortText"] = "2_" + snippet.label;
            items.push(item);
        }
    }
    
    // Add callable symbols (functions and methods)
    addCallableSymbols(items, uri, word);
    
    // Add variables in scope
    addVariableSymbols(items, uri, pos, word);
    
    // Add keywords (lowest priority)
    for (const auto& kw : getKeywords()) {
        if (matchesFilter(kw, word)) {
            JsonValue item = JsonValue::object();
            item["label"] = kw;
            item["kind"] = (int)CompletionItemKind::Keyword;
            item["sortText"] = "3_" + kw;
            items.push(item);
        }
    }
    
    return items;
}
