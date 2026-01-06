#include "lsp_semantic.hpp"
#include "jsonrpc.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
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
        "./stdlib",  // ← Make sure this path is correct!
        "/usr/local/share/magolor/stdlib",
        "/usr/share/magolor/stdlib",
        std::string(getenv("HOME") ? getenv("HOME") : "") + "/.magolor/stdlib"
    };
    
    std::string stdlibPath;
    for (const auto& path : searchPaths) {
        if (fs::exists(path)) {
            stdlibPath = path;
            std::cerr << "Found stdlib at: " << path << std::endl;  // ← Add logging
            break;
        }
    }
    
    if (!stdlibPath.empty()) {
        StdLibLoader::instance().init(stdlibPath);
    } else {
        std::cerr << "WARNING: stdlib not found!" << std::endl;  // ← Add warning
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
    auto it = stdlibSymbolCache.find(modulePath);
    if (it != stdlibSymbolCache.end()) {
        return it->second;
    }
    
    std::vector<SymbolPtr> symbols;
    
    auto functions = StdLibLoader::instance().getFunctions(modulePath);
    for (const auto& func : functions) {
        symbols.push_back(stdFunctionToSymbol(func, modulePath));
    }
    
    auto classes = StdLibLoader::instance().getClasses(modulePath);
    for (const auto& cls : classes) {
        auto sym = std::make_shared<Symbol>();
        sym->name = cls.name;
        sym->kind = SymbolKind::Class;
        sym->isPublic = true;
        sym->modulePath = modulePath;
        sym->documentation = cls.documentation;
        symbols.push_back(sym);
    }
    
    stdlibSymbolCache[modulePath] = symbols;
    return symbols;
}

SymbolPtr SemanticAnalyzer::stdFunctionToSymbol(const StdFunction& func, const std::string& modulePath) {
    auto sym = std::make_shared<Symbol>();
    sym->name = func.name;
    sym->kind = func.isConstant ? SymbolKind::Constant : SymbolKind::Function;
    sym->isPublic = true;
    sym->isCallable = !func.isConstant;
    sym->modulePath = modulePath;
    sym->detail = func.signature;
    sym->returnType = func.returnType;
    sym->paramTypes = func.paramTypes;
    sym->documentation = func.documentation;
    
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
    try {
        for (const auto& entry : fs::recursive_directory_iterator(srcDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".mg") {
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
    } catch (...) {}
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
    
    std::istringstream stream(content);
    std::string line;
    int lineNum = 0;
    std::string currentClass;
    
    while (std::getline(stream, line)) {
        lineNum++;
        
        size_t firstNonSpace = line.find_first_not_of(" \t");
        std::string trimmedLine = (firstNonSpace != std::string::npos) 
                                   ? line.substr(firstNonSpace) : "";
        
        if (trimmedLine.find("using ") == 0) {
            parseImport(line, scope.get());
        }
        else if (trimmedLine.find("class ") != std::string::npos ||
                 (trimmedLine.find("pub ") == 0 && trimmedLine.find("class ") != std::string::npos)) {
            auto sym = parseClass(line, lineNum, uri);
            if (sym) {
                sym->isCallable = false;
                symbols.push_back(sym);
                scope->define(sym);
                currentClass = sym->name;
            }
        }
        else if (trimmedLine.find("fn ") != std::string::npos) {
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
            auto sym = parseVariable(line, lineNum, uri);
            if (sym) {
                sym->containerName = currentClass;
                sym->isCallable = false;
                symbols.push_back(sym);
                scope->define(sym);
            }
        }
        
        if (!currentClass.empty() && trimmedLine == "}") {
            currentClass = "";
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
    
    importPath.erase(0, importPath.find_first_not_of(" \t"));
    importPath.erase(importPath.find_last_not_of(" \t") + 1);
    
    size_t pos = importPath.find("::");
    if (pos != std::string::npos) {
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
    size_t fnPos = line.find("fn ");
    if (fnPos == std::string::npos) return nullptr;
    
    size_t nameStart = fnPos + 3;
    size_t nameEnd = line.find('(', nameStart);
    if (nameEnd == std::string::npos) return nullptr;
    
    std::string name = line.substr(nameStart, nameEnd - nameStart);
    name.erase(0, name.find_first_not_of(" \t"));
    name.erase(name.find_last_not_of(" \t") + 1);
    
    if (name.empty()) return nullptr;
    
    auto sym = std::make_shared<Symbol>();
    sym->name = name;
    sym->kind = SymbolKind::Function;
    sym->definition.uri = uri;
    sym->definition.range.start = {lineNum - 1, (int)nameStart};
    sym->definition.range.end = {lineNum - 1, (int)nameEnd};
    sym->isPublic = line.find("pub ") != std::string::npos;
    sym->isStatic = line.find("static ") != std::string::npos;
    
    size_t parenEnd = line.find(')', nameEnd);
    size_t arrowPos = line.find("->", parenEnd != std::string::npos ? parenEnd : 0);
    
    if (parenEnd != std::string::npos) {
        std::string params = line.substr(nameEnd, parenEnd - nameEnd + 1);
        sym->detail = params;
        
        if (arrowPos != std::string::npos) {
            size_t typeStart = arrowPos + 2;
            size_t typeEnd = line.find_first_of(" {", typeStart);
            if (typeEnd == std::string::npos) typeEnd = line.size();
            sym->returnType = line.substr(typeStart, typeEnd - typeStart);
            sym->returnType.erase(0, sym->returnType.find_first_not_of(" \t"));
            sym->returnType.erase(sym->returnType.find_last_not_of(" \t") + 1);
            sym->detail += " -> " + sym->returnType;
        }
    }
    
    return sym;
}

SymbolPtr SemanticAnalyzer::parseClass(const std::string& line, int lineNum, const std::string& uri) {
    size_t classPos = line.find("class ");
    if (classPos == std::string::npos) return nullptr;
    
    size_t nameStart = classPos + 6;
    size_t nameEnd = line.find_first_of(" {<", nameStart);
    if (nameEnd == std::string::npos) return nullptr;
    
    std::string name = line.substr(nameStart, nameEnd - nameStart);
    name.erase(name.find_last_not_of(" \t") + 1);
    
    auto sym = std::make_shared<Symbol>();
    sym->name = name;
    sym->kind = SymbolKind::Class;
    sym->definition.uri = uri;
    sym->definition.range.start = {lineNum - 1, (int)nameStart};
    sym->definition.range.end = {lineNum - 1, (int)nameEnd};
    sym->isPublic = line.find("pub ") != std::string::npos;
    
    return sym;
}

SymbolPtr SemanticAnalyzer::parseVariable(const std::string& line, int lineNum, const std::string& uri) {
    size_t letPos = line.find("let ");
    if (letPos == std::string::npos) return nullptr;
    
    size_t nameStart = letPos + 4;
    
    if (line.find("mut ", nameStart) == nameStart) {
        nameStart += 4;
    }
    
    while (nameStart < line.size() && (line[nameStart] == ' ' || line[nameStart] == '\t')) {
        nameStart++;
    }
    
    size_t nameEnd = nameStart;
    while (nameEnd < line.size() && (std::isalnum(line[nameEnd]) || line[nameEnd] == '_')) {
        nameEnd++;
    }
    
    if (nameEnd == nameStart) return nullptr;
    
    std::string name = line.substr(nameStart, nameEnd - nameStart);
    
    auto sym = std::make_shared<Symbol>();
    sym->name = name;
    sym->kind = SymbolKind::Variable;
    sym->definition.uri = uri;
    sym->definition.range.start = {lineNum - 1, (int)nameStart};
    sym->definition.range.end = {lineNum - 1, (int)nameEnd};
    
    size_t colonPos = line.find(':', nameEnd);
    size_t equalPos = line.find('=', nameEnd);
    
    if (colonPos != std::string::npos && (equalPos == std::string::npos || colonPos < equalPos)) {
        size_t typeEnd = (equalPos != std::string::npos) ? equalPos : line.find(';', colonPos);
        if (typeEnd == std::string::npos) typeEnd = line.size();
        sym->type = line.substr(colonPos + 1, typeEnd - colonPos - 1);
        sym->type.erase(0, sym->type.find_first_not_of(" \t"));
        sym->type.erase(sym->type.find_last_not_of(" \t") + 1);
    }
    
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
    if (isStdLibModule(modulePath)) {
        return getStdLibSymbols(modulePath);
    }
    
    auto it = moduleSymbolCache.find(modulePath);
    if (it != moduleSymbolCache.end()) {
        return it->second;
    }
    
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


