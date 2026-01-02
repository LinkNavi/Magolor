// src/lsp_completion.cpp
#include "lsp_completion.hpp"
#include "stdlib_parser.hpp"
#include <algorithm>
#include <set>
#include "lsp_project.hpp"
// Cache the parsed stdlib functions (parse once at startup)
static std::vector<StdLibFunction> g_stdlibFunctions;
static bool g_stdlibParsed = false;

std::vector<CompletionSnippet> CompletionProvider::getBuiltinSnippets() {
  return {
      {"fn", "fn ${1:name}(${2:params}) -> ${3:void} {\n\t${0}\n}",
       "Function declaration",
       "Create a new function with parameters and return type"},
      {"fnr", "fn ${1:name}(${2:params}) -> ${3:int} {\n\treturn ${0:0};\n}",
       "Function with return", "Create a function that returns a value"},
      {"main", "fn main() {\n\t${0}\n}", "Main function",
       "Entry point of the program"},
      {"class",
       "class ${1:Name} {\n\tpub ${2:field}: ${3:int};\n\t\n\tpub fn "
       "${4:method}() {\n\t\t${0}\n\t}\n}",
       "Class definition", "Create a class with fields and methods"},
      {"if", "if (${1:condition}) {\n\t${0}\n}", "If statement",
       "Conditional execution"},
      {"ife", "if (${1:condition}) {\n\t${2}\n} else {\n\t${0}\n}",
       "If-else statement", "Conditional with alternative"},
      {"while", "while (${1:condition}) {\n\t${0}\n}", "While loop",
       "Loop while condition is true"},
      {"for", "for (${1:item} in ${2:array}) {\n\t${0}\n}", "For loop",
       "Iterate over collection"},
      {"match",
       "match ${1:value} {\n\tSome(${2:v}) => {\n\t\t${3}\n\t},\n\tNone => "
       "{\n\t\t${0}\n\t}\n}",
       "Match expression", "Pattern matching for Option types"},
      {"let", "let ${1:mut }${2:name} = ${0:value};", "Variable declaration",
       "Declare a variable (optionally mutable)"},
      {"lett", "let ${1:mut }${2:name}: ${3:type} = ${0:value};",
       "Variable with type", "Declare a typed variable"},
      {"using", "using ${1:Std.IO};", "Import statement", "Import a module"},
      {"cimport", "cimport <${1:header.h}>${2: as ${3:Name}};", "C/C++ import",
       "Import C/C++ header"},
      {"cpp", "@cpp {\n\t${0}\n}", "C++ block", "Inline C++ code"},
      {"pubfn", "pub fn ${1:name}(${2:params}) -> ${3:void} {\n\t${0}\n}",
       "Public function", "Public function declaration"},
      {"staticfn",
       "pub static fn ${1:name}(${2:params}) -> ${3:void} {\n\t${0}\n}",
       "Static function", "Static function declaration"},
      {"lambda", "fn(${1:x}: ${2:int}) -> ${3:int} {\n\treturn ${0:x};\n}",
       "Lambda function", "Anonymous function/closure"},
      {"ret", "return ${0:value};", "Return statement", "Return from function"},
      {"new", "let ${1:var} = new ${2:Class}();", "New instance",
       "Create class instance"},
  };
}

std::vector<std::string> CompletionProvider::getKeywords() {
  return {"fn",   "let",   "mut",    "return", "if",   "else",   "while",
          "for",  "match", "class",  "new",    "this", "true",   "false",
          "None", "Some",  "using",  "pub",    "priv", "static", "cimport",
          "int",  "float", "string", "bool",   "void"};
}

bool CompletionProvider::matchesFilter(const std::string &name,
                                       const std::string &filter) {
  if (filter.empty())
    return true;

  if (name.size() < filter.size())
    return false;

  for (size_t i = 0; i < filter.size(); i++) {
    if (std::tolower(name[i]) != std::tolower(filter[i])) {
      return false;
    }
  }
  return true;
}
void CompletionProvider::addImportedSymbols(JsonValue& items, const std::string& uri, const std::string& filter) {
    // Get symbols from all imported modules
    auto importedSymbols = analyzer.resolveImportedSymbols(uri);
    
    for (const auto& sym : importedSymbols) {
        if (!matchesFilter(sym->name, filter)) {
            continue;
        }
        
        JsonValue item = JsonValue::object();
        item["label"] = sym->name;
        
        if (sym->kind == SymbolKind::Function) {
            item["kind"] = (int)CompletionItemKind::Function;
        } else if (sym->kind == SymbolKind::Class) {
            item["kind"] = (int)CompletionItemKind::Class;
        } else {
            item["kind"] = (int)CompletionItemKind::Variable;
        }
        
        if (!sym->detail.empty()) {
            item["detail"] = sym->detail;
        }
        
        item["sortText"] = "1_" + sym->name;
        items.push(item);
    }
}

void CompletionProvider::addModuleCompletions(JsonValue& items, const std::string& uri) {
    // Get current project
    auto project = ProjectManager::instance().getProjectForFile(uri);
    if (!project) {
        return;
    }
    
    // Get all available modules in the project
    for (const auto& [moduleUri, module] : project->modules) {
        if (moduleUri == uri) continue; // Skip self
        
        JsonValue item = JsonValue::object();
        item["label"] = module->name;
        item["kind"] = (int)CompletionItemKind::Module;
        item["detail"] = "Module";
        item["insertText"] = module->name;
        item["sortText"] = "0_" + module->name;
        items.push(item);
    }
}
void CompletionProvider::addCallableSymbols(JsonValue &items,
                                            const std::string &uri,
                                            const std::string &filter) {
  auto callables = analyzer.getCallableSymbols(uri);

  for (const auto &sym : callables) {
    if (!matchesFilter(sym->name, filter))
      continue;

    JsonValue item = JsonValue::object();
    item["label"] = sym->name;
    item["kind"] =
        (int)(sym->kind == SymbolKind::Method ? CompletionItemKind::Method
                                              : CompletionItemKind::Function);

    if (!sym->detail.empty()) {
      item["detail"] = sym->name + sym->detail;
    } else {
      item["detail"] = sym->name + "()";
    }

    if (!sym->documentation.empty()) {
      item["documentation"] = sym->documentation;
    }

    item["sortText"] = "1_" + sym->name;
    items.push(item);
  }
}

void CompletionProvider::addVariableSymbols(JsonValue &items,
                                            const std::string &uri,
                                            Position pos,
                                            const std::string &filter) {
  auto variables = analyzer.getVariablesInScope(uri, pos);

  for (const auto &sym : variables) {
    if (!matchesFilter(sym->name, filter))
      continue;

    JsonValue item = JsonValue::object();
    item["label"] = sym->name;
    item["kind"] = (int)CompletionItemKind::Variable;

    if (!sym->type.empty()) {
      item["detail"] = sym->type;
    }

    item["sortText"] = "1_" + sym->name;
    items.push(item);
  }
}

void CompletionProvider::addImportedFunctions(JsonValue &items,
                                              const std::string &uri,
                                              const std::string &filter) {
  if (!g_stdlibParsed) {
    g_stdlibFunctions = StdLibParser::parseStdLib();
    g_stdlibParsed = true;
  }

  auto importedModules = analyzer.getImportedModules(uri);

  for (const auto &modulePath : importedModules) {
    std::string module;
    std::string submodule;

    if (modulePath.find("Std.") == 0) {
      size_t moduleStart = 4;
      size_t dotPos = modulePath.find('.', moduleStart);

      if (dotPos != std::string::npos) {
        module = modulePath.substr(moduleStart, dotPos - moduleStart);
        submodule = modulePath.substr(dotPos + 1);
      } else {
        module = modulePath.substr(moduleStart);
      }
    }

    for (const auto &func : g_stdlibFunctions) {
      if (func.module != module)
        continue;
      if (!submodule.empty() && func.submodule != submodule)
        continue;
      if (!matchesFilter(func.name, filter))
        continue;

      JsonValue item = JsonValue::object();
      item["label"] = func.name;

      if (func.isConstant) {
        item["kind"] = (int)CompletionItemKind::Constant;
      } else {
        item["kind"] = (int)CompletionItemKind::Function;
      }

      item["detail"] = func.signature;
      item["documentation"] = "From " + modulePath;
      item["sortText"] = "0_" + func.name;
      items.push(item);
    }
  }
}

void CompletionProvider::addStdLibCompletions(JsonValue &items,
                                              const std::string &context) {
  if (!g_stdlibParsed) {
    g_stdlibFunctions = StdLibParser::parseStdLib();
    g_stdlibParsed = true;
  }

  std::string currentModule;
  std::string currentSubmodule;

  if (context.find("Std.") != std::string::npos) {
    size_t stdPos = context.rfind("Std.");
    if (stdPos != std::string::npos) {
      size_t moduleStart = stdPos + 4;
      size_t moduleEnd = context.find('.', moduleStart);
      if (moduleEnd == std::string::npos) {
        moduleEnd = context.find(':', moduleStart);
      }
      if (moduleEnd == std::string::npos) {
        moduleEnd = context.length();
      }

      currentModule = context.substr(moduleStart, moduleEnd - moduleStart);

      if (moduleEnd < context.length() &&
          (context[moduleEnd] == '.' || context[moduleEnd] == ':')) {
        size_t subStart = moduleEnd + (context[moduleEnd] == ':' ? 2 : 1);
        size_t subEnd = context.find_first_of(".:( ", subStart);
        if (subEnd == std::string::npos) {
          subEnd = context.length();
        }
        currentSubmodule = context.substr(subStart, subEnd - subStart);
      }
    }
  }

  if (currentModule.empty() && context.find("Std.") != std::string::npos) {
    std::set<std::string> modules;
    for (const auto &func : g_stdlibFunctions) {
      modules.insert(func.module);
    }

    for (const auto &mod : modules) {
      JsonValue item = JsonValue::object();
      item["label"] = mod;
      item["kind"] = (int)CompletionItemKind::Module;
      item["detail"] = "Std." + mod;
      item["sortText"] = "1_" + mod;
      items.push(item);
    }
    return;
  }

  if (!currentModule.empty() && currentSubmodule.empty() &&
      (context.back() == '.' || context.back() == ':')) {
    std::set<std::string> submodules;
    for (const auto &func : g_stdlibFunctions) {
      if (func.module == currentModule && !func.submodule.empty()) {
        submodules.insert(func.submodule);
      }
    }

    for (const auto &sub : submodules) {
      JsonValue item = JsonValue::object();
      item["label"] = sub;
      item["kind"] = (int)CompletionItemKind::Module;
      item["detail"] = "Std." + currentModule + "." + sub;
      item["sortText"] = "0_" + sub;
      items.push(item);
    }
  }

  for (const auto &func : g_stdlibFunctions) {
    bool matches = false;

    if (!currentSubmodule.empty()) {
      matches =
          (func.module == currentModule && func.submodule == currentSubmodule);
    } else if (!currentModule.empty()) {
      matches = (func.module == currentModule && func.submodule.empty());
    }

    if (matches) {
      JsonValue item = JsonValue::object();
      item["label"] = func.name;

      if (func.isConstant) {
        item["kind"] = (int)CompletionItemKind::Constant;
      } else {
        item["kind"] = (int)CompletionItemKind::Function;
      }

      item["detail"] = func.signature;

      std::string fullPath = "Std." + func.module;
      if (!func.submodule.empty()) {
        fullPath += "." + func.submodule;
      }
      item["documentation"] = "From " + fullPath;

      item["sortText"] = "1_" + func.name;
      items.push(item);
    }
  }
}

JsonValue CompletionProvider::provideCompletions(const std::string &uri,
                                                 Position pos,
                                                 const std::string &lineText) {
  JsonValue items = JsonValue::array();

  std::string word;
  int charPos = pos.character - 1;
  while (charPos >= 0 &&
         (std::isalnum(lineText[charPos]) || lineText[charPos] == '_')) {
    word = lineText[charPos] + word;
    charPos--;
  }

  std::string context = lineText.substr(0, pos.character);
  addStdLibCompletions(items, context);
  addImportedFunctions(items, uri, word);
 addImportedSymbols(items, uri, word);
    
    // NEW: If we're in a using statement, add module completions
    if (lineText.find("using ") != std::string::npos) {
        addModuleCompletions(items, uri);
    }
    
  for (const auto &snippet : getBuiltinSnippets()) {
    if (matchesFilter(snippet.label, word)) {
      JsonValue item = JsonValue::object();
      item["label"] = snippet.label;
      item["kind"] = (int)CompletionItemKind::Snippet;
      item["insertText"] = snippet.insertText;
      item["insertTextFormat"] = 2;
      item["detail"] = snippet.detail;
      item["documentation"] = snippet.documentation;
      item["sortText"] = "2_" + snippet.label;
      items.push(item);
    }
  }

  addCallableSymbols(items, uri, word);
  addVariableSymbols(items, uri, pos, word);

  for (const auto &kw : getKeywords()) {
    if (matchesFilter(kw, word)) {
      JsonValue item = JsonValue::object();
      item["label"] = kw;
      item["kind"] = (int)CompletionItemKind::Keyword;
      item["sortText"] = "3_" + kw;
      items.push(item);
    }
  }

  return items;
}
