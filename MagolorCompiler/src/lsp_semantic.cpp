#include "lsp_semantic.hpp"
#include "lsp_project.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>

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
  // Load entire project on first analyze
  loadProject(uri);

  // Then analyze this specific file (updates cache)
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

        // Check if the variable is assigned from a function call
        size_t equalPos = line.find('=');
        if (equalPos != std::string::npos) {
          std::string rhs = line.substr(equalPos + 1);
          rhs.erase(0, rhs.find_first_not_of(" \t"));

          if (rhs.find('(') != std::string::npos) {
            size_t parenPos = rhs.find('(');
            std::string typeName = rhs.substr(0, parenPos);
            typeName.erase(typeName.find_last_not_of(" \t") + 1);

            size_t lastDot = typeName.rfind('.');
            if (lastDot != std::string::npos) {
              typeName = typeName.substr(lastDot + 1);
            }

            if (!typeName.empty()) {
              sym->type = typeName;
            }
          }
        }

        symbols.push_back(sym);
        currentScope->define(sym);
      }
    }

    // Track class scope closing
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

std::vector<SymbolPtr>
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
    import.importedSymbols = {"print", "println", "eprint", "eprintln",
                              "readLine", "read", "readChar", "readFile",
                              "writeFile", "appendFile"};
} else if (importPath == "Std.Network") {
    import.importedSymbols = {"HttpServer", "HttpRequest", "HttpResponse",
                              "jsonResponse", "htmlResponse", "textResponse",
                              "redirectResponse", "urlEncode", "urlDecode",
                              "parseQuery", "ping", "getLocalIP",
                              "httpGet", "Status", "serveFile"};
} else if (importPath == "Std.Math") {
    import.importedSymbols = {"sqrt", "sin", "cos", "tan", "asin", "acos", "atan", "atan2",
                              "abs", "pow", "exp", "log", "log10", "log2", "floor", "ceil",
                              "round", "min", "max", "clamp", "PI", "E"};
} else if (importPath == "Std.String") {
    import.importedSymbols = {"length", "isEmpty", "trim", "toLower",
                              "toUpper", "startsWith", "endsWith", "contains",
                              "replace", "split", "join", "repeat", "substring"};
} else if (importPath == "Std.Array") {
    import.importedSymbols = {"length", "isEmpty", "push", "pop", "contains",
                              "reverse", "sort", "indexOf", "clear"};
} else if (importPath == "Std.Parse") {
    import.importedSymbols = {"parseInt", "parseFloat", "parseBool"};
} else if (importPath == "Std.Option") {
    import.importedSymbols = {"isSome", "isNone", "unwrap", "unwrapOr"};
} else if (importPath == "Std.Map") {
    import.importedSymbols = {"create", "insert", "get", "getOr",
                              "contains", "remove", "size", "isEmpty",
                              "clear", "keys", "values"};
} else if (importPath == "Std.Set") {
    import.importedSymbols = {"create", "insert", "contains", "remove",
                              "size", "isEmpty", "clear", "toArray",
                              "union_", "intersection", "difference"};
} else if (importPath == "Std.File") {
    import.importedSymbols = {
        // Path / FS utilities
        "exists", "isFile", "isDirectory", "createDir",
        "remove", "removeAll", "copy", "rename", "size",

        // File I/O (DB-ready)
        "Handle", "Mode", "Seek", "open", "close", "read",
        "write", "read_bytes", "write_u32", "write_u64",
        "seek", "tell", "flush"
    };
} else if (importPath == "Std.Time") {
    import.importedSymbols = {"now", "sleep", "timestamp"};
} else if (importPath == "Std.Random") {
    import.importedSymbols = {"randInt", "randFloat", "randBool"};
} else if (importPath == "Std.System") {
    import.importedSymbols = {"exit", "getEnv", "execute"};
} else {
    // User module - search our cached symbols
    std::string modulePath = importPath;

    // Strip project name prefix (e.g., "MagolorDotDev.models.Package" -> "models/Package")
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
