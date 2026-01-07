#include "stdlib_loader.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>

// ============================================================================
// StdLibLoader Implementation
// ============================================================================

void StdLibLoader::init(const std::string &path) {
  if (initialized)
    return;

  // Try multiple possible locations
  std::vector<std::string> searchPaths = {path,
                                          "./stdlib",
                                          "../stdlib",
                                          "../../stdlib",
                                          "/usr/local/share/magolor/stdlib",
                                          "/usr/share/magolor/stdlib"};

  // Add HOME-based path
  const char *home = getenv("HOME");
  if (home) {
    searchPaths.push_back(std::string(home) + "/.magolor/stdlib");
  }

  for (const auto &searchPath : searchPaths) {
    if (fs::exists(searchPath) && fs::is_directory(searchPath)) {
      stdlibPath = searchPath;
      std::cerr << "[StdLibLoader] Found stdlib at: " << searchPath
                << std::endl;
      break;
    }
  }

  if (stdlibPath.empty()) {
    std::cerr << "[StdLibLoader] WARNING: stdlib directory not found!"
              << std::endl;
    initialized = true;
    return;
  }

  discoverModules();
  initialized = true;

  std::cerr << "[StdLibLoader] Loaded " << modules.size() << " modules"
            << std::endl;
}

void StdLibLoader::discoverModules() {
  if (!fs::exists(stdlibPath)) {
    return;
  }

  // Scan for .mg files
  for (const auto &entry : fs::recursive_directory_iterator(stdlibPath)) {
    if (!entry.is_regular_file())
      continue;
    if (entry.path().extension() != ".mg")
      continue;

    std::string filePath = entry.path().string();
    std::string relPath = fs::relative(entry.path(), stdlibPath).string();

    // Convert path to module name
    // e.g., "io.mg" -> "Std.IO", "core/prelude.mg" -> "Std.Core.Prelude"
    std::string moduleName = "Std";

    // Remove .mg extension
    std::string pathWithoutExt = relPath;
    if (pathWithoutExt.size() > 3) {
      pathWithoutExt = pathWithoutExt.substr(0, pathWithoutExt.size() - 3);
    }

    // Split by path separators and capitalize
    std::stringstream ss(pathWithoutExt);
    std::string segment;
    char separator = '/';
#ifdef _WIN32
    separator = '\\';
#endif

    while (std::getline(ss, segment, separator)) {
      if (segment.empty())
        continue;
      // Capitalize first letter
      std::string capitalizedName = segment;
      capitalizedName[0] = std::toupper(capitalizedName[0]);
      moduleName += "." + capitalizedName;
    }

    StdModule module;
    // After registering the capitalized version, also register lowercase
    modules[moduleName] = module;
    std::string lowerName = "Std." + segment; // Keep original case
    modules[lowerName] = module;
    module.name = moduleName.substr(moduleName.rfind('.') + 1);
    module.fullPath = moduleName;
    module.filePath = filePath;

    // Register with full path
    modules[moduleName] = module;

    // Also register short form (e.g., "Std.IO" for "Std.Core.IO")
    std::string shortName = "Std." + module.name;
    if (modules.find(shortName) == modules.end()) {
      modules[shortName] = module;
    }

    std::cerr << "[StdLibLoader] Discovered module: " << moduleName << " from "
              << relPath << std::endl;
  }
}

StdModule *StdLibLoader::loadModule(const std::string &modulePath) {
  auto it = modules.find(modulePath);
  if (it == modules.end()) {
    // Try case-insensitive search
    for (auto &[path, mod] : modules) {
      std::string lowerPath = path;
      std::string lowerSearch = modulePath;
      std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(),
                     ::tolower);
      std::transform(lowerSearch.begin(), lowerSearch.end(),
                     lowerSearch.begin(), ::tolower);
      if (lowerPath == lowerSearch) {
        it = modules.find(path);
        break;
      }
    }
    if (it == modules.end()) {
      return nullptr;
    }
  }

  StdModule &module = it->second;

  if (!module.loaded) {
    parseModuleFile(module);
    module.loaded = true;
  }

  return &module;
}

bool StdLibLoader::hasModule(const std::string &modulePath) const {
  if (modules.find(modulePath) != modules.end()) {
    return true;
  }
  // Try case-insensitive
  for (const auto &[path, _] : modules) {
    std::string lowerPath = path;
    std::string lowerSearch = modulePath;
    std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(),
                   ::tolower);
    std::transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(),
                   ::tolower);
    if (lowerPath == lowerSearch)
      return true;
  }
  return false;
}

std::vector<std::string> StdLibLoader::getAvailableModules() const {
  std::vector<std::string> result;
  std::unordered_set<std::string> seen;

  for (const auto &[path, _] : modules) {
    // Only return Std.X style paths (one dot)
    size_t dotCount = std::count(path.begin(), path.end(), '.');
    if (dotCount == 1) {
      if (seen.insert(path).second) {
        result.push_back(path);
      }
    }
  }

  std::sort(result.begin(), result.end());
  return result;
}

void StdLibLoader::parseModuleFile(StdModule &module) {
  std::ifstream file(module.filePath);
  if (!file) {
    std::cerr << "[StdLibLoader] Failed to open: " << module.filePath
              << std::endl;
    return;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string source = buffer.str();

  extractMetadata(source, module);
  extractFunctions(source, module);
  extractClasses(source, module);
  extractEnums(source, module);
  extractConstants(source, module);

  module.parsed = true;

  std::cerr << "[StdLibLoader] Parsed " << module.fullPath << ": "
            << module.functions.size() << " functions, "
            << module.classes.size() << " classes"
            << ", " << module.metadata.linkFlags.size() << " link flags"
            << std::endl;
}

void StdLibLoader::extractFunctions(const std::string &source,
                                    StdModule &module) {
  // Match: pub fn name(params) -> returnType
  // Also match: pub static fn name(params) -> returnType
  std::regex funcRegex(R"((?://[^\n]*\n)*)"             // Optional doc comments
                       R"(pub\s+(?:static\s+)?fn\s+)"   // pub [static] fn
                       R"(([a-zA-Z_][a-zA-Z0-9_]*)\s*)" // function name
                       R"(\(([^)]*)\)\s*)"              // parameters
                       R"((?:->\s*([^\{]+?))?)"         // optional return type
                       R"(\s*\{)"                       // opening brace
  );

  auto begin = std::sregex_iterator(source.begin(), source.end(), funcRegex);
  auto end = std::sregex_iterator();

  for (auto it = begin; it != end; ++it) {
    StdFunction func;
    func.name = (*it)[1].str();
    std::string params = (*it)[2].str();
    func.returnType = (*it)[3].matched ? (*it)[3].str() : "void";
    func.isStatic =
        source.substr((*it).position(), (*it).length()).find("static") !=
        std::string::npos;

    // Trim return type
    func.returnType.erase(0, func.returnType.find_first_not_of(" \t\n\r"));
    func.returnType.erase(func.returnType.find_last_not_of(" \t\n\r") + 1);
    if (func.returnType.empty())
      func.returnType = "void";

    // Parse parameters
    auto parsedParams = parseParams(params);
    std::string sig = "(";
    bool first = true;

    for (const auto &[pname, ptype] : parsedParams) {
      func.paramNames.push_back(pname);
      func.paramTypes.push_back(ptype);

      if (!first)
        sig += ", ";
      first = false;
      sig += pname + ": " + ptype;
    }

    sig += ") -> " + func.returnType;
    func.signature = sig;

    // Extract doc comment
    func.documentation = extractDocComment(source, (*it).position());

    module.functions.push_back(func);
    functionToModule[func.name] = module.fullPath;
    allFunctions[func.name] = func;
  }
}

void StdLibLoader::extractClasses(const std::string &source,
                                  StdModule &module) {
  // Match: pub class ClassName { ... }
  std::regex classRegex(R"((?://[^\n]*\n)*)" // Optional doc comments
                        R"(pub\s+class\s+)"  // pub class
                        R"(([a-zA-Z_][a-zA-Z0-9_]*)\s*)" // class name
                        R"(\{)"                          // opening brace
  );

  auto begin = std::sregex_iterator(source.begin(), source.end(), classRegex);
  auto end = std::sregex_iterator();

  for (auto it = begin; it != end; ++it) {
    StdClass cls;
    cls.name = (*it)[1].str();
    cls.documentation = extractDocComment(source, (*it).position());

    // Find class body
    size_t braceStart = source.find('{', (*it).position());
    if (braceStart == std::string::npos)
      continue;

    // Find matching closing brace
    int depth = 1;
    size_t pos = braceStart + 1;
    while (pos < source.size() && depth > 0) {
      if (source[pos] == '{')
        depth++;
      else if (source[pos] == '}')
        depth--;
      pos++;
    }

    std::string classBody = source.substr(braceStart + 1, pos - braceStart - 2);

    // Extract fields: pub fieldName: Type;
    std::regex fieldRegex(R"(pub\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*:\s*([^;]+);)");
    auto fBegin =
        std::sregex_iterator(classBody.begin(), classBody.end(), fieldRegex);
    auto fEnd = std::sregex_iterator();

    for (auto fit = fBegin; fit != fEnd; ++fit) {
      std::string fieldName = (*fit)[1].str();
      std::string fieldType = (*fit)[2].str();
      fieldType.erase(0, fieldType.find_first_not_of(" \t"));
      fieldType.erase(fieldType.find_last_not_of(" \t") + 1);
      cls.fields.push_back({fieldName, fieldType});
    }

    // Extract static constants: pub static NAME: Type = ...;
    std::regex constRegex(
        R"(pub\s+static\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*:\s*([a-zA-Z_][a-zA-Z0-9_<>]*)\s*=)");
    auto cBegin =
        std::sregex_iterator(classBody.begin(), classBody.end(), constRegex);
    auto cEnd = std::sregex_iterator();

    for (auto cit = cBegin; cit != cEnd; ++cit) {
      cls.staticConstants.push_back({(*cit)[1].str(), (*cit)[2].str()});
    }

    // Extract methods within class
    std::regex methodRegex(R"(pub\s+(?:static\s+)?fn\s+)"
                           R"(([a-zA-Z_][a-zA-Z0-9_]*)\s*)"
                           R"(\(([^)]*)\)\s*)"
                           R"((?:->\s*([^\{]+?))?)"
                           R"(\s*\{)");

    auto mBegin =
        std::sregex_iterator(classBody.begin(), classBody.end(), methodRegex);
    auto mEnd = std::sregex_iterator();

    for (auto mit = mBegin; mit != mEnd; ++mit) {
      StdFunction method;
      method.name = (*mit)[1].str();
      std::string params = (*mit)[2].str();
      method.returnType = (*mit)[3].matched ? (*mit)[3].str() : "void";

      method.returnType.erase(0,
                              method.returnType.find_first_not_of(" \t\n\r"));
      method.returnType.erase(method.returnType.find_last_not_of(" \t\n\r") +
                              1);
      if (method.returnType.empty())
        method.returnType = "void";

      auto parsedParams = parseParams(params);
      std::string sig = "(";
      bool first = true;

      for (const auto &[pname, ptype] : parsedParams) {
        method.paramNames.push_back(pname);
        method.paramTypes.push_back(ptype);
        if (!first)
          sig += ", ";
        first = false;
        sig += pname + ": " + ptype;
      }

      sig += ") -> " + method.returnType;
      method.signature = sig;

      cls.methods.push_back(method);
    }

    module.classes.push_back(cls);
    allClasses[cls.name] = cls;
  }
}

void StdLibLoader::extractEnums(const std::string &source, StdModule &module) {
  // Match: pub enum EnumName { Variant1, Variant2, ... }
  std::regex enumRegex(R"(pub\s+enum\s+)"
                       R"(([a-zA-Z_][a-zA-Z0-9_]*)\s*)"
                       R"(\{([^}]*)\})");

  auto begin = std::sregex_iterator(source.begin(), source.end(), enumRegex);
  auto end = std::sregex_iterator();

  for (auto it = begin; it != end; ++it) {
    StdEnum enumDef;
    enumDef.name = (*it)[1].str();
    enumDef.documentation = extractDocComment(source, (*it).position());

    std::string body = (*it)[2].str();

    // Parse variants
    std::regex variantRegex(R"(([a-zA-Z_][a-zA-Z0-9_]*))");
    auto vBegin = std::sregex_iterator(body.begin(), body.end(), variantRegex);
    auto vEnd = std::sregex_iterator();

    for (auto vit = vBegin; vit != vEnd; ++vit) {
      enumDef.variants.push_back((*vit)[1].str());
    }

    module.enums.push_back(enumDef);
  }
}

void StdLibLoader::extractConstants(const std::string &source,
                                    StdModule &module) {
  // Match: pub static NAME: Type = value;
  std::regex constRegex(
      R"(pub\s+static\s+)"
      R"(([A-Z_][A-Z0-9_]*)\s*:\s*)"      // Constant name (UPPERCASE)
      R"(([a-zA-Z_][a-zA-Z0-9_<>]*)\s*=)" // Type
  );

  auto begin = std::sregex_iterator(source.begin(), source.end(), constRegex);
  auto end = std::sregex_iterator();

  for (auto it = begin; it != end; ++it) {
    std::string constName = (*it)[1].str();
    std::string constType = (*it)[2].str();

    StdFunction constant;
    constant.name = constName;
    constant.returnType = constType;
    constant.isConstant = true;
    constant.signature = constName + ": " + constType;

    module.functions.push_back(constant);
    module.constants.push_back(constName);
    functionToModule[constName] = module.fullPath;
    allFunctions[constName] = constant;
  }
}

std::vector<std::pair<std::string, std::string>>
StdLibLoader::parseParams(const std::string &paramStr) {
  std::vector<std::pair<std::string, std::string>> result;

  if (paramStr.empty())
    return result;

  // Handle nested generics by tracking angle bracket depth
  std::vector<std::string> params;
  std::string current;
  int angleBrackets = 0;

  for (char c : paramStr) {
    if (c == '<')
      angleBrackets++;
    else if (c == '>')
      angleBrackets--;
    else if (c == ',' && angleBrackets == 0) {
      params.push_back(current);
      current.clear();
      continue;
    }
    current += c;
  }
  if (!current.empty())
    params.push_back(current);

  // Parse each param: "name: type"
  std::regex paramRegex(R"(([a-zA-Z_][a-zA-Z0-9_]*)\s*:\s*(.+))");

  for (const auto &param : params) {
    std::string trimmed = param;
    trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
    trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);

    std::smatch match;
    if (std::regex_match(trimmed, match, paramRegex)) {
      std::string name = match[1].str();
      std::string type = match[2].str();
      type.erase(0, type.find_first_not_of(" \t"));
      type.erase(type.find_last_not_of(" \t") + 1);
      result.push_back({name, type});
    }
  }

  return result;
}

std::string StdLibLoader::extractDocComment(const std::string &source,
                                            size_t pos) {
  // Look backwards for // comments
  std::string doc;

  if (pos == 0)
    return doc;

  // Find start of line
  size_t lineStart = pos;
  while (lineStart > 0 && source[lineStart - 1] != '\n') {
    lineStart--;
  }

  // Look at previous lines for comments
  std::vector<std::string> commentLines;
  size_t searchPos = lineStart;

  while (searchPos > 0) {
    // Find previous line
    size_t prevEnd = searchPos - 1;
    if (prevEnd > 0 && source[prevEnd] == '\n')
      prevEnd--;

    size_t prevStart = prevEnd;
    while (prevStart > 0 && source[prevStart - 1] != '\n') {
      prevStart--;
    }

    std::string prevLine = source.substr(prevStart, prevEnd - prevStart + 1);

    // Trim
    prevLine.erase(0, prevLine.find_first_not_of(" \t"));

    if (prevLine.find("//") == 0) {
      std::string comment = prevLine.substr(2);
      comment.erase(0, comment.find_first_not_of(" \t"));
      commentLines.insert(commentLines.begin(), comment);
      searchPos = prevStart;
    } else if (prevLine.empty()) {
      searchPos = prevStart;
    } else {
      break;
    }
  }

  for (const auto &line : commentLines) {
    if (!doc.empty())
      doc += "\n";
    doc += line;
  }

  return doc;
}

std::vector<StdFunction>
StdLibLoader::getFunctions(const std::string &modulePath) {
  auto *module = loadModule(modulePath);
  if (!module)
    return {};
  return module->functions;
}

std::vector<StdClass> StdLibLoader::getClasses(const std::string &modulePath) {
  auto *module = loadModule(modulePath);
  if (!module)
    return {};
  return module->classes;
}

std::vector<StdEnum> StdLibLoader::getEnums(const std::string &modulePath) {
  auto *module = loadModule(modulePath);
  if (!module)
    return {};
  return module->enums;
}

bool StdLibLoader::isStdFunction(const std::string &funcName) const {
  return allFunctions.find(funcName) != allFunctions.end();
}

std::string
StdLibLoader::getModuleForFunction(const std::string &funcName) const {
  auto it = functionToModule.find(funcName);
  return it != functionToModule.end() ? it->second : "";
}

std::string
StdLibLoader::getFunctionSignature(const std::string &funcName) const {
  auto it = allFunctions.find(funcName);
  return it != allFunctions.end() ? it->second.signature : "";
}

std::vector<std::string> StdLibLoader::getAllFunctionNames() const {
  std::vector<std::string> names;
  names.reserve(allFunctions.size());
  for (const auto &[name, _] : allFunctions) {
    names.push_back(name);
  }
  return names;
}

std::vector<std::string> StdLibLoader::getAllClassNames() const {
  std::vector<std::string> names;
  names.reserve(allClasses.size());
  for (const auto &[name, _] : allClasses) {
    names.push_back(name);
  }
  return names;
}

std::string StdLibLoader::getReturnType(const std::string &funcName) const {
  auto it = allFunctions.find(funcName);
  if (it == allFunctions.end())
    return "";
  return it->second.returnType;
}

// ============================================================================
// StdLibCodeGen Implementation
// ============================================================================

std::string StdLibCodeGen::generateIncludes() {
  return R"(#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <optional>
#include <algorithm>
#include <chrono>
#include <thread>
#include <random>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <numeric>
#include <regex>

)";
}

std::string StdLibCodeGen::generateHelpers() {
  return R"(
// Template helpers for string conversion
template<typename T>
inline std::string mg_to_string(const T& val) { 
    std::ostringstream oss; 
    oss << val; 
    return oss.str(); 
}

template<>
inline std::string mg_to_string(const bool& val) {
    return val ? "true" : "false";
}

template<>
inline std::string mg_to_string(const std::string& val) {
    return val;
}

// Global Option helpers
template<typename T>
inline bool isSome(const std::optional<T>& opt) { return opt.has_value(); }

template<typename T>
inline bool isNone(const std::optional<T>& opt) { return !opt.has_value(); }

template<typename T>
inline T unwrap(const std::optional<T>& opt) {
    if (!opt.has_value()) {
        throw std::runtime_error("Called unwrap on None value");
    }
    return opt.value();
}

template<typename T>
inline T unwrapOr(const std::optional<T>& opt, const T& defaultValue) {
    return opt.value_or(defaultValue);
}

)";
}

std::string
StdLibCodeGen::generateAll(const std::unordered_set<std::string> &usedModules) {
  std::stringstream ss;
  ss << generateIncludes();
  ss << generateHelpers();
  ss << generateNamespace(usedModules);
  return ss.str();
}

std::string StdLibCodeGen::generateNamespace(
    const std::unordered_set<std::string> &usedModules) {
  std::stringstream ss;

  ss << "namespace Std {\n\n";

  // Always include prelude
  ss << generateModuleCode("Std.Core.Prelude");

  // Generate code for each used module
  for (const auto &mod : usedModules) {
    if (mod.find("Prelude") == std::string::npos) {
      ss << generateModuleCode(mod);
    }
  }

  ss << "} // namespace Std\n\n";

  // Add global using declarations
  ss << "using Std::println;\n";
  ss << "using Std::print;\n";
  ss << "using Std::readLine;\n\n";

  return ss.str();
}

std::string StdLibCodeGen::generateModuleCode(const std::string &modulePath) {
  auto &loader = StdLibLoader::instance();
  auto *module = loader.loadModule(modulePath);

  if (!module || module->filePath.empty()) {
    return "// Module not found: " + modulePath + "\n";
  }

  // Read source file
  std::ifstream file(module->filePath);
  if (!file) {
    return "// Could not read: " + module->filePath + "\n";
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string source = buffer.str();

  std::stringstream ss;
  ss << "// " << std::string(70, '=') << "\n";
  ss << "// " << modulePath << "\n";
  ss << "// " << std::string(70, '=') << "\n";
  ss << "namespace " << module->name << " {\n\n";

  // Generate each function
  for (const auto &func : module->functions) {
    std::string cppCode = extractCppCode(source, func.name);

    if (func.isConstant) {
      ss << "    constexpr ";
      if (func.returnType == "int")
        ss << "int64_t";
      else if (func.returnType == "float")
        ss << "double";
      else
        ss << func.returnType;
      ss << " " << func.name << " = 0; // TODO: extract value\n";
    } else if (!cppCode.empty()) {
      ss << "    inline ";

      // Return type
      if (func.returnType == "int")
        ss << "int64_t";
      else if (func.returnType == "float")
        ss << "double";
      else if (func.returnType == "string")
        ss << "std::string";
      else if (func.returnType == "bool")
        ss << "bool";
      else if (func.returnType == "void")
        ss << "void";
      else if (func.returnType.find("Option<") == 0) {
        std::string inner =
            func.returnType.substr(7, func.returnType.size() - 8);
        ss << "std::optional<";
        if (inner == "int")
          ss << "int64_t";
        else if (inner == "string")
          ss << "std::string";
        else if (inner == "float")
          ss << "double";
        else
          ss << inner;
        ss << ">";
      } else if (func.returnType.find("Array<") == 0) {
        std::string inner =
            func.returnType.substr(6, func.returnType.size() - 7);
        ss << "std::vector<";
        if (inner == "int")
          ss << "int64_t";
        else if (inner == "string")
          ss << "std::string";
        else
          ss << inner;
        ss << ">";
      } else
        ss << func.returnType;

      ss << " " << func.name << "(";

      // Parameters
      for (size_t i = 0; i < func.paramNames.size(); i++) {
        if (i > 0)
          ss << ", ";

        const std::string &ptype = func.paramTypes[i];
        if (ptype == "int")
          ss << "int64_t";
        else if (ptype == "float")
          ss << "double";
        else if (ptype == "string")
          ss << "const std::string&";
        else if (ptype == "bool")
          ss << "bool";
        else if (ptype.find("Array<") == 0) {
          std::string inner = ptype.substr(6, ptype.size() - 7);
          ss << "std::vector<";
          if (inner == "int")
            ss << "int64_t";
          else if (inner == "string")
            ss << "std::string";
          else
            ss << inner;
          ss << ">&";
        } else
          ss << "const " << ptype << "&";

        ss << " " << func.paramNames[i];
      }

      ss << ") {\n";
      ss << "        " << cppCode << "\n";
      ss << "    }\n\n";
    }
  }

  ss << "}\n\n";
  return ss.str();
}


std::string StdLibCodeGen::extractCppCode(const std::string &source,
                                          const std::string &funcName) {
  // Build pattern to find the function
  std::string funcPattern = "pub\\s+(?:static\\s+)?fn\\s+" + funcName + "\\s*\\(";
  std::regex funcStartRegex(funcPattern);
  std::smatch match;

  if (!std::regex_search(source, match, funcStartRegex)) {
    return "";
  }

  size_t funcStart = match.position();
  
  // Find the opening brace of the function body
  size_t funcBodyStart = source.find('{', funcStart);
  if (funcBodyStart == std::string::npos) {
    return "";
  }
  
  // Find the matching closing brace of the function using proper brace counting
  int depth = 1;
  size_t pos = funcBodyStart + 1;
  bool inString = false;
  char stringChar = 0;
  
  while (pos < source.size() && depth > 0) {
    char c = source[pos];
    char prev = (pos > 0) ? source[pos - 1] : 0;
    
    // Handle string literals - don't count braces inside strings
    if ((c == '"' || c == '\'') && prev != '\\') {
      if (!inString) {
        inString = true;
        stringChar = c;
      } else if (c == stringChar) {
        inString = false;
      }
    }
    
    if (!inString) {
      if (c == '{') depth++;
      else if (c == '}') depth--;
    }
    pos++;
  }
  
  if (depth != 0) {
    return "";
  }
  
  size_t funcBodyEnd = pos - 1;
  std::string funcBody = source.substr(funcBodyStart + 1, funcBodyEnd - funcBodyStart - 1);
  
  // Now find @cpp block within the function body
  size_t cppStart = funcBody.find("@cpp");
  if (cppStart == std::string::npos) {
    // CRITICAL FIX: No @cpp block found - return empty string!
    // This prevents Magolor syntax from appearing in C++ output
    return "";
  }
  
  // Find opening brace after @cpp
  size_t cppBraceStart = funcBody.find('{', cppStart);
  if (cppBraceStart == std::string::npos) {
    return "";
  }
  
  // Find matching closing brace for @cpp block
  depth = 1;
  pos = cppBraceStart + 1;
  inString = false;
  stringChar = 0;
  
  while (pos < funcBody.size() && depth > 0) {
    char c = funcBody[pos];
    char prev = (pos > 0) ? funcBody[pos - 1] : 0;
    
    if ((c == '"' || c == '\'') && prev != '\\') {
      if (!inString) {
        inString = true;
        stringChar = c;
      } else if (c == stringChar) {
        inString = false;
      }
    }
    
    if (!inString) {
      if (c == '{') depth++;
      else if (c == '}') depth--;
    }
    pos++;
  }
  
  if (depth != 0) {
    return "";
  }
  
  std::string code = funcBody.substr(cppBraceStart + 1, pos - cppBraceStart - 2);

  // Trim whitespace
  size_t start = code.find_first_not_of(" \t\n\r");
  size_t end = code.find_last_not_of(" \t\n\r");

  if (start != std::string::npos && end != std::string::npos) {
    return code.substr(start, end - start + 1);
  }

  return code;
}// ============================================================================
// Metadata extraction (@link, @include, @cimport)
// ============================================================================

void StdLibLoader::extractMetadata(const std::string &source,
                                   StdModule &module) {
  // Extract @link { ... }
  std::regex linkRegex(R"(@link\s*\{\s*([^}]+)\})");
  std::smatch match;
  std::string::const_iterator searchStart = source.cbegin();

  while (std::regex_search(searchStart, source.cend(), match, linkRegex)) {
    std::string flags = match[1].str();
    std::istringstream iss(flags);
    std::string flag;
    while (iss >> flag) {
      // Trim whitespace
      size_t start = flag.find_first_not_of(" \t\n\r");
      size_t end = flag.find_last_not_of(" \t\n\r");
      if (start != std::string::npos && end != std::string::npos) {
        module.metadata.linkFlags.push_back(
            flag.substr(start, end - start + 1));
      }
    }
    searchStart = match.suffix().first;
  }

  // Extract @include { ... }
  std::regex includeRegex(R"(@include\s*\{\s*([^}]+)\})");
  searchStart = source.cbegin();

  while (std::regex_search(searchStart, source.cend(), match, includeRegex)) {
    std::string includes = match[1].str();
    // Split by whitespace or comma
    std::regex tokenRegex(R"(<[^>]+>|\"[^\"]+\"|[^\s,]+)");
    auto tokensBegin =
        std::sregex_iterator(includes.begin(), includes.end(), tokenRegex);
    auto tokensEnd = std::sregex_iterator();

    for (auto it = tokensBegin; it != tokensEnd; ++it) {
      module.metadata.cppIncludes.push_back(it->str());
    }
    searchStart = match.suffix().first;
  }

  // Extract @cimport { ... } or cimport "..."
  std::regex cimportRegex(
      R"(@?cimport\s*(?:\{\s*([^}]+)\}|\"([^\"]+)\"|<([^>]+)>))");
  searchStart = source.cbegin();

  while (std::regex_search(searchStart, source.cend(), match, cimportRegex)) {
    std::string header;
    if (match[1].matched)
      header = match[1].str();
    else if (match[2].matched)
      header = match[2].str();
    else if (match[3].matched)
      header = match[3].str();

    if (!header.empty()) {
      module.metadata.cimports.push_back(header);
    }
    searchStart = match.suffix().first;
  }
}

ModuleMetadata StdLibLoader::getModuleMetadata(const std::string &modulePath) {
  auto *module = loadModule(modulePath);
  if (module) {
    return module->metadata;
  }
  return ModuleMetadata{};
}

std::vector<std::string>
StdLibLoader::getLinkFlags(const std::string &modulePath) {
  return getModuleMetadata(modulePath).linkFlags;
}

std::vector<std::string>
StdLibLoader::getCppIncludes(const std::string &modulePath) {
  return getModuleMetadata(modulePath).cppIncludes;
}

std::vector<std::string> StdLibLoader::collectLinkFlags(
    const std::unordered_set<std::string> &usedModules) {
  std::unordered_set<std::string> uniqueFlags;
  std::vector<std::string> result;

  for (const auto &mod : usedModules) {
    auto flags = getLinkFlags(mod);
    for (const auto &flag : flags) {
      if (uniqueFlags.insert(flag).second) {
        result.push_back(flag);
      }
    }
  }

  return result;
}

std::vector<std::string> StdLibLoader::collectIncludes(
    const std::unordered_set<std::string> &usedModules) {
  std::unordered_set<std::string> uniqueIncludes;
  std::vector<std::string> result;

  for (const auto &mod : usedModules) {
    auto includes = getCppIncludes(mod);
    for (const auto &inc : includes) {
      if (uniqueIncludes.insert(inc).second) {
        result.push_back(inc);
      }
    }
  }

  return result;
}

// ============================================================================
// CHeaderParser Implementation
// ============================================================================

void CHeaderParser::init() {
  if (initialized)
    return;

  // Standard include paths
  includePaths = {"/usr/include",
                  "/usr/local/include",
                  "/usr/include/x86_64-linux-gnu",
                  "/usr/include/c++/11",
                  "/usr/include/c++/12",
                  "/usr/include/c++/13"};

  // Add paths from environment
  const char *cpath = getenv("CPATH");
  if (cpath) {
    std::istringstream iss(cpath);
    std::string path;
    while (std::getline(iss, path, ':')) {
      if (!path.empty()) {
        includePaths.push_back(path);
      }
    }
  }

  const char *cplus = getenv("CPLUS_INCLUDE_PATH");
  if (cplus) {
    std::istringstream iss(cplus);
    std::string path;
    while (std::getline(iss, path, ':')) {
      if (!path.empty()) {
        includePaths.push_back(path);
      }
    }
  }

  initialized = true;
}

void CHeaderParser::addIncludePath(const std::string &path) {
  includePaths.push_back(path);
}

std::string CHeaderParser::resolveHeaderPath(const std::string &headerPath) {
  std::string cleanPath = headerPath;

  // Remove < > or " "
  if (!cleanPath.empty() && (cleanPath[0] == '<' || cleanPath[0] == '"')) {
    cleanPath = cleanPath.substr(1);
  }
  if (!cleanPath.empty() &&
      (cleanPath.back() == '>' || cleanPath.back() == '"')) {
    cleanPath = cleanPath.substr(0, cleanPath.size() - 1);
  }

  // Check if it's an absolute path
  if (fs::exists(cleanPath)) {
    return cleanPath;
  }

  // Search in include paths
  for (const auto &incPath : includePaths) {
    std::string fullPath = incPath + "/" + cleanPath;
    if (fs::exists(fullPath)) {
      return fullPath;
    }
  }

  return "";
}

CHeader *CHeaderParser::parseHeader(const std::string &headerPath) {
  if (!initialized)
    init();

  // Check cache
  auto it = headers.find(headerPath);
  if (it != headers.end() && it->second.parsed) {
    return &it->second;
  }

  std::string absPath = resolveHeaderPath(headerPath);
  if (absPath.empty()) {
    std::cerr << "[CHeaderParser] Could not resolve: " << headerPath
              << std::endl;
    return nullptr;
  }

  std::ifstream file(absPath);
  if (!file) {
    std::cerr << "[CHeaderParser] Could not open: " << absPath << std::endl;
    return nullptr;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string content = buffer.str();

  CHeader &header = headers[headerPath];
  header.path = headerPath;
  header.absolutePath = absPath;

  parseHeaderContent(content, header);
  header.parsed = true;

  std::cerr << "[CHeaderParser] Parsed " << headerPath << ": "
            << header.symbols.size() << " symbols" << std::endl;

  return &header;
}

void CHeaderParser::parseHeaderContent(const std::string &content,
                                       CHeader &header) {
  extractFunctions(content, header);
  extractStructs(content, header);
  extractEnums(content, header);
  extractTypedefs(content, header);
  extractMacros(content, header);
}

void CHeaderParser::extractFunctions(const std::string &content,
                                     CHeader &header) {
  // Match C function declarations
  // Handles: returnType funcName(params);
  // Also handles: extern returnType funcName(params);
  std::regex funcRegex(R"((?:extern\s+)?)" // Optional extern
                       R"((?:const\s+)?)"  // Optional const
                       R"(([a-zA-Z_][a-zA-Z0-9_]*(?:\s*\*)*)\s+)" // Return type
                       R"(([a-zA-Z_][a-zA-Z0-9_]*)\s*)" // Function name
                       R"(\(([^)]*)\)\s*;)" // Parameters and semicolon
  );

  auto begin = std::sregex_iterator(content.begin(), content.end(), funcRegex);
  auto end = std::sregex_iterator();

  for (auto it = begin; it != end; ++it) {
    CSymbol sym;
    sym.kind = CSymbolKind::Function;
    sym.returnType = (*it)[1].str();
    sym.name = (*it)[2].str();
    sym.headerFile = header.path;

    // Parse parameters
    std::string params = (*it)[3].str();
    if (params != "void" && !params.empty()) {
      std::regex paramRegex(R"(([^,]+))");
      auto pbegin =
          std::sregex_iterator(params.begin(), params.end(), paramRegex);
      auto pend = std::sregex_iterator();

      for (auto pit = pbegin; pit != pend; ++pit) {
        std::string param = (*pit)[1].str();
        // Trim
        size_t start = param.find_first_not_of(" \t");
        size_t endp = param.find_last_not_of(" \t");
        if (start != std::string::npos) {
          param = param.substr(start, endp - start + 1);
        }

        // Split type and name
        size_t lastSpace = param.rfind(' ');
        size_t lastStar = param.rfind('*');
        size_t split = std::max(lastSpace, lastStar);

        if (split != std::string::npos && split < param.size() - 1) {
          std::string type = param.substr(0, split + 1);
          std::string name = param.substr(split + 1);
          // Trim name
          size_t ns = name.find_first_not_of(" \t*");
          if (ns != std::string::npos) {
            name = name.substr(ns);
          }
          sym.paramTypes.push_back(type);
          sym.paramNames.push_back(name);
        } else {
          sym.paramTypes.push_back(param);
          sym.paramNames.push_back("");
        }
      }
    }

    header.symbols.push_back(sym);
  }
}

void CHeaderParser::extractStructs(const std::string &content,
                                   CHeader &header) {
  // Match struct definitions
  std::regex structRegex(
      R"((?:typedef\s+)?struct\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\{([^}]*)\})");

  auto begin =
      std::sregex_iterator(content.begin(), content.end(), structRegex);
  auto end = std::sregex_iterator();

  for (auto it = begin; it != end; ++it) {
    CSymbol sym;
    sym.kind = CSymbolKind::Struct;
    sym.name = (*it)[1].str();
    sym.headerFile = header.path;

    // Parse fields
    std::string body = (*it)[2].str();
    std::regex fieldRegex(
        R"(([a-zA-Z_][a-zA-Z0-9_*\s]+)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*;)");
    auto fbegin = std::sregex_iterator(body.begin(), body.end(), fieldRegex);
    auto fend = std::sregex_iterator();

    for (auto fit = fbegin; fit != fend; ++fit) {
      std::string type = (*fit)[1].str();
      std::string name = (*fit)[2].str();
      // Trim type
      size_t start = type.find_first_not_of(" \t\n");
      size_t endp = type.find_last_not_of(" \t\n");
      if (start != std::string::npos) {
        type = type.substr(start, endp - start + 1);
      }
      sym.fields.emplace_back(name, type);
    }

    header.symbols.push_back(sym);
  }
}

void CHeaderParser::extractEnums(const std::string &content, CHeader &header) {
  // Match enum definitions
  std::regex enumRegex(
      R"((?:typedef\s+)?enum\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\{([^}]*)\})");

  auto begin = std::sregex_iterator(content.begin(), content.end(), enumRegex);
  auto end = std::sregex_iterator();

  for (auto it = begin; it != end; ++it) {
    CSymbol sym;
    sym.kind = CSymbolKind::Enum;
    sym.name = (*it)[1].str();
    sym.headerFile = header.path;

    // Parse values
    std::string body = (*it)[2].str();
    std::regex valRegex(R"(([a-zA-Z_][a-zA-Z0-9_]*)\s*(?:=[^,}]*)?)");
    auto vbegin = std::sregex_iterator(body.begin(), body.end(), valRegex);
    auto vend = std::sregex_iterator();

    for (auto vit = vbegin; vit != vend; ++vit) {
      sym.enumValues.push_back((*vit)[1].str());

      // Also add as separate enum value symbol
      CSymbol enumVal;
      enumVal.kind = CSymbolKind::EnumValue;
      enumVal.name = (*vit)[1].str();
      enumVal.returnType = sym.name; // Store enum type
      enumVal.headerFile = header.path;
      header.symbols.push_back(enumVal);
    }

    header.symbols.push_back(sym);
  }
}

void CHeaderParser::extractTypedefs(const std::string &content,
                                    CHeader &header) {
  // Match simple typedefs (not struct/enum)
  std::regex typedefRegex(
      R"(typedef\s+([a-zA-Z_][a-zA-Z0-9_*\s]+)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*;)");

  auto begin =
      std::sregex_iterator(content.begin(), content.end(), typedefRegex);
  auto end = std::sregex_iterator();

  for (auto it = begin; it != end; ++it) {
    std::string type = (*it)[1].str();
    // Skip struct/enum typedefs (handled elsewhere)
    if (type.find("struct") != std::string::npos ||
        type.find("enum") != std::string::npos) {
      continue;
    }

    CSymbol sym;
    sym.kind = CSymbolKind::Typedef;
    sym.name = (*it)[2].str();
    sym.returnType = type;
    sym.headerFile = header.path;

    header.symbols.push_back(sym);
  }
}

void CHeaderParser::extractMacros(const std::string &content, CHeader &header) {
  // Match #define macros (simple ones and function-like)
  std::regex macroRegex(
      R"(#define\s+([a-zA-Z_][a-zA-Z0-9_]*)(?:\(([^)]*)\))?)");

  auto begin = std::sregex_iterator(content.begin(), content.end(), macroRegex);
  auto end = std::sregex_iterator();

  for (auto it = begin; it != end; ++it) {
    CSymbol sym;
    sym.kind = CSymbolKind::Macro;
    sym.name = (*it)[1].str();
    sym.headerFile = header.path;

    // If function-like macro, parse params
    if ((*it)[2].matched) {
      std::string params = (*it)[2].str();
      std::istringstream iss(params);
      std::string param;
      while (std::getline(iss, param, ',')) {
        size_t start = param.find_first_not_of(" \t");
        size_t endp = param.find_last_not_of(" \t");
        if (start != std::string::npos) {
          sym.paramNames.push_back(param.substr(start, endp - start + 1));
        }
      }
    }

    header.symbols.push_back(sym);
  }
}

std::vector<CSymbol> CHeaderParser::getSymbols(const std::string &headerPath) {
  auto *header = parseHeader(headerPath);
  if (header) {
    return header->symbols;
  }
  return {};
}

std::vector<CSymbol>
CHeaderParser::getFunctions(const std::string &headerPath) {
  std::vector<CSymbol> result;
  auto symbols = getSymbols(headerPath);
  for (const auto &sym : symbols) {
    if (sym.kind == CSymbolKind::Function) {
      result.push_back(sym);
    }
  }
  return result;
}

std::vector<CSymbol> CHeaderParser::getStructs(const std::string &headerPath) {
  std::vector<CSymbol> result;
  auto symbols = getSymbols(headerPath);
  for (const auto &sym : symbols) {
    if (sym.kind == CSymbolKind::Struct) {
      result.push_back(sym);
    }
  }
  return result;
}

std::vector<CSymbol> CHeaderParser::getEnums(const std::string &headerPath) {
  std::vector<CSymbol> result;
  auto symbols = getSymbols(headerPath);
  for (const auto &sym : symbols) {
    if (sym.kind == CSymbolKind::Enum) {
      result.push_back(sym);
    }
  }
  return result;
}

std::vector<CSymbol> CHeaderParser::getMacros(const std::string &headerPath) {
  std::vector<CSymbol> result;
  auto symbols = getSymbols(headerPath);
  for (const auto &sym : symbols) {
    if (sym.kind == CSymbolKind::Macro) {
      result.push_back(sym);
    }
  }
  return result;
}

CSymbol *CHeaderParser::findSymbol(const std::string &headerPath,
                                   const std::string &name) {
  auto *header = parseHeader(headerPath);
  if (!header)
    return nullptr;

  for (auto &sym : header->symbols) {
    if (sym.name == name) {
      return &sym;
    }
  }
  return nullptr;
}

std::vector<std::string> CHeaderParser::getParsedHeaders() const {
  std::vector<std::string> result;
  for (const auto &[path, _] : headers) {
    result.push_back(path);
  }
  return result;
}
