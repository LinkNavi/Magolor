#pragma once
#include "jsonrpc.hpp"
#include "document.hpp"
#include "symbol_table.hpp"
#include "diagnostics.hpp"
#include <functional>
#include <unordered_map>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

class MagolorLanguageServer {
public:
    void run() {
        running = true;
        while (running) {
            auto msg = transport.receive();
            if (!msg) break;
            handleMessage(*msg);
        }
    }

private:
    Transport transport;
    DocumentManager documents;
    SymbolTable symbols;
    bool running = false;
    bool initialized = false;
    std::string rootUri;
    
    // Completion item kinds (LSP standard)
    enum CompletionItemKind {
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
    
    std::vector<CompletionSnippet> getBuiltinSnippets() {
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
    
    void handleMessage(const Message& msg) {
        if (msg.method == "initialize") handleInitialize(msg);
        else if (msg.method == "initialized") handleInitialized(msg);
        else if (msg.method == "shutdown") handleShutdown(msg);
        else if (msg.method == "exit") handleExit(msg);
        else if (msg.method == "textDocument/didOpen") handleDidOpen(msg);
        else if (msg.method == "textDocument/didChange") handleDidChange(msg);
        else if (msg.method == "textDocument/didClose") handleDidClose(msg);
        else if (msg.method == "textDocument/didSave") handleDidSave(msg);
        else if (msg.method == "textDocument/completion") handleCompletion(msg);
        else if (msg.method == "textDocument/hover") handleHover(msg);
        else if (msg.method == "textDocument/definition") handleDefinition(msg);
        else if (msg.method == "textDocument/references") handleReferences(msg);
        else if (msg.method == "textDocument/documentSymbol") handleDocumentSymbol(msg);
        else if (msg.method == "textDocument/formatting") handleFormatting(msg);
        else if (msg.method == "textDocument/rename") handleRename(msg);
        else if (msg.method == "completionItem/resolve") handleCompletionResolve(msg);
        else if (msg.isRequest()) {
            transport.respondError(msg.id.value(), -32601, "Method not found");
        }
    }
    
    void handleInitialize(const Message& msg) {
        if (msg.params.has("rootUri")) {
            rootUri = msg.params["rootUri"].asString();
        }
        
        JsonValue caps = JsonValue::object();
        
        // Text document sync
        caps["textDocumentSync"] = JsonValue::object();
        caps["textDocumentSync"]["openClose"] = true;
        caps["textDocumentSync"]["change"] = 1; // Full sync
        caps["textDocumentSync"]["save"] = JsonValue::object();
        caps["textDocumentSync"]["save"]["includeText"] = true;
        
        // Completion with snippet support
        caps["completionProvider"] = JsonValue::object();
        caps["completionProvider"]["triggerCharacters"] = JsonValue::array();
        caps["completionProvider"]["triggerCharacters"].push(".");
        caps["completionProvider"]["triggerCharacters"].push(":");
        caps["completionProvider"]["triggerCharacters"].push("(");
        caps["completionProvider"]["resolveProvider"] = true;
        
        // Hover
        caps["hoverProvider"] = true;
        
        // Go to definition
        caps["definitionProvider"] = true;
        
        // Find references
        caps["referencesProvider"] = true;
        
        // Document symbols
        caps["documentSymbolProvider"] = true;
        
        // Formatting
        caps["documentFormattingProvider"] = true;
        
        // Rename
        caps["renameProvider"] = true;
        
        // Signature help
        caps["signatureHelpProvider"] = JsonValue::object();
        caps["signatureHelpProvider"]["triggerCharacters"] = JsonValue::array();
        caps["signatureHelpProvider"]["triggerCharacters"].push("(");
        caps["signatureHelpProvider"]["triggerCharacters"].push(",");
        
        JsonValue result = JsonValue::object();
        result["capabilities"] = caps;
        result["serverInfo"] = JsonValue::object();
        result["serverInfo"]["name"] = "magolor-lsp";
        result["serverInfo"]["version"] = "0.1.0";
        
        transport.respond(msg.id.value(), result);
    }
    
    void handleInitialized(const Message&) {
        initialized = true;
        indexWorkspace();
    }
    
    void handleShutdown(const Message& msg) {
        transport.respond(msg.id.value(), JsonValue());
    }
    
    void handleExit(const Message&) {
        running = false;
    }
    
    void handleDidOpen(const Message& msg) {
        auto& td = msg.params["textDocument"];
        std::string uri = td["uri"].asString();
        std::string languageId = td["languageId"].asString();
        int version = td["version"].asInt();
        std::string text = td["text"].asString();
        
        documents.open(uri, languageId, version, text);
        analyzeDocument(uri);
    }
    
    void handleDidChange(const Message& msg) {
        auto& td = msg.params["textDocument"];
        std::string uri = td["uri"].asString();
        int version = td["version"].asInt();
        
        auto& changes = msg.params["contentChanges"].asArray();
        if (!changes.empty()) {
            std::string text = changes[0]["text"].asString();
            documents.change(uri, version, text);
            analyzeDocument(uri);
        }
    }
    
    void handleDidClose(const Message& msg) {
        std::string uri = msg.params["textDocument"]["uri"].asString();
        documents.close(uri);
    }
    
    void handleDidSave(const Message& msg) {
        std::string uri = msg.params["textDocument"]["uri"].asString();
        analyzeDocument(uri);
    }
    
    void handleCompletion(const Message& msg) {
        auto& td = msg.params["textDocument"];
        std::string uri = td["uri"].asString();
        Position pos;
        pos.line = msg.params["position"]["line"].asInt();
        pos.character = msg.params["position"]["character"].asInt();
        
        JsonValue items = JsonValue::array();
        
        auto* doc = documents.get(uri);
        if (doc) {
            std::string word = doc->getWordAt(pos);
            
            // Add snippets first (they're often what user wants)
            for (const auto& snippet : getBuiltinSnippets()) {
                if (word.empty() || snippet.label.find(word) == 0) {
                    JsonValue item = JsonValue::object();
                    item["label"] = snippet.label;
                    item["kind"] = Snippet;
                    item["insertText"] = snippet.insertText;
                    item["insertTextFormat"] = 2; // Snippet format
                    item["detail"] = snippet.detail;
                    item["documentation"] = snippet.documentation;
                    item["sortText"] = "0_" + snippet.label; // Sort snippets first
                    items.push(item);
                }
            }
            
            // Add symbols from workspace
            auto matches = symbols.findByPrefix(word);
            for (auto& sym : matches) {
                JsonValue item = JsonValue::object();
                item["label"] = sym->name;
                item["kind"] = static_cast<int>(sym->kind);
                item["detail"] = sym->detail.empty() ? sym->type : sym->detail;
                if (!sym->documentation.empty()) {
                    item["documentation"] = sym->documentation;
                }
                item["sortText"] = "1_" + sym->name; // Sort after snippets
                items.push(item);
            }
        }
        
        // Add keywords
        static const char* keywords[] = {
            "fn", "let", "mut", "return", "if", "else", "while", "for",
            "match", "class", "new", "this", "true", "false", "None", "Some",
            "using", "pub", "priv", "static", "cimport", "int", "float",
            "string", "bool", "void"
        };
        
        for (auto kw : keywords) {
            JsonValue item = JsonValue::object();
            item["label"] = kw;
            item["kind"] = Keyword;
            item["sortText"] = "2_" + std::string(kw); // Sort after symbols
            items.push(item);
        }
        
        // Add standard library completions
        if (doc) {
            std::string line = doc->getLine(pos.line);
            
            // After "Std."
            if (line.find("Std.") != std::string::npos) {
                std::vector<std::string> stdModules = {
                    "IO", "Parse", "Option", "Math", "String", 
                    "Array", "Map", "Set", "File", "Time", "Random", "System"
                };
                for (const auto& mod : stdModules) {
                    JsonValue item = JsonValue::object();
                    item["label"] = mod;
                    item["kind"] = Module;
                    item["detail"] = "Std." + mod;
                    item["sortText"] = "1_" + mod;
                    items.push(item);
                }
            }
            
            // After "Std.IO."
            if (line.find("Std.IO.") != std::string::npos) {
                std::vector<std::pair<std::string, std::string>> ioFuncs = {
                    {"print", "print(s: string)"},
                    {"println", "println(s: string)"},
                    {"eprint", "eprint(s: string)"},
                    {"eprintln", "eprintln(s: string)"},
                    {"readLine", "readLine() -> string"},
                    {"read", "read() -> string"},
                };
                for (const auto& [name, sig] : ioFuncs) {
                    JsonValue item = JsonValue::object();
                    item["label"] = name;
                    item["kind"] = Function;
                    item["detail"] = sig;
                    item["sortText"] = "1_" + name;
                    items.push(item);
                }
            }
            
            // After "Std.Math."
            if (line.find("Std.Math.") != std::string::npos) {
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
                    item["kind"] = (name == "PI" || name == "E") ? Constant : Function;
                    item["detail"] = sig;
                    item["sortText"] = "1_" + name;
                    items.push(item);
                }
            }
        }
        
        transport.respond(msg.id.value(), items);
    }
    
    void handleCompletionResolve(const Message& msg) {
        // Return the item with additional documentation if needed
        transport.respond(msg.id.value(), msg.params);
    }
    
    void handleHover(const Message& msg) {
        auto& td = msg.params["textDocument"];
        std::string uri = td["uri"].asString();
        Position pos;
        pos.line = msg.params["position"]["line"].asInt();
        pos.character = msg.params["position"]["character"].asInt();
        
        auto* doc = documents.get(uri);
        if (!doc) {
            transport.respond(msg.id.value(), JsonValue());
            return;
        }
        
        std::string word = doc->getWordAt(pos);
        auto sym = symbols.lookup(word);
        
        if (sym) {
            JsonValue result = JsonValue::object();
            JsonValue contents = JsonValue::object();
            contents["kind"] = "markdown";
            
            std::string md = "```magolor\n";
            if (sym->kind == SymbolKind::Function) {
                md += "fn " + sym->name + sym->detail + "\n";
            } else if (sym->kind == SymbolKind::Variable) {
                md += "let " + sym->name + ": " + sym->type + "\n";
            } else if (sym->kind == SymbolKind::Class) {
                md += "class " + sym->name + "\n";
            } else {
                md += sym->name + ": " + sym->type + "\n";
            }
            md += "```";
            
            if (!sym->documentation.empty()) {
                md += "\n\n" + sym->documentation;
            }
            
            contents["value"] = md;
            result["contents"] = contents;
            transport.respond(msg.id.value(), result);
        } else {
            transport.respond(msg.id.value(), JsonValue());
        }
    }
    
    void handleDefinition(const Message& msg) {
        auto& td = msg.params["textDocument"];
        std::string uri = td["uri"].asString();
        Position pos;
        pos.line = msg.params["position"]["line"].asInt();
        pos.character = msg.params["position"]["character"].asInt();
        
        auto* doc = documents.get(uri);
        if (!doc) {
            transport.respond(msg.id.value(), JsonValue());
            return;
        }
        
        std::string word = doc->getWordAt(pos);
        auto sym = symbols.lookup(word);
        
        if (sym) {
            JsonValue loc = JsonValue::object();
            loc["uri"] = sym->definition.uri;
            loc["range"] = rangeToJson(sym->definition.range);
            transport.respond(msg.id.value(), loc);
        } else {
            transport.respond(msg.id.value(), JsonValue());
        }
    }
    
    void handleReferences(const Message& msg) {
        auto& td = msg.params["textDocument"];
        std::string uri = td["uri"].asString();
        Position pos;
        pos.line = msg.params["position"]["line"].asInt();
        pos.character = msg.params["position"]["character"].asInt();
        
        auto* doc = documents.get(uri);
        if (!doc) {
            transport.respond(msg.id.value(), JsonValue::array());
            return;
        }
        
        std::string word = doc->getWordAt(pos);
        auto sym = symbols.lookup(word);
        
        JsonValue locs = JsonValue::array();
        if (sym) {
            // Include definition
            JsonValue defLoc = JsonValue::object();
            defLoc["uri"] = sym->definition.uri;
            defLoc["range"] = rangeToJson(sym->definition.range);
            locs.push(defLoc);
            
            // Include references
            for (auto& ref : sym->references) {
                JsonValue refLoc = JsonValue::object();
                refLoc["uri"] = ref.uri;
                refLoc["range"] = rangeToJson(ref.range);
                locs.push(refLoc);
            }
        }
        
        transport.respond(msg.id.value(), locs);
    }
    
    void handleDocumentSymbol(const Message& msg) {
        std::string uri = msg.params["textDocument"]["uri"].asString();
        auto syms = symbols.getSymbolsInFile(uri);
        
        JsonValue result = JsonValue::array();
        for (auto& sym : syms) {
            JsonValue s = JsonValue::object();
            s["name"] = sym->name;
            s["kind"] = static_cast<int>(sym->kind);
            s["range"] = rangeToJson(sym->definition.range);
            s["selectionRange"] = rangeToJson(sym->definition.range);
            if (!sym->containerName.empty()) {
                s["containerName"] = sym->containerName;
            }
            result.push(s);
        }
        
        transport.respond(msg.id.value(), result);
    }
    
    void handleFormatting(const Message& msg) {
        std::string uri = msg.params["textDocument"]["uri"].asString();
        auto* doc = documents.get(uri);
        
        if (!doc) {
            transport.respond(msg.id.value(), JsonValue::array());
            return;
        }
        
        std::string formatted = formatCode(doc->content);
        
        JsonValue edits = JsonValue::array();
        if (formatted != doc->content) {
            JsonValue edit = JsonValue::object();
            Range fullRange;
            fullRange.start = {0, 0};
            fullRange.end = doc->offsetToPosition(doc->content.size());
            edit["range"] = rangeToJson(fullRange);
            edit["newText"] = formatted;
            edits.push(edit);
        }
        
        transport.respond(msg.id.value(), edits);
    }
    
    void handleRename(const Message& msg) {
        auto& td = msg.params["textDocument"];
        std::string uri = td["uri"].asString();
        Position pos;
        pos.line = msg.params["position"]["line"].asInt();
        pos.character = msg.params["position"]["character"].asInt();
        std::string newName = msg.params["newName"].asString();
        
        auto* doc = documents.get(uri);
        if (!doc) {
            transport.respond(msg.id.value(), JsonValue());
            return;
        }
        
        std::string word = doc->getWordAt(pos);
        auto sym = symbols.lookup(word);
        
        if (!sym) {
            transport.respond(msg.id.value(), JsonValue());
            return;
        }
        
        JsonValue wsEdit = JsonValue::object();
        JsonValue changes = JsonValue::object();
        
        // Definition edit
        if (!changes.has(sym->definition.uri)) {
            changes[sym->definition.uri] = JsonValue::array();
        }
        JsonValue defEdit = JsonValue::object();
        defEdit["range"] = rangeToJson(sym->definition.range);
        defEdit["newText"] = newName;
        changes[sym->definition.uri].push(defEdit);
        
        // Reference edits
        for (auto& ref : sym->references) {
            if (!changes.has(ref.uri)) {
                changes[ref.uri] = JsonValue::array();
            }
            JsonValue refEdit = JsonValue::object();
            refEdit["range"] = rangeToJson(ref.range);
            refEdit["newText"] = newName;
            changes[ref.uri].push(refEdit);
        }
        
        wsEdit["changes"] = changes;
        transport.respond(msg.id.value(), wsEdit);
    }
    
    JsonValue rangeToJson(const Range& r) {
        JsonValue json = JsonValue::object();
        json["start"] = JsonValue::object();
        json["start"]["line"] = r.start.line;
        json["start"]["character"] = r.start.character;
        json["end"] = JsonValue::object();
        json["end"]["line"] = r.end.line;
        json["end"]["character"] = r.end.character;
        return json;
    }
    
    void indexWorkspace() {
        if (rootUri.empty()) return;
        
        std::string path = uriToPath(rootUri);
        if (!fs::exists(path)) return;
        
        for (auto& entry : fs::recursive_directory_iterator(path)) {
            if (entry.is_regular_file() && entry.path().extension() == ".mg") {
                std::string uri = pathToUri(entry.path().string());
                std::ifstream file(entry.path());
                std::string content((std::istreambuf_iterator<char>(file)),
                                   std::istreambuf_iterator<char>());
                documents.open(uri, "magolor", 0, content);
                analyzeDocument(uri);
            }
        }
    }
    
    void analyzeDocument(const std::string& uri) {
        auto* doc = documents.get(uri);
        if (!doc) return;
        
        std::vector<LspDiagnostic> diagnostics;
        extractSymbols(uri, doc->content, diagnostics);
        publishDiagnostics(uri, diagnostics);
    }
    
    void extractSymbols(const std::string& uri, const std::string& content,
                       std::vector<LspDiagnostic>&) {
        std::istringstream stream(content);
        std::string line;
        int lineNum = 0;
        std::string currentClass;
        
        while (std::getline(stream, line)) {
            // Function detection
            size_t fnPos = line.find("fn ");
            if (fnPos != std::string::npos) {
                size_t nameStart = fnPos + 3;
                size_t nameEnd = line.find('(', nameStart);
                if (nameEnd != std::string::npos) {
                    std::string name = line.substr(nameStart, nameEnd - nameStart);
                    while (!name.empty() && name.back() == ' ') name.pop_back();
                    while (!name.empty() && name.front() == ' ') name = name.substr(1);
                    
                    if (!name.empty() && name != "main") {
                        auto sym = std::make_shared<Symbol>();
                        sym->name = name;
                        sym->kind = currentClass.empty() ? SymbolKind::Function : SymbolKind::Method;
                        sym->containerName = currentClass;
                        sym->definition.uri = uri;
                        sym->definition.range.start = {lineNum, (int)nameStart};
                        sym->definition.range.end = {lineNum, (int)nameEnd};
                        
                        size_t sigEnd = line.find('{');
                        if (sigEnd == std::string::npos) sigEnd = line.size();
                        sym->detail = line.substr(nameEnd, sigEnd - nameEnd);
                        
                        symbols.addSymbol(sym);
                    }
                }
            }
            
            // Class detection
            size_t classPos = line.find("class ");
            if (classPos != std::string::npos) {
                size_t nameStart = classPos + 6;
                size_t nameEnd = line.find_first_of(" {", nameStart);
                if (nameEnd != std::string::npos) {
                    std::string name = line.substr(nameStart, nameEnd - nameStart);
                    while (!name.empty() && name.back() == ' ') name.pop_back();
                    
                    auto sym = std::make_shared<Symbol>();
                    sym->name = name;
                    sym->kind = SymbolKind::Class;
                    sym->definition.uri = uri;
                    sym->definition.range.start = {lineNum, (int)nameStart};
                    sym->definition.range.end = {lineNum, (int)nameEnd};
                    
                    symbols.addSymbol(sym);
                    currentClass = name;
                }
            }
            
            // Variable detection
            size_t letPos = line.find("let ");
            if (letPos != std::string::npos) {
                size_t nameStart = letPos + 4;
                if (line.substr(nameStart, 4) == "mut ") nameStart += 4;
                
                size_t nameEnd = line.find_first_of(":=", nameStart);
                if (nameEnd != std::string::npos) {
                    std::string name = line.substr(nameStart, nameEnd - nameStart);
                    while (!name.empty() && name.back() == ' ') name.pop_back();
                    
                    auto sym = std::make_shared<Symbol>();
                    sym->name = name;
                    sym->kind = SymbolKind::Variable;
                    sym->containerName = currentClass;
                    sym->definition.uri = uri;
                    sym->definition.range.start = {lineNum, (int)nameStart};
                    sym->definition.range.end = {lineNum, (int)nameEnd};
                    
                    size_t colonPos = line.find(':', nameEnd);
                    if (colonPos != std::string::npos) {
                        size_t typeEnd = line.find('=', colonPos);
                        if (typeEnd == std::string::npos) typeEnd = line.size();
                        sym->type = line.substr(colonPos + 1, typeEnd - colonPos - 1);
                        while (!sym->type.empty() && sym->type.front() == ' ') 
                            sym->type = sym->type.substr(1);
                        while (!sym->type.empty() && sym->type.back() == ' ') 
                            sym->type.pop_back();
                    }
                    
                    symbols.addSymbol(sym);
                }
            }
            
            if (line.find('}') != std::string::npos && !currentClass.empty()) {
                currentClass = "";
            }
            
            lineNum++;
        }
    }
    
    void publishDiagnostics(const std::string& uri, 
                           const std::vector<LspDiagnostic>& diagnostics) {
        JsonValue params = JsonValue::object();
        params["uri"] = uri;
        params["diagnostics"] = JsonValue::array();
        
        for (auto& d : diagnostics) {
            JsonValue diag = JsonValue::object();
            diag["range"] = rangeToJson(d.range);
            diag["severity"] = static_cast<int>(d.severity);
            diag["source"] = d.source;
            diag["message"] = d.message;
            if (!d.code.empty()) {
                diag["code"] = d.code;
            }
            params["diagnostics"].push(diag);
        }
        
        transport.notify("textDocument/publishDiagnostics", params);
    }
    
    std::string formatCode(const std::string& code) {
        std::istringstream stream(code);
        std::ostringstream result;
        std::string line;
        int indent = 0;
        
        while (std::getline(stream, line)) {
            size_t start = line.find_first_not_of(" \t");
            if (start != std::string::npos) {
                line = line.substr(start);
            } else {
                line = "";
            }
            
            if (!line.empty() && line[0] == '}') {
                indent = std::max(0, indent - 1);
            }
            
            if (!line.empty()) {
                for (int i = 0; i < indent; i++) {
                    result << "    ";
                }
                result << line << "\n";
            } else {
                result << "\n";
            }
            
            for (char c : line) {
                if (c == '{') indent++;
                else if (c == '}') indent = std::max(0, indent - 1);
            }
        }
        
        return result.str();
    }
    
    std::string uriToPath(const std::string& uri) {
        if (uri.find("file://") == 0) {
            return uri.substr(7);
        }
        return uri;
    }
    
    std::string pathToUri(const std::string& path) {
        return "file://" + path;
    }
};
