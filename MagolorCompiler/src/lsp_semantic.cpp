#include "lsp_semantic.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <regex>

namespace fs = std::filesystem;

// ============================================================================
// SemanticAnalyzer Implementation
// ============================================================================

SemanticAnalyzer::SemanticAnalyzer() {
    initStdLib();
}

void SemanticAnalyzer::initStdLib() {
    if (stdlibInitialized) return;
    
    std::vector<std::string> searchPaths = {
        "./stdlib",
        "../stdlib",
        "../../stdlib",
        "/usr/local/share/magolor/stdlib",
        "/usr/share/magolor/stdlib"
    };
    
    const char* home = getenv("HOME");
    if (home) {
        searchPaths.push_back(std::string(home) + "/.magolor/stdlib");
    }
    
    std::string stdlibPath;
    for (const auto& path : searchPaths) {
        if (fs::exists(path) && fs::is_directory(path)) {
            stdlibPath = path;
            std::cerr << "[SemanticAnalyzer] Found stdlib at: " << path << std::endl;
            break;
        }
    }
    
    if (!stdlibPath.empty()) {
        StdLibLoader::instance().init(stdlibPath);
    } else {
        std::cerr << "[SemanticAnalyzer] WARNING: stdlib not found!" << std::endl;
    }
    
    stdlibInitialized = true;
}

bool SemanticAnalyzer::isStdLibModule(const std::string& modulePath) const {
    if (modulePath.find("Std.") != 0 && modulePath != "Std") {
        return false;
    }
    return StdLibLoader::instance().hasModule(modulePath);
}

std::vector<std::string> SemanticAnalyzer::getAvailableStdModules() const {
    return StdLibLoader::instance().getAvailableModules();
}

std::vector<SymbolPtr> SemanticAnalyzer::getStdLibSymbols(const std::string& modulePath) {
    // Check cache
    auto it = stdlibSymbolCache.find(modulePath);
    if (it != stdlibSymbolCache.end()) {
        return it->second;
    }
    
    std::vector<SymbolPtr> symbols;
    
    // Get functions
    auto functions = StdLibLoader::instance().getFunctions(modulePath);
    for (const auto& func : functions) {
        symbols.push_back(stdFunctionToSymbol(func, modulePath));
    }
    
    // Get classes
    auto classes = StdLibLoader::instance().getClasses(modulePath);
    for (const auto& cls : classes) {
        symbols.push_back(stdClassToSymbol(cls, modulePath));
        
        // Add class methods as separate symbols
        for (const auto& method : cls.methods) {
            auto methodSym = stdFunctionToSymbol(method, modulePath);
            methodSym->containerName = cls.name;
            methodSym->kind = SymbolKind::Method;
            symbols.push_back(methodSym);
        }
    }
    
    // Get enums
    auto enums = StdLibLoader::instance().getEnums(modulePath);
    for (const auto& enumDef : enums) {
        symbols.push_back(stdEnumToSymbol(enumDef, modulePath));
        
        // Add enum variants
        for (const auto& variant : enumDef.variants) {
            auto variantSym = std::make_shared<Symbol>();
            variantSym->name = variant;
            variantSym->kind = SymbolKind::EnumMember;
            variantSym->containerName = enumDef.name;
            variantSym->modulePath = modulePath;
            variantSym->isPublic = true;
            symbols.push_back(variantSym);
        }
    }
    
    stdlibSymbolCache[modulePath] = symbols;
    return symbols;
}

SymbolPtr SemanticAnalyzer::stdFunctionToSymbol(const StdFunction& func, const std::string& modulePath) {
    auto sym = std::make_shared<Symbol>();
    sym->name = func.name;
    sym->kind = func.isConstant ? SymbolKind::Constant : SymbolKind::Function;
    sym->isPublic = true;
    sym->isStatic = func.isStatic;
    sym->isCallable = !func.isConstant;
    sym->modulePath = modulePath;
    sym->detail = func.signature;
    sym->returnType = func.returnType;
    sym->paramTypes = func.paramTypes;
    sym->documentation = func.documentation;
    
    return sym;
}

SymbolPtr SemanticAnalyzer::stdClassToSymbol(const StdClass& cls, const std::string& modulePath) {
    auto sym = std::make_shared<Symbol>();
    sym->name = cls.name;
    sym->kind = SymbolKind::Class;
    sym->isPublic = true;
    sym->isCallable = false;
    sym->modulePath = modulePath;
    sym->documentation = cls.documentation;
    
    // Build detail string with fields
    std::string detail = "class " + cls.name;
    if (!cls.fields.empty()) {
        detail += " { ";
        for (size_t i = 0; i < cls.fields.size() && i < 3; i++) {
            if (i > 0) detail += ", ";
            detail += cls.fields[i].first + ": " + cls.fields[i].second;
        }
        if (cls.fields.size() > 3) detail += ", ...";
        detail += " }";
    }
    sym->detail = detail;
    
    return sym;
}

SymbolPtr SemanticAnalyzer::stdEnumToSymbol(const StdEnum& enumDef, const std::string& modulePath) {
    auto sym = std::make_shared<Symbol>();
    sym->name = enumDef.name;
    sym->kind = SymbolKind::Enum;
    sym->isPublic = true;
    sym->modulePath = modulePath;
    sym->documentation = enumDef.documentation;
    
    // Build detail string with variants
    std::string detail = "enum " + enumDef.name + " { ";
    for (size_t i = 0; i < enumDef.variants.size() && i < 5; i++) {
        if (i > 0) detail += ", ";
        detail += enumDef.variants[i];
    }
    if (enumDef.variants.size() > 5) detail += ", ...";
    detail += " }";
    sym->detail = detail;
    
    return sym;
}

void SemanticAnalyzer::analyze(const std::string& uri, const std::string& content) {
    loadProject(uri);
    extractSymbols(uri, content);
}

void SemanticAnalyzer::loadProject(const std::string& startUri) {
    if (projectLoaded) return;
    
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
    
    if (!projectRoot.empty()) {
        std::string srcDir = projectRoot + "/src";
        if (fs::exists(srcDir)) {
            scanSourceDirectory(srcDir);
        }
    }
    
    projectLoaded = true;
}

void SemanticAnalyzer::scanSourceDirectory(const std::string& srcDir) {
  std::cerr << "[scanSourceDirectory] Scanning: " << srcDir << std::endl;
    try {
        for (const auto& entry : fs::recursive_directory_iterator(srcDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".mg") {
 std::cerr << "[scanSourceDirectory] Found: " << entry.path() << std::endl;
                std::string filePath = entry.path().string();
                std::string uri = "file://" + filePath;
                
                if (fileSymbols.find(uri) != fileSymbols.end()) continue;
                
                std::ifstream file(filePath);
                if (file) {
                    std::stringstream buffer;
                    buffer << file.rdbuf();
                    extractSymbols(uri, buffer.str());
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[SemanticAnalyzer] Error scanning: " << e.what() << std::endl;
    }
}

void SemanticAnalyzer::reloadProject() {
    projectLoaded = false;
    projectRoot.clear();
    fileSymbols.clear();
    fileScopes.clear();
    moduleSymbolCache.clear();
}

void SemanticAnalyzer::extractSymbols(const std::string& uri, const std::string& content) {
    std::vector<SymbolPtr> symbols;
    auto scope = std::make_shared<Scope>();
    
    // First pass: extract metadata directives (@link, @include, @cimport)
    extractMetadataDirectives(uri, content, scope.get());
    
    std::istringstream stream(content);
    std::string line;
    int lineNum = 0;
    std::string currentClass;
    std::string currentEnum;
    
    while (std::getline(stream, line)) {
        lineNum++;
        
        size_t firstNonSpace = line.find_first_not_of(" \t");
        std::string trimmedLine = (firstNonSpace != std::string::npos) 
                                   ? line.substr(firstNonSpace) : "";
        
        // Skip directive lines (already processed)
        if (trimmedLine.find("@link") == 0 || 
            trimmedLine.find("@include") == 0 ||
            trimmedLine.find("@cimport") == 0 ||
            trimmedLine.find("cimport") == 0) {
            continue;
        }
        
        // Parse imports
        if (trimmedLine.find("using ") == 0) {
            parseImport(line, scope.get());
        }
        // Parse enums
        else if (trimmedLine.find("pub enum ") != std::string::npos ||
                 trimmedLine.find("enum ") == 0) {
            auto sym = parseEnum(line, lineNum, uri);
            if (sym) {
                symbols.push_back(sym);
                scope->define(sym);
                currentEnum = sym->name;
            }
        }
        // Parse classes
        else if (trimmedLine.find("class ") != std::string::npos ||
                 (trimmedLine.find("pub ") == 0 && trimmedLine.find("class ") != std::string::npos)) {
            auto sym = parseClass(line, lineNum, uri);
            if (sym) {
                sym->isCallable = false;
                symbols.push_back(sym);
                scope->define(sym);
                currentClass = sym->name;
                currentEnum = "";
            }
        }
        // Parse functions/methods
        else if (trimmedLine.find("fn ") != std::string::npos ||
                 (trimmedLine.find("pub ") == 0 && trimmedLine.find("fn ") != std::string::npos) ||
                 (trimmedLine.find("pub static ") == 0 && trimmedLine.find("fn ") != std::string::npos)) {
            auto sym = parseFunction(line, lineNum, uri);
            if (sym) {
                sym->containerName = currentClass;
                sym->isCallable = true;
                sym->kind = currentClass.empty() ? SymbolKind::Function : SymbolKind::Method;
                symbols.push_back(sym);
                scope->define(sym);
            }
        }
        // Parse variables
        else if (trimmedLine.find("let ") == 0) {
            auto sym = parseVariable(line, lineNum, uri);
            if (sym) {
                sym->containerName = currentClass;
                sym->isCallable = false;
                symbols.push_back(sym);
                scope->define(sym);
            }
        }
        // Parse static constants
        else if (trimmedLine.find("pub static ") == 0 && trimmedLine.find("fn ") == std::string::npos) {
            // pub static NAME: Type = value;
            std::regex constRegex(R"(pub\s+static\s+([A-Za-z_][A-Za-z0-9_]*)\s*:\s*([^=]+)\s*=)");
            std::smatch match;
            if (std::regex_search(trimmedLine, match, constRegex)) {
                auto sym = std::make_shared<Symbol>();
                sym->name = match[1].str();
                sym->kind = SymbolKind::Constant;
                sym->type = match[2].str();
                sym->isPublic = true;
                sym->isStatic = true;
                sym->containerName = currentClass;
                sym->definition.uri = uri;
                sym->definition.range.start = {lineNum - 1, 0};
                sym->definition.range.end = {lineNum - 1, (int)line.size()};
                symbols.push_back(sym);
                scope->define(sym);
            }
        }
        // Parse enum variants (inside enum block)
        else if (!currentEnum.empty() && !trimmedLine.empty() && trimmedLine != "}" && trimmedLine[0] != '/') {
            // Simple variant detection
            std::regex variantRegex(R"(([A-Za-z_][A-Za-z0-9_]*))");
            std::smatch match;
            if (std::regex_search(trimmedLine, match, variantRegex)) {
                std::string variantName = match[1].str();
                if (variantName != "pub" && variantName != "enum") {
                    auto sym = std::make_shared<Symbol>();
                    sym->name = variantName;
                    sym->kind = SymbolKind::EnumMember;
                    sym->containerName = currentEnum;
                    sym->definition.uri = uri;
                    sym->definition.range.start = {lineNum - 1, (int)firstNonSpace};
                    sym->definition.range.end = {lineNum - 1, (int)(firstNonSpace + variantName.size())};
                    symbols.push_back(sym);
                }
            }
        }
        
        // Track scope exit
        if (!currentClass.empty() && trimmedLine == "}") {
            currentClass = "";
        }
        if (!currentEnum.empty() && trimmedLine == "}") {
            currentEnum = "";
        }
    }
    
    fileSymbols[uri] = symbols;
    fileScopes[uri] = scope;
}

void SemanticAnalyzer::parseImport(const std::string& line, Scope* scope) {
    size_t usingPos = line.find("using ");
    if (usingPos == std::string::npos) return;
    
    size_t start = usingPos + 6;
    size_t end = line.find(';', start);
    if (end == std::string::npos) end = line.size();
    
    std::string importPath = line.substr(start, end - start);
    
    // Trim
    importPath.erase(0, importPath.find_first_not_of(" \t"));
    importPath.erase(importPath.find_last_not_of(" \t") + 1);
    
    // Convert :: to . if present
    size_t pos;
    while ((pos = importPath.find("::")) != std::string::npos) {
        importPath.replace(pos, 2, ".");
    }
    
    ImportedModule import;
    import.fullPath = importPath;
    import.isStdLib = isStdLibModule(importPath);
    
    if (import.isStdLib) {
        auto symbols = getStdLibSymbols(importPath);
        for (const auto& sym : symbols) {
            import.importedSymbols.push_back(sym->name);
        }
    }
    
    scope->imports.push_back(import);
}

SymbolPtr SemanticAnalyzer::parseFunction(const std::string& line, int lineNum, const std::string& uri) {
    // Match: [pub] [static] fn name(params) [-> returnType]
    std::regex funcRegex(R"((?:pub\s+)?(?:static\s+)?fn\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\(([^)]*)\)\s*(?:->\s*([^\{]+))?)");
    std::smatch match;
    
    if (!std::regex_search(line, match, funcRegex)) return nullptr;
    
    auto sym = std::make_shared<Symbol>();
    sym->name = match[1].str();
    sym->kind = SymbolKind::Function;
    sym->isPublic = line.find("pub ") != std::string::npos;
    sym->isStatic = line.find("static ") != std::string::npos;
    
    sym->definition.uri = uri;
    sym->definition.range.start = {lineNum - 1, (int)line.find(sym->name)};
    sym->definition.range.end = {lineNum - 1, (int)(line.find(sym->name) + sym->name.size())};
    
    // Parse parameters
    std::string params = match[2].str();
    std::string sig = "(";
    
    if (!params.empty()) {
        std::regex paramRegex(R"(([a-zA-Z_][a-zA-Z0-9_]*)\s*:\s*([^,]+))");
        auto pBegin = std::sregex_iterator(params.begin(), params.end(), paramRegex);
        auto pEnd = std::sregex_iterator();
        
        bool first = true;
        for (auto pit = pBegin; pit != pEnd; ++pit) {
            std::string ptype = (*pit)[2].str();
            ptype.erase(0, ptype.find_first_not_of(" \t"));
            ptype.erase(ptype.find_last_not_of(" \t") + 1);
            sym->paramTypes.push_back(ptype);
            
            if (!first) sig += ", ";
            first = false;
            sig += (*pit)[1].str() + ": " + ptype;
        }
    }
    
    // Return type
    if (match[3].matched) {
        sym->returnType = match[3].str();
        sym->returnType.erase(0, sym->returnType.find_first_not_of(" \t"));
        sym->returnType.erase(sym->returnType.find_last_not_of(" \t") + 1);
    } else {
        sym->returnType = "void";
    }
    
    sig += ") -> " + sym->returnType;
    sym->detail = sig;
    
    return sym;
}

SymbolPtr SemanticAnalyzer::parseClass(const std::string& line, int lineNum, const std::string& uri) {
    std::regex classRegex(R"((?:pub\s+)?class\s+([a-zA-Z_][a-zA-Z0-9_]*))");
    std::smatch match;
    
    if (!std::regex_search(line, match, classRegex)) return nullptr;
    
    auto sym = std::make_shared<Symbol>();
    sym->name = match[1].str();
    sym->kind = SymbolKind::Class;
    sym->isPublic = line.find("pub ") != std::string::npos;
    
    sym->definition.uri = uri;
    sym->definition.range.start = {lineNum - 1, (int)line.find(sym->name)};
    sym->definition.range.end = {lineNum - 1, (int)(line.find(sym->name) + sym->name.size())};
    
    return sym;
}

SymbolPtr SemanticAnalyzer::parseVariable(const std::string& line, int lineNum, const std::string& uri) {
    // Match: let [mut] name [: type] = ...
    std::regex varRegex(R"(let\s+(?:mut\s+)?([a-zA-Z_][a-zA-Z0-9_]*)(?:\s*:\s*([^=]+))?)");
    std::smatch match;
    
    if (!std::regex_search(line, match, varRegex)) return nullptr;
    
    auto sym = std::make_shared<Symbol>();
    sym->name = match[1].str();
    sym->kind = SymbolKind::Variable;
    
    if (match[2].matched) {
        sym->type = match[2].str();
        sym->type.erase(0, sym->type.find_first_not_of(" \t"));
        sym->type.erase(sym->type.find_last_not_of(" \t") + 1);
    }
    
    sym->definition.uri = uri;
    sym->definition.range.start = {lineNum - 1, (int)line.find(sym->name)};
    sym->definition.range.end = {lineNum - 1, (int)(line.find(sym->name) + sym->name.size())};
    
    return sym;
}

SymbolPtr SemanticAnalyzer::parseEnum(const std::string& line, int lineNum, const std::string& uri) {
    std::regex enumRegex(R"((?:pub\s+)?enum\s+([a-zA-Z_][a-zA-Z0-9_]*))");
    std::smatch match;
    
    if (!std::regex_search(line, match, enumRegex)) return nullptr;
    
    auto sym = std::make_shared<Symbol>();
    sym->name = match[1].str();
    sym->kind = SymbolKind::Enum;
    sym->isPublic = line.find("pub ") != std::string::npos;
    
    sym->definition.uri = uri;
    sym->definition.range.start = {lineNum - 1, (int)line.find(sym->name)};
    sym->definition.range.end = {lineNum - 1, (int)(line.find(sym->name) + sym->name.size())};
    
    return sym;
}

std::vector<SymbolPtr> SemanticAnalyzer::getCallableSymbols(const std::string& uri) {
    std::vector<SymbolPtr> result;
    
    auto it = fileSymbols.find(uri);
    if (it != fileSymbols.end()) {
        for (const auto& sym : it->second) {
            if (sym->isCallable) {
                result.push_back(sym);
            }
        }
    }
    
    return result;
}

std::vector<SymbolPtr> SemanticAnalyzer::getVariablesInScope(const std::string& uri, Position pos) {
    std::vector<SymbolPtr> result;
    
    auto it = fileSymbols.find(uri);
    if (it != fileSymbols.end()) {
        for (const auto& sym : it->second) {
            if ((sym->kind == SymbolKind::Variable || sym->kind == SymbolKind::Parameter) &&
                sym->definition.range.start.line <= pos.line) {
                result.push_back(sym);
            }
        }
    }
    
    return result;
}

SymbolPtr SemanticAnalyzer::getSymbolAt(const std::string& uri, Position pos) {
    auto it = fileSymbols.find(uri);
    if (it == fileSymbols.end()) return nullptr;
    
    for (const auto& sym : it->second) {
        if (sym->definition.uri == uri) {
            auto& range = sym->definition.range;
            if (range.start.line == pos.line &&
                range.start.character <= pos.character &&
                pos.character <= range.end.character) {
                return sym;
            }
        }
    }
    
    return nullptr;
}

std::vector<SymbolPtr> SemanticAnalyzer::getAllSymbolsInFile(const std::string& uri) {
    auto it = fileSymbols.find(uri);
    if (it != fileSymbols.end()) {
        return it->second;
    }
    return {};
}

std::vector<std::string> SemanticAnalyzer::getImportedModules(const std::string& uri) {
    std::vector<std::string> modules;
    
    auto it = fileScopes.find(uri);
    if (it != fileScopes.end()) {
        Scope* scope = it->second.get();
        while (scope) {
            for (const auto& import : scope->imports) {
                modules.push_back(import.fullPath);
            }
            scope = scope->parent;
        }
    }
    
    return modules;
}

std::vector<SymbolPtr> SemanticAnalyzer::getSymbolsFromModule(const std::string& modulePath) {
    std::cerr << "[getSymbolsFromModule] Looking for: " << modulePath << std::endl;
    
    if (isStdLibModule(modulePath)) {
        return getStdLibSymbols(modulePath);
    }
    
    auto it = moduleSymbolCache.find(modulePath);
    if (it != moduleSymbolCache.end()) {
        return it->second;
    }
    
    std::cerr << "[getSymbolsFromModule] Searching " << fileSymbols.size() << " files" << std::endl;
    
    for (const auto& [uri, symbols] : fileSymbols) {
        std::string filename = uri;
        if (filename.find("file://") == 0) filename = filename.substr(7);
        
        size_t srcPos = filename.find("/src/");
        if (srcPos == std::string::npos) {
            std::cerr << "[getSymbolsFromModule] No /src/ in: " << filename << std::endl;
            continue;
        }
        
        std::string relativePath = filename.substr(srcPos + 5);
        
        if (relativePath.size() > 3 && relativePath.substr(relativePath.size() - 3) == ".mg") {
            relativePath = relativePath.substr(0, relativePath.size() - 3);
        }
        
        std::string fileModule;
        for (char c : relativePath) {
            fileModule += (c == '/' || c == '\\') ? '.' : c;
        }
        
        std::cerr << "[getSymbolsFromModule] File module: " << fileModule << " from " << uri << std::endl;
        
        bool matches = (modulePath == fileModule);
        if (!matches && modulePath.size() > fileModule.size()) {
            size_t offset = modulePath.size() - fileModule.size() - 1;
            if (modulePath[offset] == '.' && 
                modulePath.substr(offset + 1) == fileModule) {
                matches = true;
            }
        }
        
        std::cerr << "[getSymbolsFromModule] Comparing '" << modulePath << "' with '" << fileModule << "': " << (matches ? "MATCH" : "no match") << std::endl;
        
        if (matches) {
            std::vector<SymbolPtr> publicSymbols;
            for (const auto& sym : symbols) {
                if (sym->isPublic) {
                    std::cerr << "[getSymbolsFromModule] Found public symbol: " << sym->name << std::endl;
                    publicSymbols.push_back(sym);
                }
            }
            moduleSymbolCache[modulePath] = publicSymbols;
            return publicSymbols;
        }
    }
    
    std::cerr << "[getSymbolsFromModule] No match found for: " << modulePath << std::endl;
    return {};
}

std::vector<SymbolPtr> SemanticAnalyzer::resolveImportedSymbols(const std::string& uri) {
    std::vector<SymbolPtr> symbols;
    
    auto it = fileScopes.find(uri);
    if (it == fileScopes.end()) return symbols;
    
    Scope* scope = it->second.get();
    
    for (const auto& import : scope->imports) {
        auto moduleSymbols = getSymbolsFromModule(import.fullPath);
        for (const auto& sym : moduleSymbols) {
            if (sym->isPublic) {
                symbols.push_back(sym);
            }
        }
    }
    
    return symbols;
}

SymbolPtr SemanticAnalyzer::findSymbolInImports(const std::string& uri, const std::string& symbolName) {
    auto importedSymbols = resolveImportedSymbols(uri);
    
    for (const auto& sym : importedSymbols) {
        if (sym->name == symbolName) {
            return sym;
        }
    }
    
    return nullptr;
}

std::vector<SemanticAnalyzer::ImportError> SemanticAnalyzer::validateImports(const std::string& uri) {
    std::vector<ImportError> errors;
    
    auto it = fileScopes.find(uri);
    if (it == fileScopes.end()) return errors;
    
    for (const auto& import : it->second->imports) {
        if (import.fullPath.find("Std.") == 0 || import.fullPath == "Std") {
            if (!isStdLibModule(import.fullPath)) {
                ImportError error;
                error.modulePath = import.fullPath;
                error.message = "Unknown stdlib module: " + import.fullPath;
                errors.push_back(error);
            }
        }
    }
    
    return errors;
}

void SemanticAnalyzer::extractMetadataDirectives(const std::string& uri, const std::string& content, Scope* scope) {
    auto& cHeaderParser = CHeaderParser::instance();
    
    // Extract @cimport directives
    std::regex cimportBraceRegex(R"(@cimport\s*\{\s*([^}]+)\})");
    std::regex cimportQuoteRegex(R"(cimport\s*\"([^\"]+)\")");
    std::regex cimportAngleRegex(R"(cimport\s*<([^>]+)>)");
    
    std::smatch match;
    std::string::const_iterator searchStart = content.cbegin();
    
    // Match @cimport { header1.h header2.h }
    while (std::regex_search(searchStart, content.cend(), match, cimportBraceRegex)) {
        std::string headers = match[1].str();
        std::istringstream iss(headers);
        std::string header;
        while (iss >> header) {
            // Trim and clean header path
            size_t start = header.find_first_not_of(" \t\n\r<\"");
            size_t end = header.find_last_not_of(" \t\n\r>\"");
            if (start != std::string::npos && end != std::string::npos) {
                header = header.substr(start, end - start + 1);
            }
            
            if (!header.empty()) {
                std::cerr << "[SemanticAnalyzer] Processing cimport: " << header << std::endl;
                cHeaderParser.parseHeader(header);
                
                // Store in file's cimport list
                fileCImports[uri].push_back(header);
            }
        }
        searchStart = match.suffix().first;
    }
    
    // Match cimport "header.h"
    searchStart = content.cbegin();
    while (std::regex_search(searchStart, content.cend(), match, cimportQuoteRegex)) {
        std::string header = match[1].str();
        std::cerr << "[SemanticAnalyzer] Processing cimport: " << header << std::endl;
        cHeaderParser.parseHeader(header);
        fileCImports[uri].push_back(header);
        searchStart = match.suffix().first;
    }
    
    // Match cimport <header.h>
    searchStart = content.cbegin();
    while (std::regex_search(searchStart, content.cend(), match, cimportAngleRegex)) {
        std::string header = match[1].str();
        std::cerr << "[SemanticAnalyzer] Processing cimport: " << header << std::endl;
        cHeaderParser.parseHeader(header);
        fileCImports[uri].push_back(header);
        searchStart = match.suffix().first;
    }
}

std::vector<std::string> SemanticAnalyzer::getCImports(const std::string& uri) const {
    auto it = fileCImports.find(uri);
    if (it != fileCImports.end()) {
        return it->second;
    }
    return {};
}
