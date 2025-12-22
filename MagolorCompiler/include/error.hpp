#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <sstream>

enum class ErrorLevel {
    ERROR,
    WARNING,
    NOTE
};

struct SourceLocation {
    std::string file;
    int line;
    int col;
    int length;
};

struct Diagnostic {
    ErrorLevel level;
    std::string message;
    SourceLocation location;
    std::vector<std::pair<SourceLocation, std::string>> notes;
    std::string hint;
};

class ErrorReporter {
public:
    ErrorReporter(const std::string& filename, const std::string& source)
        : filename(filename), source(source) {
        buildLineIndex();
    }
    
    virtual ~ErrorReporter() = default;
    
    virtual void error(const std::string& msg, SourceLocation loc, const std::string& hint = "") {
        Diagnostic diag;
        diag.level = ErrorLevel::ERROR;
        diag.message = msg;
        diag.location = loc;
        diag.hint = hint;
        diagnostics.push_back(diag);
        hasErrors = true;
    }
    
    virtual void warning(const std::string& msg, SourceLocation loc) {
        Diagnostic diag;
        diag.level = ErrorLevel::WARNING;
        diag.message = msg;
        diag.location = loc;
        diagnostics.push_back(diag);
    }
    
    void addNote(const std::string& msg, SourceLocation loc) {
        if (!diagnostics.empty()) {
            diagnostics.back().notes.push_back({loc, msg});
        }
    }
    
    bool hasError() const { return hasErrors; }
    
    const std::vector<Diagnostic>& getDiagnosticList() const {
        return diagnostics;
    }
    
    void printDiagnostics() {
        for (const auto& diag : diagnostics) {
            printDiagnostic(diag);
        }
    }
    
protected:
    std::string filename;
    std::string source;
    std::vector<Diagnostic> diagnostics;
    std::vector<size_t> lineStarts;
    bool hasErrors = false;
    
    void buildLineIndex() {
        lineStarts.push_back(0);
        for (size_t i = 0; i < source.size(); i++) {
            if (source[i] == '\n') {
                lineStarts.push_back(i + 1);
            }
        }
    }
    
    std::string getLine(int lineNum) {
        if (lineNum < 1 || lineNum > (int)lineStarts.size()) return "";
        size_t start = lineStarts[lineNum - 1];
        size_t end = (lineNum < (int)lineStarts.size()) ? lineStarts[lineNum] - 1 : source.size();
        return source.substr(start, end - start);
    }
    
    std::string levelToString(ErrorLevel level) {
        switch (level) {
            case ErrorLevel::ERROR: return "\033[1;31merror\033[0m";
            case ErrorLevel::WARNING: return "\033[1;33mwarning\033[0m";
            case ErrorLevel::NOTE: return "\033[1;36mnote\033[0m";
        }
        return "error";
    }
    
    void printDiagnostic(const Diagnostic& diag) {
        // Header: error[E0XXX]: message
        std::cout << levelToString(diag.level) << ": " << diag.message << "\n";
        
        // Location: --> file:line:col
        std::cout << "  \033[1;34m-->\033[0m " << diag.location.file 
                  << ":" << diag.location.line 
                  << ":" << diag.location.col << "\n";
        
        // Line number and source code
        std::string lineNum = std::to_string(diag.location.line);
        std::string padding(lineNum.length() + 1, ' ');
        
        std::cout << padding << "\033[1;34m|\033[0m\n";
        std::cout << " \033[1;34m" << lineNum << " |\033[0m " << getLine(diag.location.line) << "\n";
        std::cout << padding << "\033[1;34m|\033[0m ";
        
        // Underline with carets
        for (int i = 0; i < diag.location.col - 1; i++) std::cout << " ";
        std::cout << "\033[1;31m";
        for (int i = 0; i < (diag.location.length > 0 ? diag.location.length : 1); i++) {
            std::cout << "^";
        }
        std::cout << "\033[0m\n";
        
        // Hint
        if (!diag.hint.empty()) {
            std::cout << padding << "\033[1;34m|\033[0m\n";
            std::cout << padding << "\033[1;34m= \033[1;37mhelp:\033[0m " << diag.hint << "\n";
        }
        
        // Additional notes
        for (const auto& note : diag.notes) {
            std::cout << "\n";
            std::cout << "\033[1;36mnote\033[0m: " << note.second << "\n";
            std::cout << "  \033[1;34m-->\033[0m " << note.first.file 
                      << ":" << note.first.line 
                      << ":" << note.first.col << "\n";
        }
        
        std::cout << "\n";
    }
};
