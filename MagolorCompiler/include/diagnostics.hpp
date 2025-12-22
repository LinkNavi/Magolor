#pragma once
#include "position.hpp"
#include <string>
#include <vector>

enum class DiagnosticSeverity {
    Error = 1,
    Warning = 2,
    Information = 3,
    Hint = 4
};

struct Diagnostic {
    Range range;
    DiagnosticSeverity severity;
    std::string code;
    std::string source = "magolor";
    std::string message;
};

struct PublishDiagnosticsParams {
    std::string uri;
    std::vector<Diagnostic> diagnostics;
};
