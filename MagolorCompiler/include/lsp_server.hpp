#pragma once
#include "jsonrpc.hpp"
#include "lsp_semantic.hpp"
#include "lsp_completion.hpp"
#include <unordered_map>
#include <string>

struct TextDocument {
    std::string uri;
    std::string languageId;
    int version = 0;
    std::string content;
    std::vector<size_t> lineOffsets;
    
    void updateLineOffsets() {
        lineOffsets.clear();
        lineOffsets.push_back(0);
        for (size_t i = 0; i < content.size(); i++) {
            if (content[i] == '\n') {
                lineOffsets.push_back(i + 1);
            }
        }
    }
    
    std::string getLine(int line) const {
        if (line < 0 || line >= (int)lineOffsets.size()) return "";
        size_t start = lineOffsets[line];
        size_t end = (line + 1 < (int)lineOffsets.size()) 
                     ? lineOffsets[line + 1] - 1 : content.size();
        return content.substr(start, end - start);
    }
    
    std::string getWordAt(const Position& pos) const {
        std::string line = getLine(pos.line);
        if (pos.character >= (int)line.size()) return "";
        
        int start = pos.character;
        int end = pos.character;
        
        while (start > 0 && isIdentChar(line[start - 1])) start--;
        while (end < (int)line.size() && isIdentChar(line[end])) end++;
        
        return line.substr(start, end - start);
    }
    
private:
    static bool isIdentChar(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
               (c >= '0' && c <= '9') || c == '_';
    }
};

class DocumentManager {
public:
    void open(const std::string& uri, const std::string& languageId,
              int version, const std::string& content) {
        TextDocument doc;
        doc.uri = uri;
        doc.languageId = languageId;
        doc.version = version;
        doc.content = content;
        doc.updateLineOffsets();
        documents[uri] = doc;
    }
    
    void change(const std::string& uri, int version, const std::string& content) {
        if (documents.find(uri) != documents.end()) {
            documents[uri].version = version;
            documents[uri].content = content;
            documents[uri].updateLineOffsets();
        }
    }
    
    void close(const std::string& uri) {
        documents.erase(uri);
    }
    
    TextDocument* get(const std::string& uri) {
        auto it = documents.find(uri);
        return it != documents.end() ? &it->second : nullptr;
    }

private:
    std::unordered_map<std::string, TextDocument> documents;
};

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
    SemanticAnalyzer analyzer;
    CompletionProvider completion{analyzer};
    bool running = false;
    bool initialized = false;
    
    void handleMessage(const Message& msg);
    void handleInitialize(const Message& msg);
    void handleInitialized(const Message& msg);
    void handleShutdown(const Message& msg);
    void handleExit(const Message& msg);
    void handleDidOpen(const Message& msg);
    void handleDidChange(const Message& msg);
    void handleDidClose(const Message& msg);
    void handleDidSave(const Message& msg);
    void handleCompletion(const Message& msg);
    void handleHover(const Message& msg);
    void handleDefinition(const Message& msg);
    void handleReferences(const Message& msg);
    void handleDocumentSymbol(const Message& msg);
    
    JsonValue rangeToJson(const Range& r);
};
