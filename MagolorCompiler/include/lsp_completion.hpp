#pragma once
#include "lsp_semantic.hpp"
#include "jsonrpc.hpp"
#include <vector>
#include <string>

// Forward declarations - actual definitions are in lsp_semantic.hpp
// This prevents duplicate definitions
class SemanticAnalyzer;
struct Position;
class JsonValue;
enum class CompletionItemKind;
struct CompletionSnippet;

// CompletionProvider is already declared in lsp_semantic.hpp
// We just need the implementation in lsp_completion.cpp
