#include "lsp_semantic.hpp"
#include <algorithm>
#include <regex>
#include <sstream>

void SemanticAnalyzer::analyze(const std::string &uri,
                               const std::string &content) {
  extractSymbols(uri, content);
}

void SemanticAnalyzer::extractSymbols(const std::string &uri,
                                      const std::string &content) {
  std::vector<SymbolPtr> symbols;
  auto scope = std::make_shared<Scope>();

  std::istringstream stream(content);
  std::string line;
  int lineNum = 0;
  std::string currentClass;
  Scope *currentScope = scope.get();

  while (std::getline(stream, line)) {
    // Trim whitespace
    line.erase(0, line.find_first_not_of(" \t"));

    // Parse imports FIRST
    if (line.find("using ") != std::string::npos) {
      parseImport(line, currentScope);
    }
    // Parse cimports - handle both <header> and "header" syntax
    else if (line.find("cimport ") != std::string::npos) {
      // Just skip cimport lines - they're handled by the compiler
      // Don't try to extract symbols from them in the LSP
      lineNum++;
      continue;
    }
    // Class detection
    else if (line.find("class ") != std::string::npos) {
      auto sym = parseClass(line, lineNum, uri);
      if (sym) {
        sym->isCallable = false;
        symbols.push_back(sym);
        currentScope->define(sym);
        currentClass = sym->name;

        auto classScope = std::make_shared<Scope>();
        classScope->parent = currentScope;
        currentScope = classScope.get();
      }
    }
    // Function/Method detection
    else if (line.find("fn ") != std::string::npos) {
      auto sym = parseFunction(line, lineNum, uri);
      if (sym) {
        sym->containerName = currentClass;
        sym->isCallable = true;
        sym->kind =
            currentClass.empty() ? SymbolKind::Function : SymbolKind::Method;

        // Extract parameter types and return type
        size_t parenStart = line.find('(');
        size_t parenEnd = line.find(')');
        if (parenStart != std::string::npos && parenEnd != std::string::npos) {
          std::string params =
              line.substr(parenStart + 1, parenEnd - parenStart - 1);

          std::istringstream paramStream(params);
          std::string param;
          while (std::getline(paramStream, param, ',')) {
            size_t colonPos = param.find(':');
            if (colonPos != std::string::npos) {
              std::string type = param.substr(colonPos + 1);
              type.erase(0, type.find_first_not_of(" \t"));
              type.erase(type.find_last_not_of(" \t") + 1);
              sym->paramTypes.push_back(type);
            }
          }

          size_t arrowPos = line.find("->", parenEnd);
          if (arrowPos != std::string::npos) {
            size_t bracePos = line.find('{', arrowPos);
            std::string retType =
                line.substr(arrowPos + 2, bracePos - arrowPos - 2);
            retType.erase(0, retType.find_first_not_of(" \t"));
            retType.erase(retType.find_last_not_of(" \t") + 1);
            sym->returnType = retType;
          } else {
            sym->returnType = "void";
          }

          sym->detail = "(";
          for (size_t i = 0; i < sym->paramTypes.size(); i++) {
            if (i > 0)
              sym->detail += ", ";
            sym->detail += sym->paramTypes[i];
          }
          sym->detail += ") -> " + sym->returnType;
        }

        symbols.push_back(sym);
        currentScope->define(sym);
      }
    }
    // Variable detection
    else if (line.find("let ") != std::string::npos) {
      auto sym = parseVariable(line, lineNum, uri);
      if (sym) {
        sym->containerName = currentClass;
        sym->isCallable = false;
        
        // Check if the variable is assigned from a function call that creates an object
        // For example: let server = Std.Network.HttpServer(8080);
        size_t equalPos = line.find('=');
        if (equalPos != std::string::npos) {
          std::string rhs = line.substr(equalPos + 1);
          rhs.erase(0, rhs.find_first_not_of(" \t"));
          
          // Check if it's a constructor call (contains '(' and looks like a function call)
          if (rhs.find('(') != std::string::npos) {
            // Extract the type name before the '('
            size_t parenPos = rhs.find('(');
            std::string typeName = rhs.substr(0, parenPos);
            typeName.erase(typeName.find_last_not_of(" \t") + 1);
            
            // If it contains dots, take the last part as the type name
            size_t lastDot = typeName.rfind('.');
            if (lastDot != std::string::npos) {
              typeName = typeName.substr(lastDot + 1);
            }
            
            // Set the variable's type
            if (!typeName.empty()) {
              sym->type = typeName;
            }
          }
        }
        
        symbols.push_back(sym);
        currentScope->define(sym);
      }
    }

    if (line.find('}') != std::string::npos && !currentClass.empty()) {
      bool inString = false;
      int braceCount = 0;
      for (char c : line) {
        if (c == '"')
          inString = !inString;
        if (!inString) {
          if (c == '{')
            braceCount++;
          else if (c == '}')
            braceCount--;
        }
      }

      if (braceCount < 0) {
        currentClass = "";
        if (currentScope && currentScope->parent) {
          currentScope = currentScope->parent;
        }
      }
    }

    lineNum++;
  }

  fileSymbols[uri] = symbols;
  fileScopes[uri] = scope;
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
  if (line.substr(nameStart, 4) == "mut ")
    nameStart += 4;

  size_t nameEnd = line.find_first_of(":=", nameStart);
  if (nameEnd == std::string::npos)
    return nullptr;

  std::string name = line.substr(nameStart, nameEnd - nameStart);
  name.erase(0, name.find_first_not_of(" \t"));
  name.erase(name.find_last_not_of(" \t") + 1);

  auto sym = std::make_shared<Symbol>();
  sym->name = name;
  sym->kind = SymbolKind::Variable;
  sym->definition.uri = uri;
  sym->definition.range.start = {lineNum, (int)nameStart};
  sym->definition.range.end = {lineNum, (int)nameEnd};

  size_t colonPos = line.find(':', nameEnd);
  if (colonPos != std::string::npos) {
    size_t typeEnd = line.find('=', colonPos);
    if (typeEnd == std::string::npos)
      typeEnd = line.size();
    sym->type = line.substr(colonPos + 1, typeEnd - colonPos - 1);
    sym->type.erase(0, sym->type.find_first_not_of(" \t"));
    sym->type.erase(sym->type.find_last_not_of(" \t") + 1);
  }

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
        "httpGet",          "Status"};
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
    import.importedSymbols = {"exists",    "isFile", "isDirectory",
                              "createDir", "remove", "removeAll",
                              "copy",      "rename", "size"};
  } else if (importPath == "Std.Time") {
    import.importedSymbols = {"now", "sleep", "timestamp"};
  } else if (importPath == "Std.Random") {
    import.importedSymbols = {"randInt", "randFloat", "randBool"};
  } else if (importPath == "Std.System") {
    import.importedSymbols = {"exit", "getEnv", "execute"};
  }

  if (!import.importedSymbols.empty()) {
    scope->imports.push_back(import);
  }
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
