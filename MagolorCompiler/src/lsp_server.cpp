#include "lsp_server.hpp"

void MagolorLanguageServer::handleMessage(const Message& msg) {
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
    else if (msg.isRequest()) {
        transport.respondError(msg.id.value(), -32601, "Method not found");
    }
}

void MagolorLanguageServer::handleInitialize(const Message& msg) {
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
    caps["completionProvider"]["resolveProvider"] = true;
    
    // Other capabilities
    caps["hoverProvider"] = true;
    caps["definitionProvider"] = true;
    caps["referencesProvider"] = true;
    caps["documentSymbolProvider"] = true;
    
    JsonValue result = JsonValue::object();
    result["capabilities"] = caps;
    result["serverInfo"] = JsonValue::object();
    result["serverInfo"]["name"] = "magolor-lsp";
    result["serverInfo"]["version"] = "0.2.0";
    
    transport.respond(msg.id.value(), result);
}

void MagolorLanguageServer::handleInitialized(const Message&) {
    initialized = true;
}

void MagolorLanguageServer::handleShutdown(const Message& msg) {
    transport.respond(msg.id.value(), JsonValue());
}

void MagolorLanguageServer::handleExit(const Message&) {
    running = false;
}

void MagolorLanguageServer::handleDidOpen(const Message& msg) {
    auto& td = msg.params["textDocument"];
    std::string uri = td["uri"].asString();
    std::string languageId = td["languageId"].asString();
    int version = td["version"].asInt();
    std::string text = td["text"].asString();
    
    documents.open(uri, languageId, version, text);
    analyzer.analyze(uri, text);
}

void MagolorLanguageServer::handleDidChange(const Message& msg) {
    auto& td = msg.params["textDocument"];
    std::string uri = td["uri"].asString();
    int version = td["version"].asInt();
    
    auto& changes = msg.params["contentChanges"].asArray();
    if (!changes.empty()) {
        std::string text = changes[0]["text"].asString();
        documents.change(uri, version, text);
        analyzer.analyze(uri, text);
    }
}

void MagolorLanguageServer::handleDidClose(const Message& msg) {
    std::string uri = msg.params["textDocument"]["uri"].asString();
    documents.close(uri);
}

void MagolorLanguageServer::handleDidSave(const Message& msg) {
    std::string uri = msg.params["textDocument"]["uri"].asString();
    auto* doc = documents.get(uri);
    if (doc) {
        analyzer.analyze(uri, doc->content);
    }
}

void MagolorLanguageServer::handleCompletion(const Message& msg) {
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
    
    std::string lineText = doc->getLine(pos.line);
    JsonValue items = completion.provideCompletions(uri, pos, lineText);
    
    transport.respond(msg.id.value(), items);
}

void MagolorLanguageServer::handleHover(const Message& msg) {
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
    auto sym = analyzer.getSymbolAt(uri, pos);
    
    if (sym) {
        JsonValue result = JsonValue::object();
        JsonValue contents = JsonValue::object();
        contents["kind"] = "markdown";
        
        std::string md = "```magolor\n";
        if (sym->kind == SymbolKind::Function || sym->kind == SymbolKind::Method) {
            md += (sym->kind == SymbolKind::Method ? "method " : "fn ");
            md += sym->name + sym->detail + "\n";
        } else if (sym->kind == SymbolKind::Variable) {
            md += "let " + sym->name;
            if (!sym->type.empty()) {
                md += ": " + sym->type;
            }
            md += "\n";
        } else if (sym->kind == SymbolKind::Class) {
            md += "class " + sym->name + "\n";
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

void MagolorLanguageServer::handleDefinition(const Message& msg) {
    auto& td = msg.params["textDocument"];
    std::string uri = td["uri"].asString();
    Position pos;
    pos.line = msg.params["position"]["line"].asInt();
    pos.character = msg.params["position"]["character"].asInt();
    
    auto sym = analyzer.getSymbolAt(uri, pos);
    
    if (sym) {
        JsonValue loc = JsonValue::object();
        loc["uri"] = sym->definition.uri;
        loc["range"] = rangeToJson(sym->definition.range);
        transport.respond(msg.id.value(), loc);
    } else {
        transport.respond(msg.id.value(), JsonValue());
    }
}

void MagolorLanguageServer::handleReferences(const Message& msg) {
    auto& td = msg.params["textDocument"];
    std::string uri = td["uri"].asString();
    Position pos;
    pos.line = msg.params["position"]["line"].asInt();
    pos.character = msg.params["position"]["character"].asInt();
    
    auto sym = analyzer.getSymbolAt(uri, pos);
    
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

void MagolorLanguageServer::handleDocumentSymbol(const Message& msg) {
    std::string uri = msg.params["textDocument"]["uri"].asString();
    auto syms = analyzer.getAllSymbolsInFile(uri);
    
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

JsonValue MagolorLanguageServer::rangeToJson(const Range& r) {
    JsonValue json = JsonValue::object();
    json["start"] = JsonValue::object();
    json["start"]["line"] = r.start.line;
    json["start"]["character"] = r.start.character;
    json["end"] = JsonValue::object();
    json["end"]["line"] = r.end.line;
    json["end"]["character"] = r.end.character;
    return json;
}
