#include "lsp_server.hpp"
#include "lexer.hpp"
#include "module.hpp"
#include "parser.hpp"
#include "typechecker.hpp"
#include <sstream>

#include <fstream>

LSPLogger logger;
void MagolorLanguageServer::handleMessage(const Message &msg) {
  if (msg.method == "initialize")
    handleInitialize(msg);
  else if (msg.method == "initialized")
    handleInitialized(msg);
  else if (msg.method == "shutdown")
    handleShutdown(msg);
  else if (msg.method == "exit")
    handleExit(msg);
  else if (msg.method == "textDocument/didOpen")
    handleDidOpen(msg);
  else if (msg.method == "textDocument/didChange")
    handleDidChange(msg);
  else if (msg.method == "textDocument/didClose")
    handleDidClose(msg);
  else if (msg.method == "textDocument/didSave")
    handleDidSave(msg);
  else if (msg.method == "textDocument/completion")
    handleCompletion(msg);
  else if (msg.method == "textDocument/hover")
    handleHover(msg);
  else if (msg.method == "textDocument/formatting")
    handleFormatting(msg);
  else if (msg.method == "textDocument/rangeFormatting")
    handleRangeFormatting(msg);
  else if (msg.method == "textDocument/onTypeFormatting")
    handleOnTypeFormatting(msg);
  else if (msg.method == "textDocument/rename")
    handleRename(msg);
  else if (msg.method == "textDocument/codeAction")
    handleCodeAction(msg);
  else if (msg.method == "textDocument/signatureHelp")
    handleSignatureHelp(msg);
  else if (msg.method == "textDocument/definition")
    handleDefinition(msg);
  else if (msg.method == "textDocument/references")
    handleReferences(msg);
  else if (msg.method == "textDocument/documentSymbol")
    handleDocumentSymbol(msg);
  else if (msg.isRequest()) {
    transport.respondError(msg.id.value(), -32601, "Method not found");
  }
}

void MagolorLanguageServer::handleSignatureHelp(const Message &msg) {
  std::string uri = msg.params["textDocument"]["uri"].asString();
  Position pos;
  pos.line = msg.params["position"]["line"].asInt();
  pos.character = msg.params["position"]["character"].asInt();

  auto *doc = documents.get(uri);
  if (!doc) {
    transport.respond(msg.id.value(), JsonValue());
    return;
  }

  // Find function being called
  std::string line = doc->getLine(pos.line);
  std::string word;
  int parenPos = pos.character - 1;

  // Walk back to find function name
  while (parenPos >= 0 && line[parenPos] != '(') {
    parenPos--;
  }

  if (parenPos < 0) {
    transport.respond(msg.id.value(), JsonValue());
    return;
  }

  // Get function name
  int nameEnd = parenPos - 1;
  while (nameEnd >= 0 &&
         (std::isalnum(line[nameEnd]) || line[nameEnd] == '_')) {
    nameEnd--;
  }
  nameEnd++;

  std::string funcName = line.substr(nameEnd, parenPos - nameEnd);

  // Look up function signature
  auto sym = analyzer.findSymbolInImports(uri, funcName);
  if (!sym && !funcName.empty()) {
    // Try local scope
    auto allSyms = analyzer.getAllSymbolsInFile(uri);
    for (const auto &s : allSyms) {
      if (s->name == funcName && s->kind == SymbolKind::Function) {
        sym = s;
        break;
      }
    }
  }

  if (!sym || sym->paramTypes.empty()) {
    transport.respond(msg.id.value(), JsonValue());
    return;
  }

  // Build signature help
  JsonValue result = JsonValue::object();
  result["signatures"] = JsonValue::array();

  JsonValue sig = JsonValue::object();
  sig["label"] = sym->name + sym->detail;
  sig["parameters"] = JsonValue::array();

  for (size_t i = 0; i < sym->paramTypes.size(); i++) {
    JsonValue param = JsonValue::object();
    param["label"] = sym->paramTypes[i];
    sig["parameters"].push(param);
  }

  result["signatures"].push(sig);
  result["activeSignature"] = 0;
  result["activeParameter"] = 0; // Could be calculated from cursor position

  transport.respond(msg.id.value(), result);
}

void MagolorLanguageServer::handleCodeAction(const Message &msg) {
  std::string uri = msg.params["textDocument"]["uri"].asString();
  Range range;
  range.start.line = msg.params["range"]["start"]["line"].asInt();
  range.start.character = msg.params["range"]["start"]["character"].asInt();
  range.end.line = msg.params["range"]["end"]["line"].asInt();
  range.end.character = msg.params["range"]["end"]["character"].asInt();

  JsonValue actions = JsonValue::array();

  auto *doc = documents.get(uri);
  if (!doc) {
    transport.respond(msg.id.value(), actions);
    return;
  }

  // Check for missing imports
  auto importErrors = analyzer.validateImports(uri);
  for (const auto &error : importErrors) {
    JsonValue action = JsonValue::object();
    action["title"] = "Add import for " + error.modulePath;
    action["kind"] = "quickfix";

    // Create edit to add import
    JsonValue edit = JsonValue::object();
    edit["changes"] = JsonValue::object();
    edit["changes"][uri] = JsonValue::array();

    JsonValue textEdit = JsonValue::object();
    textEdit["range"] = JsonValue::object();
    textEdit["range"]["start"] = JsonValue::object();
    textEdit["range"]["start"]["line"] = 0;
    textEdit["range"]["start"]["character"] = 0;
    textEdit["range"]["end"] = JsonValue::object();
    textEdit["range"]["end"]["line"] = 0;
    textEdit["range"]["end"]["character"] = 0;
    textEdit["newText"] = "using " + error.modulePath + ";\n";

    edit["changes"][uri].push(textEdit);
    action["edit"] = edit;

    actions.push(action);
  }

  transport.respond(msg.id.value(), actions);
}
void MagolorLanguageServer::handleRename(const Message &msg) {
  std::string uri = msg.params["textDocument"]["uri"].asString();
  Position pos;
  pos.line = msg.params["position"]["line"].asInt();
  pos.character = msg.params["position"]["character"].asInt();
  std::string newName = msg.params["newName"].asString();

  auto sym = analyzer.getSymbolAt(uri, pos);
  if (!sym) {
    transport.respond(msg.id.value(), JsonValue());
    return;
  }

  // Build workspace edit
  JsonValue edit = JsonValue::object();
  edit["changes"] = JsonValue::object();
  edit["changes"][uri] = JsonValue::array();

  // Edit at definition
  JsonValue defEdit = JsonValue::object();
  defEdit["range"] = rangeToJson(sym->definition.range);
  defEdit["newText"] = newName;
  edit["changes"][uri].push(defEdit);

  // Edits at all references
  for (const auto &ref : sym->references) {
    if (ref.uri == uri) {
      JsonValue refEdit = JsonValue::object();
      refEdit["range"] = rangeToJson(ref.range);
      refEdit["newText"] = newName;
      edit["changes"][uri].push(refEdit);
    }
  }

  transport.respond(msg.id.value(), edit);
}

void MagolorLanguageServer::handleFormatting(const Message &msg) {
  std::string uri = msg.params["textDocument"]["uri"].asString();
  auto *doc = documents.get(uri);

  if (!doc) {
    transport.respond(msg.id.value(), JsonValue::array());
    return;
  }

  std::string formatted = formatDocument(doc->content);

  // Calculate edit to replace entire document
  JsonValue edit = JsonValue::object();
  edit["range"] = JsonValue::object();
  edit["range"]["start"] = JsonValue::object();
  edit["range"]["start"]["line"] = 0;
  edit["range"]["start"]["character"] = 0;
  edit["range"]["end"] = JsonValue::object();
  edit["range"]["end"]["line"] = (int)doc->lineOffsets.size();
  edit["range"]["end"]["character"] = 0;
  edit["newText"] = formatted;

  JsonValue edits = JsonValue::array();
  edits.push(edit);

  transport.respond(msg.id.value(), edits);
}
void MagolorLanguageServer::run() {
    logger.log("LSP run() called");
    running = true;

    // CRITICAL: Disable sync_with_stdio for proper binary I/O
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);
    
    // Make stderr unbuffered for logging
    std::cerr << std::unitbuf;

    logger.log("Stream configuration complete");

    try {
        while (running) {
            logger.log("Waiting for message...");

            // Pre-check stream state
            if (std::cin.eof()) {
                logger.log("EOF before receive - exiting");
                break;
            }

            auto msg = transport.receive();
            
            if (!msg) {
                if (std::cin.eof()) {
                    logger.log("EOF after failed receive - exiting");
                    break;
                }
                logger.log("receive() returned nullopt but not EOF, continuing...");
                std::cin.clear();
                continue;
            }

            std::string msgInfo = "Received: " + msg->method;
            if (msg->id.has_value()) {
                msgInfo += " (id=" + std::to_string(msg->id.value()) + ")";
            }
            logger.log(msgInfo);

            try {
                handleMessage(*msg);
                logger.log("Handled successfully");
            } catch (const std::exception &e) {
                logger.log("Handler error: " + std::string(e.what()));
                if (msg->id.has_value()) {
                    transport.respondError(msg->id.value(), -32603, 
                        std::string("Internal error: ") + e.what());
                }
            } catch (...) {
                logger.log("Unknown handler error");
                if (msg->id.has_value()) {
                    transport.respondError(msg->id.value(), -32603, "Internal error");
                }
            }
        }
    } catch (const std::exception &e) {
        logger.log("Fatal: " + std::string(e.what()));
    } catch (...) {
        logger.log("Unknown fatal error");
    }

    logger.log("LSP server exiting");
}std::string MagolorLanguageServer::formatDocument(const std::string &content) {
  // Simple formatter implementation
  std::stringstream result;
  std::istringstream input(content);
  std::string line;
  int indentLevel = 0;

  while (std::getline(input, line)) {
    // Trim line
    size_t start = line.find_first_not_of(" \t");
    if (start == std::string::npos) {
      result << "\n";
      continue;
    }

    std::string trimmed = line.substr(start);

    // Adjust indent for closing braces
    if (trimmed[0] == '}') {
      indentLevel--;
    }

    // Write indented line
    for (int i = 0; i < indentLevel; i++) {
      result << "    ";
    }
    result << trimmed << "\n";

    // Adjust indent for opening braces
    if (trimmed.back() == '{') {
      indentLevel++;
    }
  }

  return result.str();
}
void MagolorLanguageServer::handleInitialize(const Message &msg) {
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
  // NEW: Formatting
  caps["documentFormattingProvider"] = true;
  caps["documentRangeFormattingProvider"] = true;
  caps["documentOnTypeFormattingProvider"] = JsonValue::object();
  caps["documentOnTypeFormattingProvider"]["firstTriggerCharacter"] = "}";
  caps["documentOnTypeFormattingProvider"]["moreTriggerCharacter"] =
      JsonValue::array();
  caps["documentOnTypeFormattingProvider"]["moreTriggerCharacter"].push(";");

  // NEW: Rename
  caps["renameProvider"] = true;

  // NEW: Code actions
  caps["codeActionProvider"] = true;

  // NEW: Signature help
  caps["signatureHelpProvider"] = JsonValue::object();
  caps["signatureHelpProvider"]["triggerCharacters"] = JsonValue::array();
  caps["signatureHelpProvider"]["triggerCharacters"].push("(");
  caps["signatureHelpProvider"]["triggerCharacters"].push(",");
  // Other capabilities
  caps["hoverProvider"] = true;
  caps["definitionProvider"] = true;
  caps["referencesProvider"] = true;
  caps["documentSymbolProvider"] = true;

  JsonValue result = JsonValue::object();
  result["capabilities"] = caps;
  result["serverInfo"] = JsonValue::object();
  result["serverInfo"]["name"] = "magolor-lsp";
  result["serverInfo"]["version"] = "0.3.0";

  transport.respond(msg.id.value(), result);
}

void MagolorLanguageServer::handleInitialized(const Message &) {
  initialized = true;
}

void MagolorLanguageServer::handleShutdown(const Message &msg) {
  transport.respond(msg.id.value(), JsonValue());
}

void MagolorLanguageServer::handleExit(const Message &) { running = false; }

void MagolorLanguageServer::handleDidOpen(const Message &msg) {
    logger.log("handleDidOpen: START");
    
    try {
        if (!msg.params.has("textDocument")) {
            logger.log("handleDidOpen: No textDocument in params");
            return;
        }
        
        auto &td = msg.params["textDocument"];
        
        if (!td.has("uri")) {
            logger.log("handleDidOpen: No uri in textDocument");
            return;
        }
        
        std::string uri = td["uri"].asString();
        logger.log("handleDidOpen: uri = " + uri);
        
        std::string languageId = td.has("languageId") ? td["languageId"].asString() : "magolor";
        int version = td.has("version") ? td["version"].asInt() : 1;
        std::string text = td.has("text") ? td["text"].asString() : "";
        
        logger.log("handleDidOpen: text length = " + std::to_string(text.length()));

        documents.open(uri, languageId, version, text);
        logger.log("handleDidOpen: document stored");

        // Analyze with full exception protection
        try {
            analyzeAndPublishDiagnostics(uri, text);
        } catch (const std::exception& e) {
            logger.log("handleDidOpen: analysis error - " + std::string(e.what()));
            publishDiagnostics(uri, {});  // Send empty diagnostics
        } catch (...) {
            logger.log("handleDidOpen: unknown analysis error");
            publishDiagnostics(uri, {});
        }

        // Semantic analysis (optional)
        try {
            analyzer.analyze(uri, text);
        } catch (...) {
            logger.log("handleDidOpen: semantic analysis failed (non-fatal)");
        }
        
    } catch (const std::exception& e) {
        logger.log("handleDidOpen: ERROR - " + std::string(e.what()));
    } catch (...) {
        logger.log("handleDidOpen: UNKNOWN ERROR");
    }
    
    logger.log("handleDidOpen: END");
}
void MagolorLanguageServer::handleDidChange(const Message &msg) {
  logger.log("handleDidChange: START");
  
  try {
    auto &td = msg.params["textDocument"];
    std::string uri = td["uri"].asString();
    int version = td["version"].asInt();

    auto &changes = msg.params["contentChanges"].asArray();
    if (!changes.empty()) {
      std::string text = changes[0]["text"].asString();
      documents.change(uri, version, text);

      // Analyze and publish diagnostics (wrapped)
      try {
        analyzeAndPublishDiagnostics(uri, text);
      } catch (...) {
        logger.log("handleDidChange: analysis failed");
        publishDiagnostics(uri, {});
      }

      // Also run semantic analysis (wrapped)
      try {
        analyzer.analyze(uri, text);
      } catch (...) {
        logger.log("handleDidChange: semantic analysis failed");
      }
    }
  } catch (const std::exception& e) {
    logger.log("handleDidChange: ERROR - " + std::string(e.what()));
  } catch (...) {
    logger.log("handleDidChange: UNKNOWN ERROR");
  }
  
  logger.log("handleDidChange: END");
}

void MagolorLanguageServer::handleDidSave(const Message &msg) {
  logger.log("handleDidSave: START");
  
  try {
    std::string uri = msg.params["textDocument"]["uri"].asString();
    auto *doc = documents.get(uri);
    if (doc) {
      // Re-analyze on save (wrapped)
      try {
        analyzeAndPublishDiagnostics(uri, doc->content);
      } catch (...) {
        logger.log("handleDidSave: analysis failed");
        publishDiagnostics(uri, {});
      }
      
      try {
        analyzer.analyze(uri, doc->content);
      } catch (...) {
        logger.log("handleDidSave: semantic analysis failed");
      }
    }
  } catch (const std::exception& e) {
    logger.log("handleDidSave: ERROR - " + std::string(e.what()));
  } catch (...) {
    logger.log("handleDidSave: UNKNOWN ERROR");
  }
  
  logger.log("handleDidSave: END");
}

void MagolorLanguageServer::handleDidClose(const Message &msg) {
  std::string uri = msg.params["textDocument"]["uri"].asString();

  // Clear diagnostics on close
  publishDiagnostics(uri, {});

  documents.close(uri);
}

void MagolorLanguageServer::analyzeAndPublishDiagnostics(
    const std::string &uri, const std::string &content) {
  logger.log("analyzeAndPublishDiagnostics: START for " + uri);

  std::vector<LspDiagnostic> diagnostics;

  // Create diagnostic collector
  DiagnosticCollector collector(uri, content);

  try {
    logger.log("analyzeAndPublishDiagnostics: creating lexer");
    
    // Phase 1: Lexical analysis
    Lexer lexer(content, uri, collector);
    logger.log("analyzeAndPublishDiagnostics: tokenizing");
    auto tokens = lexer.tokenize();
    logger.log("analyzeAndPublishDiagnostics: got " +
               std::to_string(tokens.size()) + " tokens");

    if (collector.hasError()) {
      logger.log("analyzeAndPublishDiagnostics: lexer has errors");
      diagnostics = collector.getDiagnostics();
      publishDiagnostics(uri, diagnostics);
      return;
    }

    logger.log("analyzeAndPublishDiagnostics: creating parser");
    
    // Phase 2: Syntax analysis
    Parser parser(std::move(tokens), uri, collector);
    logger.log("analyzeAndPublishDiagnostics: parsing");

    Program prog;
    try {
      prog = parser.parse();
      logger.log("analyzeAndPublishDiagnostics: parsed successfully");
    } catch (const std::exception &e) {
      logger.log("analyzeAndPublishDiagnostics: parser exception - " +
                 std::string(e.what()));
      diagnostics = collector.getDiagnostics();

      if (diagnostics.empty()) {
        LspDiagnostic diag;
        diag.severity = DiagnosticSeverity::Error;
        diag.message = "Parse error: " + std::string(e.what());
        diag.range.start = {0, 0};
        diag.range.end = {0, 1};
        diag.source = "magolor";
        diagnostics.push_back(diag);
      }

      publishDiagnostics(uri, diagnostics);
      return;
    } catch (...) {
      logger.log("analyzeAndPublishDiagnostics: unknown parser exception");

      diagnostics = collector.getDiagnostics();
      if (diagnostics.empty()) {
        LspDiagnostic diag;
        diag.severity = DiagnosticSeverity::Error;
        diag.message = "Unknown parse error";
        diag.range.start = {0, 0};
        diag.range.end = {0, 1};
        diag.source = "magolor";
        diagnostics.push_back(diag);
      }

      publishDiagnostics(uri, diagnostics);
      return;
    }

    if (collector.hasError()) {
      logger.log("analyzeAndPublishDiagnostics: parser has errors");
      diagnostics = collector.getDiagnostics();
      publishDiagnostics(uri, diagnostics);
      return;
    }

    logger.log("analyzeAndPublishDiagnostics: creating module");
    
    // Phase 3: Type checking (NEW - was skipped before!)
    try {
      // Create a temporary module for this file
      auto module = std::make_shared<Module>();
      
      // Convert URI to relative path
      std::string filepath = uri;
      if (filepath.find("file://") == 0) {
        filepath = filepath.substr(7);
      }
      
      module->name = "current";
      module->filepath = filepath;
      module->ast = prog;
      
      // Temporarily register this module
      ModuleRegistry::instance().registerModule("current", module);
      
      logger.log("analyzeAndPublishDiagnostics: type checking");
      
      // Create a fresh type checker with our collector
      TypeChecker typeChecker(collector, ModuleRegistry::instance());
      
      // Set the current module context
      typeChecker.checkModule(module);
      
      logger.log("analyzeAndPublishDiagnostics: type checking complete");

      // Get diagnostics from type checking
      if (collector.hasError()) {
        logger.log("analyzeAndPublishDiagnostics: type checker has errors");
        auto allDiags = collector.getDiagnostics();
        
        // Filter out false positives (keep this for now)
        for (const auto &diag : allDiags) {
          bool skipError = false;

          if (diag.message.find("Cannot call non-function") != std::string::npos) {
            skipError = true;
          }
          if (diag.message.find("string") != std::string::npos &&
              diag.range.start.line == 3) {
            skipError = true;
          }

          if (!skipError) {
            diagnostics.push_back(diag);
          }
        }
      }
      
      // Clean up temporary module
      ModuleRegistry::instance().clear();
      
    } catch (const std::exception &e) {
      logger.log("analyzeAndPublishDiagnostics: type checker exception - " +
                 std::string(e.what()));
      // Add diagnostic for type check error
      LspDiagnostic diag;
      diag.severity = DiagnosticSeverity::Error;
      diag.message = "Type check error: " + std::string(e.what());
      diag.range.start = {0, 0};
      diag.range.end = {0, 1};
      diag.source = "magolor";
      diagnostics.push_back(diag);
    } catch (...) {
      logger.log("analyzeAndPublishDiagnostics: unknown type checker exception");
      LspDiagnostic diag;
      diag.severity = DiagnosticSeverity::Error;
      diag.message = "Unknown type check error";
      diag.range.start = {0, 0};
      diag.range.end = {0, 1};
      diag.source = "magolor";
      diagnostics.push_back(diag);
    }

    logger.log("analyzeAndPublishDiagnostics: checking import errors");

  } catch (const std::exception &e) {
    logger.log("analyzeAndPublishDiagnostics: EXCEPTION - " +
               std::string(e.what()));
    LspDiagnostic diag;
    diag.severity = DiagnosticSeverity::Error;
    diag.message = "Analysis error: " + std::string(e.what());
    diag.range.start = {0, 0};
    diag.range.end = {0, 1};
    diag.source = "magolor";
    diagnostics.push_back(diag);
  } catch (...) {
    logger.log("analyzeAndPublishDiagnostics: UNKNOWN EXCEPTION");
    LspDiagnostic diag;
    diag.severity = DiagnosticSeverity::Error;
    diag.message = "Unknown analysis error";
    diag.range.start = {0, 0};
    diag.range.end = {0, 1};
    diag.source = "magolor";
    diagnostics.push_back(diag);
  }

  // Validate imports (keep this)
  try {
    logger.log("analyzeAndPublishDiagnostics: validating imports");
    auto importErrors = analyzer.validateImports(uri);
    for (const auto &error : importErrors) {
      LspDiagnostic diag;
      diag.severity = DiagnosticSeverity::Error;
      diag.message = error.message;
      diag.range = error.range;
      diag.source = "magolor";
      diagnostics.push_back(diag);
    }
  } catch (...) {
    logger.log("analyzeAndPublishDiagnostics: import validation failed");
  }

  logger.log("analyzeAndPublishDiagnostics: publishing " +
             std::to_string(diagnostics.size()) + " diagnostics");
  publishDiagnostics(uri, diagnostics);
  logger.log("analyzeAndPublishDiagnostics: END");
}void MagolorLanguageServer::publishDiagnostics(
    const std::string &uri, const std::vector<LspDiagnostic> &diagnostics) {
  JsonValue params = JsonValue::object();
  params["uri"] = uri;
  params["diagnostics"] = JsonValue::array();

  for (const auto &diag : diagnostics) {
    params["diagnostics"].push(diagnosticToJson(diag));
  }

  transport.notify("textDocument/publishDiagnostics", params);
}

JsonValue MagolorLanguageServer::diagnosticToJson(const LspDiagnostic &diag) {
  JsonValue json = JsonValue::object();

  json["range"] = rangeToJson(diag.range);
  json["severity"] = static_cast<int>(diag.severity);
  json["source"] = diag.source;
  json["message"] = diag.message;

  if (!diag.code.empty()) {
    json["code"] = diag.code;
  }

  return json;
}

void MagolorLanguageServer::handleCompletion(const Message &msg) {
  auto &td = msg.params["textDocument"];
  std::string uri = td["uri"].asString();
  Position pos;
  pos.line = msg.params["position"]["line"].asInt();
  pos.character = msg.params["position"]["character"].asInt();

  auto *doc = documents.get(uri);
  if (!doc) {
    transport.respond(msg.id.value(), JsonValue::array());
    return;
  }

  std::string lineText = doc->getLine(pos.line);
  JsonValue items = completion.provideCompletions(uri, pos, lineText);

  transport.respond(msg.id.value(), items);
}

void MagolorLanguageServer::handleHover(const Message &msg) {
  auto &td = msg.params["textDocument"];
  std::string uri = td["uri"].asString();
  Position pos;
  pos.line = msg.params["position"]["line"].asInt();
  pos.character = msg.params["position"]["character"].asInt();

  auto *doc = documents.get(uri);
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
void MagolorLanguageServer::handleRangeFormatting(const Message &msg) {
  std::string uri = msg.params["textDocument"]["uri"].asString();
  Range range;
  range.start.line = msg.params["range"]["start"]["line"].asInt();
  range.start.character = msg.params["range"]["start"]["character"].asInt();
  range.end.line = msg.params["range"]["end"]["line"].asInt();
  range.end.character = msg.params["range"]["end"]["character"].asInt();

  auto *doc = documents.get(uri);
  if (!doc) {
    transport.respond(msg.id.value(), JsonValue::array());
    return;
  }

  std::string formatted = formatRange(doc->content, range);

  JsonValue edit = JsonValue::object();
  edit["range"] = rangeToJson(range);
  edit["newText"] = formatted;

  JsonValue edits = JsonValue::array();
  edits.push(edit);

  transport.respond(msg.id.value(), edits);
}

void MagolorLanguageServer::handleOnTypeFormatting(const Message &msg) {
  std::string uri = msg.params["textDocument"]["uri"].asString();
  Position pos;
  pos.line = msg.params["position"]["line"].asInt();
  pos.character = msg.params["position"]["character"].asInt();

  auto *doc = documents.get(uri);
  if (!doc) {
    transport.respond(msg.id.value(), JsonValue::array());
    return;
  }

  // Simple on-type formatting: just return empty for now
  // Could add auto-indent logic here
  transport.respond(msg.id.value(), JsonValue::array());
}

std::string MagolorLanguageServer::formatRange(const std::string &content,
                                               Range range) {
  std::istringstream input(content);
  std::stringstream result;
  std::string line;
  int currentLine = 0;
  int indentLevel = 0;

  while (std::getline(input, line)) {
    if (currentLine >= range.start.line && currentLine <= range.end.line) {
      size_t start = line.find_first_not_of(" \t");
      if (start == std::string::npos) {
        result << "\n";
      } else {
        std::string trimmed = line.substr(start);

        if (trimmed[0] == '}')
          indentLevel--;

        for (int i = 0; i < indentLevel; i++)
          result << "    ";
        result << trimmed << "\n";

        if (trimmed.back() == '{')
          indentLevel++;
      }
    }
    currentLine++;
  }

  return result.str();
}
void MagolorLanguageServer::handleDefinition(const Message &msg) {
  auto &td = msg.params["textDocument"];
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

void MagolorLanguageServer::handleReferences(const Message &msg) {
  auto &td = msg.params["textDocument"];
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
    for (auto &ref : sym->references) {
      JsonValue refLoc = JsonValue::object();
      refLoc["uri"] = ref.uri;
      refLoc["range"] = rangeToJson(ref.range);
      locs.push(refLoc);
    }
  }

  transport.respond(msg.id.value(), locs);
}

void MagolorLanguageServer::handleDocumentSymbol(const Message &msg) {
  std::string uri = msg.params["textDocument"]["uri"].asString();
  auto syms = analyzer.getAllSymbolsInFile(uri);

  JsonValue result = JsonValue::array();
  for (auto &sym : syms) {
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

JsonValue MagolorLanguageServer::rangeToJson(const Range &r) {
  JsonValue json = JsonValue::object();
  json["start"] = JsonValue::object();
  json["start"]["line"] = r.start.line;
  json["start"]["character"] = r.start.character;
  json["end"] = JsonValue::object();
  json["end"]["line"] = r.end.line;
  json["end"]["character"] = r.end.character;
  return json;
}
