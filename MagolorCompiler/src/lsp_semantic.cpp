#include "lsp_semantic.hpp"
#include "lsp_logger.hpp"
#include "lsp_project.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
// logger is already declared extern in the header
namespace fs = std::filesystem;

// Helper function to convert TypePtr to string
std::string typeToString(const TypePtr &type) {
  if (!type)
    return "void";

  switch (type->kind) {
  case Type::INT:
    return "int";
  case Type::FLOAT:
    return "float";
  case Type::STRING:
    return "string";
  case Type::BOOL:
    return "bool";
  case Type::VOID:
    return "void";
  case Type::CLASS:
    return type->className;
  case Type::OPTION:
    return "Option<" + typeToString(type->innerType) + ">";
  case Type::ARRAY:
    return "Array<" + typeToString(type->innerType) + ">";
  case Type::FUNCTION: {
    std::string result = "fn(";
    for (size_t i = 0; i < type->paramTypes.size(); i++) {
      if (i > 0)
        result += ", ";
      result += typeToString(type->paramTypes[i]);
    }
    result += ") -> " + typeToString(type->returnType);
    return result;
  }
  }
  return "unknown";
}

void SemanticAnalyzer::loadProject(const std::string &startUri) {
  if (projectLoaded)
    return;

  // Find project root by looking for project.toml
  std::string path = startUri;
  if (path.find("file://") == 0) {
    path = path.substr(7);
  }

  fs::path current = fs::path(path).parent_path();
  while (!current.empty() && current.has_parent_path()) {
    if (fs::exists(current / "project.toml")) {
      projectRoot = current.string();
      break;
    }
    current = current.parent_path();
  }

  if (projectRoot.empty())
    return;

  // Scan src directory
  std::string srcDir = projectRoot + "/src";
  if (fs::exists(srcDir)) {
    scanSourceDirectory(srcDir);
  }

  projectLoaded = true;
}

void SemanticAnalyzer::scanSourceDirectory(const std::string &srcDir) {
  try {
    for (const auto &entry : fs::recursive_directory_iterator(srcDir)) {
      if (entry.is_regular_file() && entry.path().extension() == ".mg") {
        std::string filePath = entry.path().string();
        std::string uri = "file://" + filePath;

        // Skip if already analyzed
        if (fileSymbols.find(uri) != fileSymbols.end())
          continue;

        // Read and analyze
        std::ifstream file(filePath);
        if (file) {
          std::stringstream buffer;
          buffer << file.rdbuf();
          extractSymbols(uri, buffer.str());
        }
      }
    }
  } catch (const std::exception &e) {
    // Silently ignore filesystem errors
  }
}

void SemanticAnalyzer::reloadProject() {
  projectLoaded = false;
  projectRoot.clear();
  fileSymbols.clear();
  fileScopes.clear();
  moduleSymbols.clear();
}

void SemanticAnalyzer::analyze(const std::string &uri,
                               const std::string &content) {
  logger.log("SemanticAnalyzer::analyze START for " + uri);

  try {
    // Load entire project on first analyze
    logger.log("SemanticAnalyzer::analyze: loading project");
    
    try {
      loadProject(uri);
      logger.log("SemanticAnalyzer::analyze: project loaded");
    } catch (const std::exception &e) {
      logger.log("SemanticAnalyzer::analyze: project load failed - " + std::string(e.what()));
      // Continue anyway - we can still analyze this file
    } catch (...) {
      logger.log("SemanticAnalyzer::analyze: project load failed - unknown error");
      // Continue anyway
    }

    // Then analyze this specific file (updates cache)
    logger.log("SemanticAnalyzer::analyze: extracting symbols");
    
    try {
      extractSymbols(uri, content);
      logger.log("SemanticAnalyzer::analyze: symbols extracted");
    } catch (const std::exception &e) {
      logger.log("SemanticAnalyzer::analyze: symbol extraction failed - " + std::string(e.what()));
      // Mark as failed but don't crash
      fileSymbols[uri] = {};
      fileScopes[uri] = std::make_shared<Scope>();
    } catch (...) {
      logger.log("SemanticAnalyzer::analyze: symbol extraction failed - unknown error");
      fileSymbols[uri] = {};
      fileScopes[uri] = std::make_shared<Scope>();
    }

  } catch (const std::exception &e) {
    logger.log("SemanticAnalyzer::analyze: EXCEPTION - " + std::string(e.what()));
    // Don't re-throw - LSP must never crash
  } catch (...) {
    logger.log("SemanticAnalyzer::analyze: UNKNOWN EXCEPTION");
    // Don't re-throw
  }

  logger.log("SemanticAnalyzer::analyze END");
}
void SemanticAnalyzer::extractSymbols(const std::string &uri,
                                      const std::string &content) {
  logger.log("extractSymbols: START for " + uri);

  std::vector<SymbolPtr> symbols;
  auto scope = std::make_shared<Scope>();

  std::istringstream stream(content);
  std::string line;
  int lineNum = 0;
  std::string currentClass;

  logger.log("extractSymbols: parsing " + std::to_string(content.length()) + " bytes");

  while (std::getline(stream, line)) {
    lineNum++;
    
    // Log EVERY line to find the crash
    logger.log("extractSymbols: LINE " + std::to_string(lineNum) + ": " + 
               line.substr(0, std::min((size_t)50, line.size())));

    // Trim leading whitespace safely
    size_t firstNonSpace = line.find_first_not_of(" \t");
    std::string trimmedLine = (firstNonSpace != std::string::npos) 
                              ? line.substr(firstNonSpace) 
                              : "";

    try {
      if (trimmedLine.find("using ") == 0) {
        logger.log("extractSymbols: parsing import at line " + std::to_string(lineNum));
        parseImport(line, scope.get());
      } 
      else if (trimmedLine.find("cimport ") == 0) {
        continue;
      } 
      else if (trimmedLine.find("class ") != std::string::npos ||
               (trimmedLine.find("pub ") == 0 && trimmedLine.find("class ") != std::string::npos)) {
        logger.log("extractSymbols: parsing class at line " + std::to_string(lineNum));
        auto sym = parseClass(line, lineNum, uri);
        if (sym) {
          sym->isCallable = false;
          symbols.push_back(sym);
          scope->define(sym);
          currentClass = sym->name;
        }
      } 
      else if (trimmedLine.find("fn ") != std::string::npos) {
        logger.log("extractSymbols: parsing function at line " + std::to_string(lineNum));
        auto sym = parseFunction(line, lineNum, uri);
        if (sym) {
          sym->containerName = currentClass;
          sym->isCallable = true;
          sym->kind = currentClass.empty() ? SymbolKind::Function : SymbolKind::Method;
          symbols.push_back(sym);
          scope->define(sym);
        }
      } 
      else if (trimmedLine.find("let ") == 0) {
        logger.log("extractSymbols: parsing variable at line " + std::to_string(lineNum));
        auto sym = parseVariable(line, lineNum, uri);
        if (sym) {
          sym->containerName = currentClass;
          sym->isCallable = false;
          symbols.push_back(sym);
          scope->define(sym);
        }
      }

      // Check for class closing brace
      if (!currentClass.empty() && trimmedLine.find('}') != std::string::npos) {
        // Simple check: if line is just "}" or starts with "}"
        if (trimmedLine == "}" || trimmedLine[0] == '}') {
          logger.log("extractSymbols: closing class at line " + std::to_string(lineNum));
          currentClass = "";
        }
      }
    } catch (const std::exception &e) {
      logger.log("extractSymbols: EXCEPTION at line " + std::to_string(lineNum) + ": " + e.what());
    } catch (...) {
      logger.log("extractSymbols: UNKNOWN EXCEPTION at line " + std::to_string(lineNum));
    }
  }

  logger.log("extractSymbols: parsed " + std::to_string(symbols.size()) + " symbols");
  fileSymbols[uri] = symbols;
  fileScopes[uri] = scope;
  logger.log("extractSymbols: END");
}std::vector<SymbolPtr>
SemanticAnalyzer::getSymbolsFromModule(const std::string &modulePath) {
  std::vector<SymbolPtr> symbols;

  auto it = moduleSymbols.find(modulePath);
  if (it != moduleSymbols.end()) {
    return it->second;
  }

  auto project = ProjectManager::instance().getProjectForFile(projectRoot);
  if (!project) {
    return symbols;
  }

  auto exportedNames = project->getExportedSymbols(modulePath);
  for (const auto &name : exportedNames) {
    auto sym = std::make_shared<Symbol>();
    sym->name = name;
    sym->kind = SymbolKind::Function;
    sym->isPublic = true;
    symbols.push_back(sym);
  }

  moduleSymbols[modulePath] = symbols;
  return symbols;
}

std::vector<SymbolPtr>
SemanticAnalyzer::resolveImportedSymbols(const std::string &uri) {
  std::vector<SymbolPtr> symbols;

  auto it = fileScopes.find(uri);
  if (it == fileScopes.end()) {
    return symbols;
  }

  Scope *scope = it->second.get();

  for (const auto &import : scope->imports) {
    // For each imported symbol name, find the actual symbol in our cache
    for (const auto &symName : import.importedSymbols) {
      // Search all cached files for this symbol
      for (const auto &[fileUri, fileSyms] : fileSymbols) {
        for (const auto &sym : fileSyms) {
          if (sym->name == symName &&
              (sym->isPublic || sym->kind == SymbolKind::Function)) {
            symbols.push_back(sym);
            break;
          }
        }
      }
    }
  }

  // Also check project registry for modules
  auto project = ProjectManager::instance().getProjectForFile(uri);
  if (project) {
    for (const auto &import : scope->imports) {
      if (ModuleResolver::isBuiltinModule(import.fullPath)) {
        continue;
      }

      auto module = project->registry.getModule(import.fullPath);
      if (!module)
        continue;

      for (const auto &fn : module->ast.functions) {
        if (fn.isPublic) {
          auto sym = std::make_shared<Symbol>();
          sym->name = fn.name;
          sym->kind = SymbolKind::Function;
          sym->isPublic = true;
          sym->isCallable = true;

          sym->detail = "(";
          for (size_t i = 0; i < fn.params.size(); i++) {
            if (i > 0)
              sym->detail += ", ";
            sym->detail +=
                fn.params[i].name + ": " + typeToString(fn.params[i].type);
          }
          sym->detail += ") -> " + typeToString(fn.returnType);

          sym->definition.uri = module->filepath;
          sym->definition.range.start.line = fn.loc.line;
          sym->definition.range.start.character = fn.loc.col;

          symbols.push_back(sym);
        }
      }

      for (const auto &cls : module->ast.classes) {
        if (cls.isPublic) {
          auto sym = std::make_shared<Symbol>();
          sym->name = cls.name;
          sym->kind = SymbolKind::Class;
          sym->isPublic = true;
          sym->isCallable = false;

          sym->definition.uri = module->filepath;
          sym->definition.range.start.line = cls.loc.line;
          sym->definition.range.start.character = cls.loc.col;

          symbols.push_back(sym);
        }
      }
    }
  }

  return symbols;
}

SymbolPtr SemanticAnalyzer::findSymbolInImports(const std::string &uri,
                                                const std::string &symbolName) {
  auto importedSymbols = resolveImportedSymbols(uri);

  for (const auto &sym : importedSymbols) {
    if (sym->name == symbolName) {
      return sym;
    }
  }

  return nullptr;
}

std::vector<SemanticAnalyzer::ImportError>
SemanticAnalyzer::validateImports(const std::string &uri) {
  std::vector<ImportError> errors;

  auto project = ProjectManager::instance().getProjectForFile(uri);
  if (!project) {
    return errors;
  }

  auto validationErrors = project->validateImports(uri);
  for (const auto &error : validationErrors) {
    ImportError ie;
    ie.message = error;
    size_t pos = error.find("Cannot find module: ");
    if (pos != std::string::npos) {
      ie.modulePath = error.substr(pos + 20);
    }
    errors.push_back(ie);
  }

  return errors;
}

SymbolPtr SemanticAnalyzer::parseFunction(const std::string &line, int lineNum,
                                          const std::string &uri) {
  size_t fnPos = line.find("fn ");
  if (fnPos == std::string::npos)
    return nullptr;

  size_t nameStart = fnPos + 3;
  size_t nameEnd = line.find('(', nameStart);
  if (nameEnd == std::string::npos)
    return nullptr;

  std::string name = line.substr(nameStart, nameEnd - nameStart);
  name.erase(0, name.find_first_not_of(" \t"));
  name.erase(name.find_last_not_of(" \t") + 1);

  if (name.empty())
    return nullptr;

  auto sym = std::make_shared<Symbol>();
  sym->name = name;
  sym->kind = SymbolKind::Function;
  sym->definition.uri = uri;
  sym->definition.range.start = {lineNum, (int)nameStart};
  sym->definition.range.end = {lineNum, (int)nameEnd};

  sym->isPublic = line.find("pub ") != std::string::npos;
  sym->isStatic = line.find("static ") != std::string::npos;

  return sym;
}

SymbolPtr SemanticAnalyzer::parseClass(const std::string &line, int lineNum,
                                       const std::string &uri) {
  size_t classPos = line.find("class ");
  if (classPos == std::string::npos)
    return nullptr;

  size_t nameStart = classPos + 6;
  size_t nameEnd = line.find_first_of(" {", nameStart);
  if (nameEnd == std::string::npos)
    return nullptr;

  std::string name = line.substr(nameStart, nameEnd - nameStart);
  name.erase(name.find_last_not_of(" \t") + 1);

  auto sym = std::make_shared<Symbol>();
  sym->name = name;
  sym->kind = SymbolKind::Class;
  sym->definition.uri = uri;
  sym->definition.range.start = {lineNum, (int)nameStart};
  sym->definition.range.end = {lineNum, (int)nameEnd};
  sym->isPublic = line.find("pub ") != std::string::npos;

  return sym;
}

SymbolPtr SemanticAnalyzer::parseVariable(const std::string &line, int lineNum,
                                          const std::string &uri) {
   size_t letPos = line.find("let ");
  if (letPos == std::string::npos)
    return nullptr;

  size_t nameStart = letPos + 4;

  // Skip 'mut' keyword if present
  size_t mutPos = line.find("mut ", nameStart);
  if (mutPos == nameStart) {
    nameStart += 4;
  }

  // Skip any leading whitespace after 'let' or 'mut'
  while (nameStart < line.size() &&
         (line[nameStart] == ' ' || line[nameStart] == '\t')) {
    nameStart++;
  }

  // Find the end of the variable name - it's the first character that's not alphanumeric or underscore
  size_t nameEnd = nameStart;
  while (nameEnd < line.size() && 
         (std::isalnum(line[nameEnd]) || line[nameEnd] == '_')) {
    nameEnd++;
  }

  if (nameEnd == nameStart) {
    logger.log("parseVariable: no variable name found at line " +
               std::to_string(lineNum));
    return nullptr;
  }

  std::string name = line.substr(nameStart, nameEnd - nameStart);

  // Check if name is empty (shouldn't happen after the check above, but be safe)
  if (name.empty()) {
    logger.log("parseVariable: empty variable name at line " +
               std::to_string(lineNum));
    return nullptr;
  }

  // Skip whitespace after name
  while (nameEnd < line.size() && 
         (line[nameEnd] == ' ' || line[nameEnd] == '\t')) {
    nameEnd++;
  }

  // Now we should have either : (type annotation) or = (assignment)
  if (nameEnd >= line.size() || (line[nameEnd] != ':' && line[nameEnd] != '=')) {
    logger.log("parseVariable: expected ':' or '=' after variable name at line " +
               std::to_string(lineNum));
    return nullptr;
  }

  // Check for spaces in the name (indicates C++/Java style: "let Type name")
  // But be more lenient - only reject if there are MULTIPLE words
  size_t spaceCount = 0;
  for (char c : name) {
    if (c == ' ' || c == '\t')
      spaceCount++;
  }

  if (spaceCount > 0) {
    logger.log("parseVariable: invalid variable declaration syntax at line " +
               std::to_string(lineNum) +
               " - possible C++/Java style type before name (name='" + name +
               "')");
    return nullptr;
  }

  auto sym = std::make_shared<Symbol>();
  sym->name = name;
  sym->kind = SymbolKind::Variable;
  sym->definition.uri = uri;
  sym->definition.range.start = {lineNum - 1, (int)nameStart}; // LSP is 0-based
  sym->definition.range.end = {lineNum - 1, (int)nameEnd};

 // Try to extract type
  try {
    size_t equalPos = line.find('=', nameEnd);
    size_t colonPos = line.find(':', nameEnd);
    
    // Check if colon comes before equal sign (explicit type annotation)
    if (colonPos != std::string::npos && 
        (equalPos == std::string::npos || colonPos < equalPos)) {
      // Has explicit type annotation
      size_t typeEnd = equalPos;
      if (typeEnd == std::string::npos)
        typeEnd = line.find(';', colonPos);
      if (typeEnd == std::string::npos)
        typeEnd = line.size();

      if (colonPos + 1 < typeEnd) {
        sym->type = line.substr(colonPos + 1, typeEnd - colonPos - 1);
        sym->type.erase(0, sym->type.find_first_not_of(" \t"));
        if (!sym->type.empty()) {
          size_t lastNonSpace = sym->type.find_last_not_of(" \t");
          if (lastNonSpace != std::string::npos) {
            sym->type = sym->type.substr(0, lastNonSpace + 1);
          }
        }
      }
    } else if (equalPos != std::string::npos) {
      // Try to infer type from initialization
      size_t newPos = line.find("new ", equalPos);
      if (newPos != std::string::npos) {
        size_t classStart = newPos + 4;
        if (classStart < line.size()) {
          size_t classEnd = line.find_first_of("( \t;", classStart);
          if (classEnd == std::string::npos) {
            classEnd = line.size();
          }
          if (classEnd > classStart) {
            sym->type = line.substr(classStart, classEnd - classStart);
          }
        }
      } else {
        // Check for ClassName.new() pattern (like SlateDB.new())
        size_t dotNewPos = line.find(".new(", equalPos);
        if (dotNewPos != std::string::npos) {
          size_t classStart = equalPos + 1;
          while (classStart < dotNewPos && 
                 (line[classStart] == ' ' || line[classStart] == '\t')) {
            classStart++;
          }
          if (classStart < dotNewPos) {
            sym->type = line.substr(classStart, dotNewPos - classStart);
          }
        }
      }
    }
  } catch (const std::exception &e) {
    logger.log("parseVariable: exception extracting type - " +
               std::string(e.what()));
  } catch (...) {
    logger.log("parseVariable: unknown exception extracting type");
  }  logger.log("parseVariable: successfully parsed '" + sym->name + "' type='" +
             (sym->type.empty() ? "<inferred>" : sym->type) + "'");

  return sym;
}
std::vector<SymbolPtr>
SemanticAnalyzer::getCallableSymbols(const std::string &uri) {
  std::vector<SymbolPtr> result;

  auto it = fileSymbols.find(uri);
  if (it != fileSymbols.end()) {
    for (const auto &sym : it->second) {
      if (sym->isCallable) {
        result.push_back(sym);
      }
    }
  }

  return result;
}

std::vector<SymbolPtr>
SemanticAnalyzer::getVariablesInScope(const std::string &uri, Position pos) {
  std::vector<SymbolPtr> result;

  auto it = fileSymbols.find(uri);
  if (it != fileSymbols.end()) {
    for (const auto &sym : it->second) {
      if (sym->kind == SymbolKind::Variable ||
          sym->kind == SymbolKind::Parameter) {
        if (sym->definition.range.start.line <= pos.line) {
          result.push_back(sym);
        }
      }
    }
  }

  return result;
}

SymbolPtr SemanticAnalyzer::getSymbolAt(const std::string &uri, Position pos) {
  auto it = fileSymbols.find(uri);
  if (it == fileSymbols.end())
    return nullptr;

  for (const auto &sym : it->second) {
    if (sym->definition.uri == uri) {
      auto &range = sym->definition.range;
      if (range.start.line == pos.line &&
          range.start.character <= pos.character &&
          pos.character <= range.end.character) {
        return sym;
      }
    }

    for (const auto &ref : sym->references) {
      if (ref.uri == uri) {
        auto &range = ref.range;
        if (range.start.line == pos.line &&
            range.start.character <= pos.character &&
            pos.character <= range.end.character) {
          return sym;
        }
      }
    }
  }

  return nullptr;
}

std::vector<SymbolPtr>
SemanticAnalyzer::getAllSymbolsInFile(const std::string &uri) {
  auto it = fileSymbols.find(uri);
  if (it != fileSymbols.end()) {
    return it->second;
  }
  return {};
}

void SemanticAnalyzer::parseImport(const std::string &line, Scope *scope) {
  size_t usingPos = line.find("using ");
  if (usingPos == std::string::npos)
    return;

  size_t start = usingPos + 6;
  size_t end = line.find(';', start);
  if (end == std::string::npos)
    return;

  std::string importPath = line.substr(start, end - start);
  importPath.erase(0, importPath.find_first_not_of(" \t"));
  importPath.erase(importPath.find_last_not_of(" \t") + 1);

  size_t doubleColonPos = importPath.find("::");
  if (doubleColonPos != std::string::npos) {
    importPath.replace(doubleColonPos, 2, ".");
  }

  ImportedModule import;
  import.fullPath = importPath;

  // Map standard library modules to their symbols
  if (importPath == "Std.IO") {
    import.importedSymbols = {"print",     "println",   "eprint",   "eprintln",
                              "readLine",  "read",      "readChar", "readFile",
                              "writeFile", "appendFile"};
  } else if (importPath == "Std.Network") {
    import.importedSymbols = {
        "HttpServer",       "HttpRequest",  "HttpResponse",
        "jsonResponse",     "htmlResponse", "textResponse",
        "redirectResponse", "urlEncode",    "urlDecode",
        "parseQuery",       "ping",         "getLocalIP",
        "httpGet",          "Status",       "serveFile"};
  } else if (importPath == "Std.Math") {
    import.importedSymbols = {
        "sqrt",  "sin", "cos", "tan",   "asin",  "acos", "atan",  "atan2",
        "abs",   "pow", "exp", "log",   "log10", "log2", "floor", "ceil",
        "round", "min", "max", "clamp", "PI",    "E"};
  } else if (importPath == "Std.String") {
    import.importedSymbols = {"length",   "isEmpty",    "trim",     "toLower",
                              "toUpper",  "startsWith", "endsWith", "contains",
                              "replace",  "split",      "join",     "repeat",
                              "substring"};
  } else if (importPath == "Std.Array") {
    import.importedSymbols = {"length", "isEmpty",  "push",
                              "pop",    "contains", "reverse",
                              "sort",   "indexOf",  "clear"};
  } else if (importPath == "Std.Parse") {
    import.importedSymbols = {"parseInt", "parseFloat", "parseBool"};
  } else if (importPath == "Std.Option") {
    import.importedSymbols = {"isSome", "isNone", "unwrap", "unwrapOr"};
  } else if (importPath == "Std.Map") {
    import.importedSymbols = {"create",   "insert", "get",   "getOr",
                              "contains", "remove", "size",  "isEmpty",
                              "clear",    "keys",   "values"};
  } else if (importPath == "Std.Set") {
    import.importedSymbols = {"create", "insert",       "contains",  "remove",
                              "size",   "isEmpty",      "clear",     "toArray",
                              "union_", "intersection", "difference"};
  } else if (importPath == "Std.File") {
    import.importedSymbols = {// Path / FS utilities
                              "exists", "isFile", "isDirectory", "createDir",
                              "remove", "removeAll", "copy", "rename", "size",

                              // File I/O functions
                              "readFile", "writeFile", "appendFile",

                              // Low-level file operations
                              "Handle", "Mode", "Seek", "open", "close", "read",
                              "write", "read_bytes", "write_u32", "write_u64",
                              "seek", "tell", "flush"};
  } else if (importPath == "Std.Time") {
    import.importedSymbols = {"now", "sleep", "timestamp"};
  } else if (importPath == "Std.Random") {
    import.importedSymbols = {"randInt", "randFloat", "randBool"};
  } else if (importPath == "Std.System") {
    import.importedSymbols = {"exit", "getEnv", "execute"};
  } else {
    // User module - search our cached symbols
    std::string modulePath = importPath;

    // Strip project name prefix (e.g., "MagolorDotDev.models.Package" ->
    // "models/Package")
    size_t firstDot = modulePath.find('.');
    if (firstDot != std::string::npos) {
      std::string possibleProjectName = modulePath.substr(0, firstDot);
      if (possibleProjectName != "Std") {
        modulePath = modulePath.substr(firstDot + 1);
      }
    }

    // Convert dots to slashes for path matching
    std::string pathPattern = modulePath;
    std::replace(pathPattern.begin(), pathPattern.end(), '.', '/');

    // Search cached files for matching path
    for (const auto &[uri, symbols] : fileSymbols) {
      // Check if this file matches the import path
      if (uri.find(pathPattern + ".mg") != std::string::npos ||
          uri.find(pathPattern) != std::string::npos) {
        // Found the module - add all public/function symbols
        for (const auto &sym : symbols) {
          if (sym->isPublic || sym->kind == SymbolKind::Function ||
              sym->kind == SymbolKind::Class) {
            import.importedSymbols.push_back(sym->name);
          }
        }
        break;
      }
    }
  }

  scope->imports.push_back(import);
}

std::vector<std::string>
SemanticAnalyzer::getImportedModules(const std::string &uri) {
  std::vector<std::string> modules;

  auto it = fileScopes.find(uri);
  if (it != fileScopes.end()) {
    Scope *scope = it->second.get();
    while (scope) {
      for (const auto &import : scope->imports) {
        modules.push_back(import.fullPath);
      }
      scope = scope->parent;
    }
  }

  return modules;
}
