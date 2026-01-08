#include "codegen.hpp"
#include "stdlib.hpp"
#include "stdlib_loader.hpp"
#include <fstream>
#include <functional>
#include <regex>
#include <sstream>
#include <unordered_set>
#include <variant>

void CodeGen::emit(const std::string &s) { out << s; }
void CodeGen::emitLine(const std::string &s) {
  emitIndent();
  out << s << "\n";
}
void CodeGen::emitIndent() {
  for (int i = 0; i < indent; i++)
    out << "    ";
}

std::string CodeGen::typeToString(const TypePtr &type) {
  if (!type)
    return "auto";
  switch (type->kind) {
  case Type::INT:
    return "int64_t";
  case Type::FLOAT:
    return "double";
  case Type::STRING:
    return "std::string";
  case Type::BOOL:
    return "bool";
  case Type::VOID:
    return "void";
  case Type::CLASS:
    return type->className;
  case Type::OPTION:
    return "std::optional<" + typeToString(type->innerType) + ">";
  case Type::ARRAY:
    return "std::vector<" + typeToString(type->innerType) + ">";
  case Type::GENERIC: {
    if (type->className == "Array" && type->genericArgs.size() == 1) {
      return "std::vector<" + typeToString(type->genericArgs[0]) + ">";
    }
    if (type->className == "Map" && type->genericArgs.size() == 2) {
      return "std::unordered_map<" + typeToString(type->genericArgs[0]) + ", " +
             typeToString(type->genericArgs[1]) + ">";
    }
    if (type->className == "Set" && type->genericArgs.size() == 1) {
      return "std::unordered_set<" + typeToString(type->genericArgs[0]) + ">";
    }
    if (type->className == "Option" && type->genericArgs.size() == 1) {
      return "std::optional<" + typeToString(type->genericArgs[0]) + ">";
    }
    std::string result = type->className;
    if (!type->genericArgs.empty()) {
      result += "<";
      for (size_t i = 0; i < type->genericArgs.size(); i++) {
        if (i > 0)
          result += ", ";
        result += typeToString(type->genericArgs[i]);
      }
      result += ">";
    }
    return result;
  }
  case Type::FUNCTION: {
    std::string s = "std::function<" + typeToString(type->returnType) + "(";
    for (size_t i = 0; i < type->paramTypes.size(); i++) {
      if (i > 0)
        s += ", ";
      s += typeToString(type->paramTypes[i]);
    }
    return s + ")>";
  }
  }
  return "auto";
}

void CodeGen::genStdLib() { out << StdLibGenerator::generateAll(); }

void CodeGen::genCImports(const std::vector<CImportDecl> &cimports) {
  if (cimports.empty())
    return;

  out << "// ===================================================================\n";
  out << "// C/C++ Imports\n";
  out << "// ===================================================================\n";

  for (const auto &imp : cimports) {
    if (imp.isSystemHeader) {
      out << "#include <" << imp.header << ">\n";
    } else {
      out << "#include \"" << imp.header << "\"\n";
    }

    if (!imp.asNamespace.empty()) {
      out << "namespace " << imp.asNamespace << " {\n";
      if (!imp.symbols.empty()) {
        for (const auto &sym : imp.symbols) {
          out << "    using ::" << sym << ";\n";
        }
      }
      out << "}\n";
      importedNamespaces.insert(imp.asNamespace);
    } else if (!imp.symbols.empty()) {
      for (const auto &sym : imp.symbols) {
        out << "using ::" << sym << ";\n";
      }
    }
  }
  out << "\n";
}

bool CodeGen::isClassName(const std::string &name) const {
  return knownClassNames.count(name) > 0;
}

std::string CodeGen::extractStdLibCppCode(const std::string &modulePath) {
  auto &loader = StdLibLoader::instance();
  auto *module = loader.loadModule(modulePath);
  if (!module || module->filePath.empty()) {
    return "";
  }

  std::ifstream file(module->filePath);
  if (!file) {
    return "";
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string source = buffer.str();

  std::stringstream cppCode;
  std::regex cppBlockRegex(R"(@cpp\s*\{)");
  std::smatch match;

  std::string::const_iterator searchStart = source.cbegin();
  while (std::regex_search(searchStart, source.cend(), match, cppBlockRegex)) {
    size_t blockStart = match.position(0) + (searchStart - source.cbegin());
    size_t braceStart = source.find('{', blockStart);

    if (braceStart == std::string::npos) {
      searchStart = match.suffix().first;
      continue;
    }

    int depth = 1;
    size_t pos = braceStart + 1;
    while (pos < source.size() && depth > 0) {
      if (source[pos] == '"') {
        pos++;
        while (pos < source.size() && source[pos] != '"') {
          if (source[pos] == '\\')
            pos++;
          pos++;
        }
      } else if (source[pos] == '{') {
        depth++;
      } else if (source[pos] == '}') {
        depth--;
      }
      pos++;
    }

    if (depth == 0) {
      std::string code = source.substr(braceStart + 1, pos - braceStart - 2);
      size_t start = code.find_first_not_of(" \t\n\r");
      size_t end = code.find_last_not_of(" \t\n\r");
      if (start != std::string::npos && end != std::string::npos) {
        code = code.substr(start, end - start + 1);
      }
      cppCode << "    " << code << "\n\n";
    }

    searchStart = source.cbegin() + pos;
  }

  return cppCode.str();
}



std::string CodeGen::generateStdModuleImpl(const std::string &modulePath) {
  auto &loader = StdLibLoader::instance();
  auto *module = loader.loadModule(modulePath);
  if (!module || module->filePath.empty()) {
    return "";
  }

  std::stringstream out;

  std::ifstream file(module->filePath);
  if (!file) {
    return "";
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string source = buffer.str();

  std::string moduleName = modulePath;
  size_t lastDot = moduleName.rfind('.');
  if (lastDot != std::string::npos) {
    moduleName = moduleName.substr(lastDot + 1);
  }

  out << "// =======================================================================\n";
  out << "// " << modulePath << " (Auto-generated from stdlib)\n";
  out << "// =======================================================================\n";
  out << "namespace " << moduleName << " {\n\n";

  // =========================================================================
  // Extract classes WITH their methods inside
  // =========================================================================
  std::regex classRegex(R"(pub\s+class\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\{)");
  auto classBegin = std::sregex_iterator(source.begin(), source.end(), classRegex);
  auto classEnd = std::sregex_iterator();

  for (auto it = classBegin; it != classEnd; ++it) {
    std::string className = (*it)[1].str();
    size_t classStart = (*it).position();
    size_t braceStart = source.find('{', classStart);

    int depth = 1;
    size_t pos = braceStart + 1;
    while (pos < source.size() && depth > 0) {
      if (source[pos] == '{') depth++;
      else if (source[pos] == '}') depth--;
      pos++;
    }

    std::string classBody = source.substr(braceStart + 1, pos - braceStart - 2);
    
    // Use struct so all members are public by default
    out << "    struct " << className << " {\n";

    // Extract fields
    std::regex fieldRegex(R"(pub\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*:\s*([^;]+);)");
    auto fBegin = std::sregex_iterator(classBody.begin(), classBody.end(), fieldRegex);
    auto fEnd = std::sregex_iterator();

    for (auto fit = fBegin; fit != fEnd; ++fit) {
      std::string fieldName = (*fit)[1].str();
      std::string fieldType = (*fit)[2].str();
      
      while (!fieldType.empty() && std::isspace(fieldType.front())) fieldType.erase(0, 1);
      while (!fieldType.empty() && std::isspace(fieldType.back())) fieldType.pop_back();

      std::string cppType;
      std::string defaultVal;
      
      if (fieldType == "int") {
        cppType = "int64_t";
        defaultVal = " = 0";
      } else if (fieldType == "float") {
        cppType = "double";
        defaultVal = " = 0.0";
      } else if (fieldType == "string") {
        cppType = "std::string";
        defaultVal = "";
      } else if (fieldType == "bool") {
        cppType = "bool";
        defaultVal = " = false";
      } else if (fieldType.find("Array<") == 0) {
        size_t start = fieldType.find('<') + 1;
        size_t end = fieldType.rfind('>');
        std::string inner = fieldType.substr(start, end - start);
        if (inner == "int") inner = "int64_t";
        else if (inner == "float") inner = "double";
        else if (inner == "string") inner = "std::string";
        cppType = "std::vector<" + inner + ">";
      } else if (fieldType.find("Option<") == 0) {
        size_t start = fieldType.find('<') + 1;
        size_t end = fieldType.rfind('>');
        std::string inner = fieldType.substr(start, end - start);
        if (inner == "int") inner = "int64_t";
        else if (inner == "string") inner = "std::string";
        cppType = "std::optional<" + inner + ">";
      } else if (fieldType.find("Map<") == 0) {
        size_t start = fieldType.find('<') + 1;
        size_t end = fieldType.rfind('>');
        std::string inner = fieldType.substr(start, end - start);
        size_t comma = inner.find(',');
        if (comma != std::string::npos) {
          std::string key = inner.substr(0, comma);
          std::string val = inner.substr(comma + 1);
          while (!key.empty() && std::isspace(key.back())) key.pop_back();
          while (!val.empty() && std::isspace(val.front())) val.erase(0, 1);
          if (key == "string") key = "std::string";
          if (val == "string") val = "std::string";
          if (key == "int") key = "int64_t";
          if (val == "int") val = "int64_t";
          cppType = "std::unordered_map<" + key + ", " + val + ">";
        } else {
          cppType = fieldType;
        }
      } else {
        cppType = fieldType;
      }

      out << "        " << cppType << " " << fieldName << defaultVal << ";\n";
    }

    // Extract methods INSIDE the class
    std::regex methodRegex(
        R"(pub\s+(?:static\s+)?fn\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\(([^)]*)\)\s*(?:->\s*([^\{]+))?\s*\{)");
    auto mBegin = std::sregex_iterator(classBody.begin(), classBody.end(), methodRegex);
    auto mEnd = std::sregex_iterator();

    for (auto mit = mBegin; mit != mEnd; ++mit) {
      std::string methodName = (*mit)[1].str();
      std::string params = (*mit)[2].str();
      std::string returnType = (*mit)[3].matched ? (*mit)[3].str() : "void";

      // Trim return type
      size_t rtStart = returnType.find_first_not_of(" \t\n");
      if (rtStart != std::string::npos) {
        returnType = returnType.substr(rtStart);
        size_t rtEnd = returnType.find_last_not_of(" \t\n");
        returnType = returnType.substr(0, rtEnd + 1);
      }
      if (returnType.empty()) returnType = "void";

      // Convert return type
      if (returnType == "int") returnType = "int64_t";
      else if (returnType == "float") returnType = "double";
      else if (returnType == "string") returnType = "std::string";
      else if (returnType == "Self" || returnType == className) returnType = className + "&";

      // Find method body and @cpp block
      size_t methodStart = (*mit).position();
      size_t methodBodyStart = classBody.find('{', methodStart) + 1;
      
      int mdepth = 1;
      size_t mpos = methodBodyStart;
      while (mpos < classBody.size() && mdepth > 0) {
        if (classBody[mpos] == '{') mdepth++;
        else if (classBody[mpos] == '}') mdepth--;
        mpos++;
      }
      
      std::string methodBody = classBody.substr(methodBodyStart, mpos - methodBodyStart - 1);
      
      // Find @cpp block
      size_t cppPos = methodBody.find("@cpp");
      std::string cppImpl;
      
      if (cppPos != std::string::npos) {
        size_t cppBraceStart = methodBody.find('{', cppPos);
        if (cppBraceStart != std::string::npos) {
          int cdepth = 1;
          size_t cpos = cppBraceStart + 1;
          bool inStr = false;
          while (cpos < methodBody.size() && cdepth > 0) {
            if (methodBody[cpos] == '"' && (cpos == 0 || methodBody[cpos-1] != '\\')) {
              inStr = !inStr;
            } else if (!inStr) {
              if (methodBody[cpos] == '{') cdepth++;
              else if (methodBody[cpos] == '}') cdepth--;
            }
            cpos++;
          }
          
          if (cdepth == 0) {
            cppImpl = methodBody.substr(cppBraceStart + 1, cpos - cppBraceStart - 2);
            size_t s = cppImpl.find_first_not_of(" \t\n\r");
            size_t e = cppImpl.find_last_not_of(" \t\n\r");
            if (s != std::string::npos && e != std::string::npos) {
              cppImpl = cppImpl.substr(s, e - s + 1);
            }
          }
        }
      }

      // CRITICAL: Skip methods without @cpp implementation
      if (cppImpl.empty()) continue;

      // Parse method parameters
      std::vector<std::pair<std::string, std::string>> methodParams;
      if (!params.empty()) {
        std::vector<std::string> paramParts;
        int pdepth = 0;
        std::string current;
        for (char c : params) {
          if (c == '<') pdepth++;
          else if (c == '>') pdepth--;
          if (c == ',' && pdepth == 0) {
            paramParts.push_back(current);
            current.clear();
          } else {
            current += c;
          }
        }
        if (!current.empty()) paramParts.push_back(current);
        
        for (auto& param : paramParts) {
          size_t ps = param.find_first_not_of(" \t");
          if (ps != std::string::npos) param = param.substr(ps);
          
          size_t colon = param.find(':');
          if (colon != std::string::npos) {
            std::string pname = param.substr(0, colon);
            std::string ptype = param.substr(colon + 1);
            
            ps = pname.find_last_not_of(" \t");
            if (ps != std::string::npos) pname = pname.substr(0, ps + 1);
            ps = ptype.find_first_not_of(" \t");
            if (ps != std::string::npos) ptype = ptype.substr(ps);
            size_t pe = ptype.find_last_not_of(" \t");
            if (pe != std::string::npos) ptype = ptype.substr(0, pe + 1);
            
            // Convert types
            if (ptype == "int") ptype = "int64_t";
            else if (ptype == "float") ptype = "double";
            else if (ptype == "string") ptype = "const std::string&";
            else if (ptype == "bool") ptype = "bool";
            
            methodParams.push_back({pname, ptype});
          }
        }
      }

      out << "\n        inline " << returnType << " " << methodName << "(";
      for (size_t pi = 0; pi < methodParams.size(); pi++) {
        if (pi > 0) out << ", ";
        out << methodParams[pi].second << " " << methodParams[pi].first;
      }
      out << ") {\n";
      
      std::istringstream implStream(cppImpl);
      std::string line;
      while (std::getline(implStream, line)) {
        out << "            " << line << "\n";
      }
      
      out << "        }\n";
    }

    out << "    };\n\n";
  }

  // =========================================================================
  // Extract standalone functions (not class methods)
  // =========================================================================
  std::regex funcRegex(
      R"(pub\s+(?:static\s+)?fn\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\(([^)]*)\)\s*(?:->\s*([^\{]+))?\s*\{)");
  auto funcBegin = std::sregex_iterator(source.begin(), source.end(), funcRegex);
  auto funcEnd = std::sregex_iterator();

  // Track class method names to skip them as standalone functions
  std::unordered_set<std::string> classMethodNames;
  for (auto it = classBegin; it != classEnd; ++it) {
    size_t classStart = (*it).position();
    size_t braceStart = source.find('{', classStart);
    int depth = 1;
    size_t pos = braceStart + 1;
    while (pos < source.size() && depth > 0) {
      if (source[pos] == '{') depth++;
      else if (source[pos] == '}') depth--;
      pos++;
    }
    std::string classBody = source.substr(braceStart + 1, pos - braceStart - 2);
    
    std::regex methodRegex(R"(pub\s+(?:static\s+)?fn\s+([a-zA-Z_][a-zA-Z0-9_]*))");
    auto mBegin = std::sregex_iterator(classBody.begin(), classBody.end(), methodRegex);
    auto mEnd = std::sregex_iterator();
    for (auto mit = mBegin; mit != mEnd; ++mit) {
      classMethodNames.insert((*mit)[1].str());
    }
  }

  for (auto it = funcBegin; it != funcEnd; ++it) {
    std::string funcName = (*it)[1].str();
    
    // Skip if this is a class method
    if (classMethodNames.count(funcName) > 0) continue;
    
    std::string params = (*it)[2].str();
    std::string returnType = (*it)[3].matched ? (*it)[3].str() : "void";

    // Trim return type
    size_t start = returnType.find_first_not_of(" \t\n");
    if (start != std::string::npos) {
      returnType = returnType.substr(start);
      size_t end = returnType.find_last_not_of(" \t\n");
      returnType = returnType.substr(0, end + 1);
    }

    // Convert types with FULL int->int64_t support
    auto convertType = [](const std::string& type) -> std::string {
      std::string t = type;
      size_t s = t.find_first_not_of(" \t\n");
      size_t e = t.find_last_not_of(" \t\n");
      if (s != std::string::npos && e != std::string::npos) t = t.substr(s, e - s + 1);
      
      if (t == "int") return "int64_t";
      if (t == "float") return "double";
      if (t == "string") return "std::string";
      if (t == "bool") return "bool";
      if (t == "void") return "void";
      if (t == "any") return "auto";
      
      if (t.find("Array<") == 0) {
        size_t start = t.find('<') + 1;
        size_t end = t.rfind('>');
        if (end != std::string::npos) {
          std::string inner = t.substr(start, end - start);
          if (inner == "int") inner = "int64_t";
          else if (inner == "float") inner = "double";
          else if (inner == "string") inner = "std::string";
          return "std::vector<" + inner + ">";
        }
      }
      
      if (t.find("Option<") == 0) {
        size_t start = t.find('<') + 1;
        size_t end = t.rfind('>');
        if (end != std::string::npos) {
          std::string inner = t.substr(start, end - start);
          if (inner == "int") inner = "int64_t";
          else if (inner == "float") inner = "double";
          else if (inner == "string") inner = "std::string";
          return "std::optional<" + inner + ">";
        }
      }
      
      // FIX: Handle Map<K,V> with int->int64_t conversion
      if (t.find("Map<") == 0) {
        size_t start = t.find('<') + 1;
        size_t end = t.rfind('>');
        if (end != std::string::npos) {
          std::string inner = t.substr(start, end - start);
          size_t comma = inner.find(',');
          if (comma != std::string::npos) {
            std::string key = inner.substr(0, comma);
            std::string val = inner.substr(comma + 1);
            while (!key.empty() && (key.back() == ' ' || key.back() == '\t')) key.pop_back();
            while (!val.empty() && val.front() == ' ') val = val.substr(1);
            if (key == "string") key = "std::string";
            if (val == "string") val = "std::string";
            if (key == "int") key = "int64_t";      // CRITICAL FIX
            if (val == "int") val = "int64_t";      // CRITICAL FIX
            return "std::unordered_map<" + key + ", " + val + ">";
          }
        }
      }
      
      return t;
    };
    
    returnType = convertType(returnType);

    // Find @cpp block
    size_t funcStart = (*it).position();
    size_t funcBodyStart = source.find('{', funcStart) + 1;
    size_t cppPos = source.find("@cpp", funcBodyStart);
    std::string cppImpl;

    if (cppPos != std::string::npos) {
      size_t nextFunc = source.find("pub fn", funcStart + 10);
      size_t nextStaticFunc = source.find("pub static fn", funcStart + 10);
      size_t boundary = std::min(
          nextFunc != std::string::npos ? nextFunc : source.size(),
          nextStaticFunc != std::string::npos ? nextStaticFunc : source.size());

      if (cppPos < boundary) {
        size_t cppBraceStart = source.find('{', cppPos);
        if (cppBraceStart != std::string::npos) {
          int depth = 1;
          size_t pos = cppBraceStart + 1;
          bool inStr = false;
          while (pos < source.size() && depth > 0) {
            if (source[pos] == '"' && (pos == 0 || source[pos-1] != '\\')) {
              inStr = !inStr;
            } else if (!inStr) {
              if (source[pos] == '{') depth++;
              else if (source[pos] == '}') depth--;
            }
            pos++;
          }

          if (depth == 0) {
            cppImpl = source.substr(cppBraceStart + 1, pos - cppBraceStart - 2);
            size_t s = cppImpl.find_first_not_of(" \t\n\r");
            size_t e = cppImpl.find_last_not_of(" \t\n\r");
            if (s != std::string::npos && e != std::string::npos) {
              cppImpl = cppImpl.substr(s, e - s + 1);
            }
          }
        }
      }
    }

    // CRITICAL: Skip functions without @cpp implementation
    if (cppImpl.empty()) {
      continue;
    }

    // Parse parameters
    std::vector<std::pair<std::string, std::string>> paramList;
    if (!params.empty()) {
      std::vector<std::string> paramParts;
      int depth = 0;
      std::string current;
      for (char c : params) {
        if (c == '<') depth++;
        else if (c == '>') depth--;
        
        if (c == ',' && depth == 0) {
          paramParts.push_back(current);
          current.clear();
        } else {
          current += c;
        }
      }
      if (!current.empty()) paramParts.push_back(current);
      
      for (auto& param : paramParts) {
        size_t s = param.find_first_not_of(" \t");
        if (s != std::string::npos) param = param.substr(s);

        size_t colon = param.find(':');
        if (colon != std::string::npos) {
          std::string pname = param.substr(0, colon);
          std::string ptype = param.substr(colon + 1);

          s = pname.find_last_not_of(" \t");
          if (s != std::string::npos) pname = pname.substr(0, s + 1);

          s = ptype.find_first_not_of(" \t");
          if (s != std::string::npos) ptype = ptype.substr(s);
          size_t e = ptype.find_last_not_of(" \t");
          if (e != std::string::npos) ptype = ptype.substr(0, e + 1);

          // Convert parameter types with full Map support
          if (ptype == "int") ptype = "int64_t";
          else if (ptype == "float") ptype = "double";
          else if (ptype == "string") ptype = "const std::string&";
          else if (ptype == "bool") ptype = "bool";
          else if (ptype.find("Array<") == 0) {
            size_t start = ptype.find('<') + 1;
            size_t end = ptype.rfind('>');
            if (end != std::string::npos) {
              std::string inner = ptype.substr(start, end - start);
              if (inner == "int") inner = "int64_t";
              else if (inner == "float") inner = "double";
              else if (inner == "string") inner = "std::string";
              ptype = "const std::vector<" + inner + ">&";
            }
          }
          else if (ptype.find("Map<") == 0) {
            size_t start = ptype.find('<') + 1;
            size_t end = ptype.rfind('>');
            if (end != std::string::npos) {
              std::string inner = ptype.substr(start, end - start);
              size_t comma = inner.find(',');
              if (comma != std::string::npos) {
                std::string key = inner.substr(0, comma);
                std::string val = inner.substr(comma + 1);
                while (!key.empty() && (key.back() == ' ' || key.back() == '\t')) key.pop_back();
                while (!val.empty() && val.front() == ' ') val = val.substr(1);
                if (key == "string") key = "std::string";
                else if (key == "int") key = "int64_t";
                if (val == "string") val = "std::string";
                else if (val == "int") val = "int64_t";
                ptype = "const std::unordered_map<" + key + ", " + val + ">&";
              }
            }
          }

          paramList.push_back({pname, ptype});
        }
      }
    }

    out << "    inline " << returnType << " " << funcName << "(";
    for (size_t i = 0; i < paramList.size(); i++) {
      if (i > 0) out << ", ";
      out << paramList[i].second << " " << paramList[i].first;
    }
    out << ") {\n";

    std::istringstream implStream(cppImpl);
    std::string line;
    while (std::getline(implStream, line)) {
      out << "        " << line << "\n";
    }

    out << "    }\n\n";
  }

  out << "} // namespace " << moduleName << "\n\n";

  return out.str();
}

std::vector<std::string> CodeGen::collectLinkFlags(const Program& prog) {
  std::unordered_set<std::string> uniqueFlags;
  std::vector<std::string> result;
  
  // Collect from @link blocks
  for (const auto &linkDecl : prog.linkDecls) {
    for (const auto &flag : linkDecl.flags) {
      if (uniqueFlags.insert(flag).second) {
        result.push_back(flag);
      }
    }
  }
  
  // Collect from used modules
  std::unordered_set<std::string> usedModules;
  for (const auto &usingDecl : prog.usings) {
    std::string modulePath;
    for (size_t i = 0; i < usingDecl.path.size(); i++) {
      if (i > 0) modulePath += ".";
      modulePath += usingDecl.path[i];
    }
    if (modulePath.find("Std.") == 0) {
      usedModules.insert(modulePath);
    }
  }
  
  auto moduleFlags = StdLibLoader::instance().collectLinkFlags(usedModules);
  for (const auto& flag : moduleFlags) {
    if (uniqueFlags.insert(flag).second) {
      result.push_back(flag);
    }
  }
  
  return result;
}

std::string CodeGen::generate(const Program &prog) {
   out.str("");
  out.clear();
  importedNamespaces.clear();
  knownClassNames.clear();

  // Track generated namespaces to avoid duplicates
  std::unordered_set<std::string> generatedNamespaces;

  out << "// Auto-generated C++ code from Magolor\n";

  // @cpp_header blocks first
  if (!prog.cppHeaders.empty()) {
    out << "\n// User-provided C++ headers\n";
    for (const auto &header : prog.cppHeaders) {
      out << header.code;
      if (!header.code.empty() && header.code.back() != '\n') {
        out << "\n";
      }
    }
    out << "\n";
  }

  // NEW: @include blocks - output C++ includes
  if (!prog.includeDecls.empty()) {
    out << "// User-specified includes (@include blocks)\n";
    for (const auto &includeDecl : prog.includeDecls) {
      for (const auto &header : includeDecl.headers) {
        out << "#include " << header << "\n";
      }
    }
    out << "\n";
  }

  // Collect includes from modules
  std::unordered_set<std::string> allIncludes;
  for (const auto &usingDecl : prog.usings) {
    std::string modulePath;
    for (size_t i = 0; i < usingDecl.path.size(); i++) {
      if (i > 0) modulePath += ".";
      modulePath += usingDecl.path[i];
    }

    if (modulePath.find("Std.") == 0) {
      auto includes = StdLibLoader::instance().getCppIncludes(modulePath);
      for (const auto &inc : includes) {
        allIncludes.insert(inc);
      }
    }
  }

  if (!allIncludes.empty()) {
    out << "// Module-required includes\n";
    for (const auto &inc : allIncludes) {
      out << "#include " << inc << "\n";
    }
    out << "\n";
  }

  // Standard includes
  out << "#include <vector>\n";
  out << "#include <unordered_map>\n";
  out << "#include <unordered_set>\n";
  out << "#include <optional>\n";
  out << "#include <iostream>\n";
  out << "#include <string>\n";
  out << "#include <algorithm>\n";
  out << "#include <functional>\n";
  out << "#include <sstream>\n";
  out << "#include <fstream>\n";
  out << "#include <filesystem>\n";
  out << "#include <random>\n";
  out << "#include <chrono>\n";
  out << "#include <thread>\n";
  out << "#include <cmath>\n";
  out << "#include <stdexcept>\n";
  out << "#include <cstring>\n";
  out << "#include <unistd.h>\n";
  out << "\n";  // Track imported Std modules
  std::unordered_set<std::string> importedStdModules;
  for (const auto &usingDecl : prog.usings) {
    std::string modulePath;
    for (size_t i = 0; i < usingDecl.path.size(); i++) {
      if (i > 0) modulePath += ".";
      modulePath += usingDecl.path[i];
    }

    if (modulePath.find("Std.") == 0) {
      importedStdModules.insert(modulePath);
    }
  }

  // Generate stdlib implementations - with duplicate prevention
  if (!importedStdModules.empty()) {
    out << "// =======================================================================\n";
    out << "// Auto-generated Standard Library Implementations\n";
    out << "// =======================================================================\n\n";

    for (const auto &modulePath : importedStdModules) {
      std::string moduleName = modulePath;
      size_t lastDot = moduleName.rfind('.');
      if (lastDot != std::string::npos) {
        moduleName = moduleName.substr(lastDot + 1);
      }

      // Skip if already generated
      if (generatedNamespaces.count(moduleName) > 0) {
        continue;
      }
      generatedNamespaces.insert(moduleName);

      std::string moduleImpl = generateStdModuleImpl(modulePath);
      if (!moduleImpl.empty()) {
        out << moduleImpl;
      }
    }
  }

  // Collect class names
  for (const auto &cls : prog.classes) {
    knownClassNames.insert(cls.name);
  }

  // C/C++ imports
  genCImports(prog.cimports);

  // Stdlib helpers with guards
  out << "// =======================================================================\n";
  out << "// Standard Library Helpers (generated once)\n";
  out << "// =======================================================================\n";
  out << "#ifndef MAGOLOR_STDLIB_HELPERS_H\n";
  out << "#define MAGOLOR_STDLIB_HELPERS_H\n\n";

  out << "// Template helpers for string conversion\n";
  out << "template<typename T>\n";
  out << "inline std::string mg_to_string(const T& val) { \n";
  out << "    std::ostringstream oss; \n";
  out << "    oss << val; \n";
  out << "    return oss.str(); \n";
  out << "}\n\n";

  out << "template<>\n";
  out << "inline std::string mg_to_string(const bool& val) {\n";
  out << "    return val ? \"true\" : \"false\";\n";
  out << "}\n\n";

  out << "template<>\n";
  out << "inline std::string mg_to_string(const std::string& val) {\n";
  out << "    return val;\n";
  out << "}\n\n";

  out << "// Global Option helpers\n";
  out << "template<typename T>\n";
  out << "inline bool isSome(const std::optional<T>& opt) { return opt.has_value(); }\n\n";

  out << "template<typename T>\n";
  out << "inline bool isNone(const std::optional<T>& opt) { return !opt.has_value(); }\n\n";

  out << "template<typename T>\n";
  out << "inline T unwrap(const std::optional<T>& opt) {\n";
  out << "    if (!opt.has_value()) {\n";
  out << "        throw std::runtime_error(\"Called unwrap on None value\");\n";
  out << "    }\n";
  out << "    return opt.value();\n";
  out << "}\n\n";

  out << "template<typename T>\n";
  out << "inline T unwrapOr(const std::optional<T>& opt, const T& defaultValue) {\n";
  out << "    return opt.value_or(defaultValue);\n";
  out << "}\n\n";

  out << "#endif // MAGOLOR_STDLIB_HELPERS_H\n\n";

  // Array helpers - only if not already generated
  if (generatedNamespaces.count("Array") == 0) {
    out << "// Array helper wrappers\n";
    out << "template<typename T> int length(const ::std::vector<T>& arr) { return arr.size(); }\n";
    out << "template<typename T> void push(::std::vector<T>& arr, const T& val) { arr.push_back(val); }\n";
    out << "template<typename T> T pop(::std::vector<T>& arr) { auto v = arr.back(); arr.pop_back(); return v; }\n";
    out << "\n";
  }

  // Map helpers - only if not already generated
  if (generatedNamespaces.count("Map") == 0) {
    out << "// Map helper wrappers\n";
    out << "namespace Map {\n";
    out << "  template<typename K, typename V> ::std::unordered_map<K,V> create() { return {}; }\n";
    out << "  template<typename K, typename V> void insert(::std::unordered_map<K,V>& m, const K& k, const V& v) { m[k] = v; }\n";
    out << "  template<typename K, typename V> ::std::optional<V> get(const ::std::unordered_map<K,V>& m, const K& k) {\n";
    out << "    auto it = m.find(k); return it != m.end() ? ::std::optional<V>(it->second) : ::std::nullopt;\n";
    out << "  }\n";
    out << "  template<typename K, typename V> ::std::vector<V> values(const ::std::unordered_map<K,V>& m) {\n";
    out << "    ::std::vector<V> r; for(auto& p : m) r.push_back(p.second); return r;\n";
    out << "  }\n";
    out << "}\n\n";
  }

  // File helper - only if not already generated
  if (generatedNamespaces.count("File") == 0) {
    out << "// File helper\n";
    out << "namespace File {\n";
    out << "  inline bool exists(const ::std::string& path) {\n";
    out << "    return std::filesystem::exists(path);\n";
    out << "  }\n";
    out << "  inline bool isFile(const ::std::string& path) {\n";
    out << "    return std::filesystem::is_regular_file(path);\n";
    out << "  }\n";
    out << "  inline bool isDirectory(const ::std::string& path) {\n";
    out << "    return std::filesystem::is_directory(path);\n";
    out << "  }\n";
    out << "  inline std::string absolutePath(const ::std::string& path) {\n";
    out << "    return std::filesystem::absolute(path).string();\n";
    out << "  }\n";
    out << "  inline std::string parentDir(const ::std::string& path) {\n";
    out << "    return std::filesystem::path(path).parent_path().string();\n";
    out << "  }\n";
    out << "  inline std::string fileName(const ::std::string& path) {\n";
    out << "    return std::filesystem::path(path).filename().string();\n";
    out << "  }\n";
    out << "  inline std::string extension(const ::std::string& path) {\n";
    out << "    return std::filesystem::path(path).extension().string();\n";
    out << "  }\n";
    out << "  inline std::string tempDir() {\n";
    out << "    return std::filesystem::temp_directory_path().string();\n";
    out << "  }\n";
    out << "  inline std::optional<std::string> createTempFile(const ::std::string& prefix) {\n";
    out << "    std::string dir = std::filesystem::temp_directory_path().string();\n";
    out << "    std::string path = dir + \"/\" + prefix + \"_XXXXXX\";\n";
    out << "    std::vector<char> buf(path.begin(), path.end());\n";
    out << "    buf.push_back('\\0');\n";
    out << "    int fd = mkstemp(buf.data());\n";
    out << "    if (fd == -1) return std::nullopt;\n";
    out << "    close(fd);\n";
    out << "    return std::string(buf.data());\n";
    out << "  }\n";
    out << "  inline std::string cwd() {\n";
    out << "    return std::filesystem::current_path().string();\n";
    out << "  }\n";
    out << "}\n\n";
  }

  // Global I/O functions
  out << "// Global I/O functions\n";
  out << "inline void print(const std::string& s) { std::cout << s; }\n";
  out << "inline void println(const std::string& s) { std::cout << s << std::endl; }\n";
  out << "inline void println() { std::cout << std::endl; }\n";
  out << "inline void eprint(const std::string& s) { std::cerr << s; }\n";
  out << "inline void eprintln(const std::string& s) { std::cerr << s << std::endl; }\n";
  out << "inline std::string readLine() { std::string line; std::getline(std::cin, line); return line; }\n";
  out << "\n";
  out << "// Overloads for common types\n";
  out << "template<typename T>\n";
  out << "inline void print(const T& val) { std::cout << val; }\n";
  out << "template<typename T>\n";
  out << "inline void println(const T& val) { std::cout << val << std::endl; }\n";
  out << "\n";

  // Forward declarations for classes
  for (const auto &cls : prog.classes) {
    emitLine("class " + cls.name + ";");
  }
  emitLine("");

  // Generate classes
  for (const auto &cls : prog.classes) {
    genClass(cls);
  }

  // Forward declarations for functions
  for (const auto &fn : prog.functions) {
    if (fn.name != "main") {
      emit(typeToString(fn.returnType) + " " + fn.name + "(");
      for (size_t i = 0; i < fn.params.size(); i++) {
        if (i > 0) emit(", ");
        emit(typeToString(fn.params[i].type) + " " + fn.params[i].name);
      }
      emit(");\n");
    }
  }
  emitLine("");

  // Generate function definitions
  for (const auto &fn : prog.functions) {
    genFunction(fn);
    emitLine("");
  }

  return out.str();
}

void CodeGen::genFunction(const FnDecl &fn, const std::string &className) {
  currentClassName = className;

  std::string retType = typeToString(fn.returnType);
  if (fn.name == "main" && className.empty())
    emitLine("int main() {");
  else if (fn.name == "create" && !className.empty()) {
    emitIndent();
    emit("void create(");
    for (size_t i = 0; i < fn.params.size(); i++) {
      if (i > 0) emit(", ");
      emit(typeToString(fn.params[i].type) + " " + fn.params[i].name);
    }
    emit(") {\n");
  } else {
    emitIndent();
    if (fn.isStatic && !className.empty()) {
      emit("static ");
    }
    emit(retType + " " + fn.name + "(");
    for (size_t i = 0; i < fn.params.size(); i++) {
      if (i > 0) emit(", ");
      emit(typeToString(fn.params[i].type) + " " + fn.params[i].name);
    }
    emit(") {\n");
  }
  indent++;
  for (const auto &stmt : fn.body)
    genStmt(stmt);
  if (fn.name == "main" && className.empty())
    emitLine("return 0;");
  indent--;
  emitLine("}");

  currentClassName.clear();
}

void CodeGen::genClass(const ClassDecl &cls) {
  emitLine("class " + cls.name + " {");

  bool hasPublic = false;
  bool hasPrivate = false;

  for (const auto &f : cls.fields) {
    if (f.isPublic) hasPublic = true;
    else hasPrivate = true;
  }
  for (const auto &m : cls.methods) {
    if (m.isPublic) hasPublic = true;
    else hasPrivate = true;
  }

  if (hasPublic) {
    emitLine("public:");
    indent++;

    for (const auto &f : cls.fields) {
      if (f.isPublic && f.isStatic) {
        emitIndent();
        emit("static constexpr " + typeToString(f.type) + " " + f.name + " = ");
        if (f.initValue) {
          genExpr(f.initValue);
        }
        emit(";\n");
      }
    }

    for (const auto &f : cls.fields) {
      if (f.isPublic && !f.isStatic) {
        emitLine(typeToString(f.type) + " " + f.name + ";");
      }
    }

    for (const auto &m : cls.methods) {
      if (m.isPublic) {
        genFunction(m, cls.name);
      }
    }

    indent--;
  }

  if (hasPrivate) {
    emitLine("private:");
    indent++;

    for (const auto &f : cls.fields) {
      if (!f.isPublic) {
        emitLine(typeToString(f.type) + " " + f.name + ";");
      }
    }

    for (const auto &m : cls.methods) {
      if (!m.isPublic) {
        genFunction(m, cls.name);
      }
    }

    indent--;
  }

  emitLine("};");
  emitLine("");

  // Auto-generate operator<< for printing
  emitLine("// Auto-generated print support");
  emitLine("inline std::ostream& operator<<(std::ostream& os, const " + cls.name + "& obj) {");
  indent++;
  emitLine("os << \"" + cls.name + " { \";");

  bool first = true;
  for (const auto &f : cls.fields) {
    if (f.isPublic && !f.isStatic) {
      bool isSimpleType = false;
      if (f.type) {
        switch (f.type->kind) {
        case Type::INT:
        case Type::FLOAT:
        case Type::STRING:
        case Type::BOOL:
          isSimpleType = true;
          break;
        default:
          isSimpleType = false;
          break;
        }
      }

      if (isSimpleType) {
        if (!first) {
          emitLine("os << \", \";");
        }
        emitLine("os << \"" + f.name + ": \" << obj." + f.name + ";");
        first = false;
      }
    }
  }

  emitLine("os << \" }\";");
  emitLine("return os;");
  indent--;
  emitLine("}");
  emitLine("");
}

void CodeGen::collectCaptures(const std::vector<StmtPtr> &,
                              const std::vector<Param> &params) {
  capturedVars.clear();
  for (const auto &p : params)
    capturedVars.insert(p.name);
}

void CodeGen::genStmt(const StmtPtr &stmt) {
  std::visit(
      [this](auto &&s) {
        using T = std::decay_t<decltype(s)>;
        if constexpr (std::is_same_v<T, LetStmt>) {
          emitIndent();
          emit((s.type ? typeToString(s.type) : "auto") + " " + s.name + " = ");
          genExpr(s.init);
          emit(";\n");
        } else if constexpr (std::is_same_v<T, ReturnStmt>) {
          emitIndent();
          emit("return");
          if (s.value) {
            emit(" ");
            if (std::holds_alternative<ThisExpr>(s.value->data)) {
              if (!currentClassName.empty()) {
                emit("*this");
              } else {
                genExpr(s.value);
              }
            } else {
              genExpr(s.value);
            }
          }
          emit(";\n");
        } else if constexpr (std::is_same_v<T, ExprStmt>) {
          emitIndent();
          genExpr(s.expr);
          emit(";\n");
        } else if constexpr (std::is_same_v<T, IfStmt>) {
          emitIndent();
          emit("if (");
          genExpr(s.cond);
          emit(") {\n");
          indent++;
          for (const auto &st : s.thenBody) genStmt(st);
          indent--;
          emitLine("}");
          if (!s.elseBody.empty()) {
            emitLine("else {");
            indent++;
            for (const auto &st : s.elseBody) genStmt(st);
            indent--;
            emitLine("}");
          }
        } else if constexpr (std::is_same_v<T, WhileStmt>) {
          emitIndent();
          emit("while (");
          genExpr(s.cond);
          emit(") {\n");
          indent++;
          for (const auto &st : s.body) genStmt(st);
          indent--;
          emitLine("}");
        } else if constexpr (std::is_same_v<T, ForStmt>) {
          emitIndent();
          emit("for (auto& " + s.var + " : ");
          genExpr(s.iterable);
          emit(") {\n");
          indent++;
          for (const auto &st : s.body) genStmt(st);
          indent--;
          emitLine("}");
        } else if constexpr (std::is_same_v<T, MatchStmt>) {
          emitIndent();
          emit("{\n");
          indent++;
          emitIndent();
          emit("auto _match_val = ");
          genExpr(s.expr);
          emit(";\n");
          bool first = true;
          for (const auto &arm : s.arms) {
            emitIndent();
            if (!first) emit("else ");
            first = false;
            if (arm.pattern == "Some") {
              emit("if (_match_val.has_value()) {\n");
              indent++;
              if (!arm.bindVar.empty())
                emitLine("auto " + arm.bindVar + " = _match_val.value();");
            } else if (arm.pattern == "None") {
              emit("if (!_match_val.has_value()) {\n");
              indent++;
            } else {
              emit("if (_match_val == " + arm.pattern + ") {\n");
              indent++;
            }
            for (const auto &st : arm.body) genStmt(st);
            indent--;
            emitLine("}");
          }
          indent--;
          emitLine("}");
        } else if constexpr (std::is_same_v<T, BlockStmt>) {
          emitLine("{");
          indent++;
          for (const auto &st : s.stmts) genStmt(st);
          indent--;
          emitLine("}");
        } else if constexpr (std::is_same_v<T, CppStmt>) {
          emitLine("// Inline C++ code:");
          out << s.code;
          if (!s.code.empty() && s.code.back() != '\n') {
            out << "\n";
          }
        }
      },
      stmt->data);
}

void CodeGen::genExpr(const ExprPtr &expr) {
  std::visit(
      [this](auto &&e) {
        using T = std::decay_t<decltype(e)>;
        if constexpr (std::is_same_v<T, IntLitExpr>)
          emit(std::to_string(e.value));
        else if constexpr (std::is_same_v<T, FloatLitExpr>)
          emit(std::to_string(e.value));
        else if constexpr (std::is_same_v<T, StringLitExpr>) {
          if (e.interpolated) {
            emit("(");
            std::string s = e.value;
            std::string current;
            size_t i = 0;
            bool first = true;
            while (i < s.size()) {
              if (s[i] == '{') {
                if (!current.empty()) {
                  if (!first) emit(" + ");
                  first = false;
                  emit("std::string(\"");
                  for (char c : current) {
                    if (c == '\n') emit("\\n");
                    else if (c == '\\') emit("\\\\");
                    else if (c == '"') emit("\\\"");
                    else emit(std::string(1, c));
                  }
                  emit("\")");
                  current.clear();
                }
                i++;
                std::string varName;
                while (i < s.size() && s[i] != '}') varName += s[i++];
                i++;
                if (!first) emit(" + ");
                first = false;
                emit("mg_to_string(" + varName + ")");
              } else
                current += s[i++];
            }
            if (!current.empty()) {
              if (!first) emit(" + ");
              emit("std::string(\"");
              for (char c : current) {
                if (c == '\n') emit("\\n");
                else if (c == '\\') emit("\\\\");
                else if (c == '"') emit("\\\"");
                else emit(std::string(1, c));
              }
              emit("\")");
            }
            emit(")");
          } else {
            emit("std::string(\"");
            for (char c : e.value) {
              if (c == '\n') emit("\\n");
              else if (c == '\t') emit("\\t");
              else if (c == '\\') emit("\\\\");
              else if (c == '"') emit("\\\"");
              else emit(std::string(1, c));
            }
            emit("\")");
          }
        } else if constexpr (std::is_same_v<T, BoolLitExpr>)
          emit(e.value ? "true" : "false");
        else if constexpr (std::is_same_v<T, IdentExpr>)
          emit(e.name);
        else if constexpr (std::is_same_v<T, BinaryExpr>) {
          emit("(");
          genExpr(e.left);
          emit(" " + e.op + " ");
          genExpr(e.right);
          emit(")");
        } else if constexpr (std::is_same_v<T, UnaryExpr>) {
          emit("(" + e.op);
          genExpr(e.operand);
          emit(")");
        } else if constexpr (std::is_same_v<T, CallExpr>) {
          genExpr(e.callee);
          emit("(");
          for (size_t i = 0; i < e.args.size(); i++) {
            if (i > 0) emit(", ");
            genExpr(e.args[i]);
          }
          emit(")");
        } else if constexpr (std::is_same_v<T, MemberExpr>) {
          if (auto *objIdent = std::get_if<IdentExpr>(&e.object->data)) {
            static const std::unordered_set<std::string> stdModules = {
                "Crypto", "File", "String", "Array", "Map",
                "Math", "IO", "Parse", "Option", "Time",
                "Random", "System", "Network"};

            if (stdModules.count(objIdent->name) > 0) {
              emit(objIdent->name + "::" + e.member);
              return;
            }
          }

          bool isNamespacePath = false;
          ExprPtr root = e.object;
          while (auto *nested = std::get_if<MemberExpr>(&root->data)) {
            root = nested->object;
          }

          if (auto *rootIdent = std::get_if<IdentExpr>(&root->data)) {
            if (rootIdent->name == "Std" || rootIdent->name == "std") {
              isNamespacePath = true;
            } else if (importedNamespaces.count(rootIdent->name) > 0) {
              isNamespacePath = true;
            }
          }

          if (isNamespacePath) {
            std::vector<std::string> pathParts;
            ExprPtr current = e.object;

            while (auto *member = std::get_if<MemberExpr>(&current->data)) {
              pathParts.push_back(member->member);
              current = member->object;
            }

            if (auto *rootIdent = std::get_if<IdentExpr>(&current->data)) {
              if (rootIdent->name != "Std" && rootIdent->name != "std") {
                emit(rootIdent->name);
                emit("::");
              }
            }

            for (auto it = pathParts.rbegin(); it != pathParts.rend(); ++it) {
              emit(*it);
              emit("::");
            }

            emit(e.member);
            return;
          }

          if (auto *ident = std::get_if<IdentExpr>(&e.object->data)) {
            if (isClassName(ident->name)) {
              emit(ident->name + "::" + e.member);
              return;
            }
          }

          if (std::holds_alternative<ThisExpr>(e.object->data)) {
            emit("this->" + e.member);
            return;
          }

          genExpr(e.object);
          emit("." + e.member);
        } else if constexpr (std::is_same_v<T, IndexExpr>) {
          genExpr(e.object);
          emit("[");
          genExpr(e.index);
          emit("]");
        } else if constexpr (std::is_same_v<T, AssignExpr>) {
          genExpr(e.target);
          emit(" = ");
          genExpr(e.value);
        } else if constexpr (std::is_same_v<T, LambdaExpr>) {
          emit("[=](");
          for (size_t i = 0; i < e.params.size(); i++) {
            if (i > 0) emit(", ");
            emit(paramTypeToString(e.params[i].type) + " " + e.params[i].name);
          }
          emit(")");
          if (e.returnType) emit(" -> " + typeToString(e.returnType));
          emit(" {\n");
          indent++;
          for (const auto &st : e.body) genStmt(st);
          indent--;
          emitIndent();
          emit("}");
        } else if constexpr (std::is_same_v<T, NewExpr>) {
          emit(e.className + "(");
          for (size_t i = 0; i < e.args.size(); i++) {
            if (i > 0) emit(", ");
            genExpr(e.args[i]);
          }
          emit(")");
        } else if constexpr (std::is_same_v<T, SomeExpr>) {
          emit("std::make_optional(");
          genExpr(e.value);
          emit(")");
        } else if constexpr (std::is_same_v<T, NoneExpr>)
          emit("std::nullopt");
        else if constexpr (std::is_same_v<T, ThisExpr>)
          emit("this");
        else if constexpr (std::is_same_v<T, ArrayExpr>) {
          std::string elemType = "int";
          if (!e.elements.empty() && e.elements[0]->type) {
            elemType = typeToString(e.elements[0]->type);
          }

          emit("std::vector<" + elemType + ">{");
          for (size_t i = 0; i < e.elements.size(); i++) {
            if (i > 0) emit(", ");
            genExpr(e.elements[i]);
          }
          emit("}");
        }
      },
      expr->data);
}

std::string CodeGen::paramTypeToString(const TypePtr &type) {
  if (!type || type->kind == Type::VOID) {
    return "auto";
  }
  return typeToString(type);
}

void CodeGen::enterScope() {}
void CodeGen::exitScope() {}

void CodeGen::registerVar(const std::string &name, const std::string &type, bool isMut) {
  scopeVars[name] = {type, isMut};
}
