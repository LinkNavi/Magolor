#pragma once
#include "position.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>

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
    
    Position offsetToPosition(size_t offset) const {
        Position pos;
        for (size_t i = 0; i < lineOffsets.size(); i++) {
            if (i + 1 >= lineOffsets.size() || lineOffsets[i + 1] > offset) {
                pos.line = i;
                pos.character = offset - lineOffsets[i];
                break;
            }
        }
        return pos;
    }
    
    size_t positionToOffset(const Position& pos) const {
        if (pos.line >= (int)lineOffsets.size()) return content.size();
        return lineOffsets[pos.line] + pos.character;
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
    
    std::vector<std::string> getOpenUris() const {
        std::vector<std::string> uris;
        for (auto& [uri, _] : documents) {
            uris.push_back(uri);
        }
        return uris;
    }

private:
    std::unordered_map<std::string, TextDocument> documents;
};
