#include "lsp_project.hpp"
#include <fstream>
#include <sstream>

bool ProjectContext::loadProject(const std::string& projectRoot) {
    rootPath = projectRoot;
    registry.clear();
    modules.clear();
    
    // Find project.toml
    std::string projectToml = projectRoot + "/project.toml";
    if (!fs::exists(projectToml)) {
        return false;
    }
    
    // Parse project name
    std::ifstream toml(projectToml);
    std::string line;
    while (std::getline(toml, line)) {
        if (line.find("name =") != std::string::npos) {
            size_t start = line.find('"') + 1;
            size_t end = line.rfind('"');
            projectName = line.substr(start, end - start);
            break;
        }
    }
    
    // Find all .mg files in src/
    std::string srcDir = projectRoot + "/src";
    if (!fs::exists(srcDir)) {
        return false;
    }
    
    // Load all modules
    for (const auto& entry : fs::recursive_directory_iterator(srcDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".mg") {
            std::string filepath = entry.path().string();
            std::string relPath = fs::relative(filepath, projectRoot).string();
            
            try {
                std::ifstream file(filepath);
                std::stringstream buffer;
                buffer << file.rdbuf();
                std::string content = buffer.str();
                
                reloadFile("file://" + filepath, content);
            } catch (...) {
                continue;
            }
        }
    }
    
    // Resolve imports for all modules
    ImportResolver importResolver;
    for (auto& [uri, module] : modules) {
        importResolver.resolve(module);
    }
    
    return true;
}

bool ProjectContext::reloadFile(const std::string& uri, const std::string& content) {
    // Convert URI to file path
    std::string filepath = uri;
    if (filepath.find("file://") == 0) {
        filepath = filepath.substr(7);
    }
    
    // Create error reporter
    class SilentReporter : public ErrorReporter {
    public:
        SilentReporter() : ErrorReporter("", "") {}
        void error(const std::string&, SourceLocation, const std::string&) override {}
        void warning(const std::string&, SourceLocation) override {}
    };
    
    SilentReporter reporter;
    
    try {
        // Lex and parse
        Lexer lexer(content, filepath, reporter);
        auto tokens = lexer.tokenize();
        
        if (reporter.hasError()) {
            return false;
        }
        
        Parser parser(std::move(tokens), filepath, reporter);
        Program prog = parser.parse();
        
        if (reporter.hasError()) {
            return false;
        }
        
        // Create module
        auto module = std::make_shared<Module>();
        
        // Determine module name from file path
        std::string relPath = fs::relative(filepath, rootPath).string();
        module->name = ModuleResolver::filePathToModuleName(relPath, projectName);
        module->filepath = filepath;
        module->packageName = projectName;
        module->ast = prog;
        
        // Mark top-level functions as public by default
        for (auto& fn : module->ast.functions) {
            fn.isPublic = true;
        }
        
        // Register module
        registry.registerModule(module);
        modules[uri] = module;
        
        return true;
    } catch (...) {
        return false;
    }
}

ModulePtr ProjectContext::getModuleForUri(const std::string& uri) {
    auto it = modules.find(uri);
    if (it != modules.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::string> ProjectContext::getExportedSymbols(const std::string& modulePath) {
    std::vector<std::string> symbols;
    
    ModulePtr module = registry.getModule(modulePath);
    if (!module) {
        return symbols;
    }
    
    // Get public functions
    for (const auto& fn : module->ast.functions) {
        if (fn.isPublic) {
            symbols.push_back(fn.name);
        }
    }
    
    // Get public classes
    for (const auto& cls : module->ast.classes) {
        if (cls.isPublic) {
            symbols.push_back(cls.name);
        }
    }
    
    return symbols;
}

std::vector<std::string> ProjectContext::validateImports(const std::string& uri) {
    std::vector<std::string> errors;
    
    ModulePtr module = getModuleForUri(uri);
    if (!module) {
        return errors;
    }
    
    for (const auto& usingDecl : module->ast.usings) {
        std::string importPath;
        for (size_t i = 0; i < usingDecl.path.size(); i++) {
            if (i > 0) importPath += ".";
            importPath += usingDecl.path[i];
        }
        
        // Skip built-in modules
        if (ModuleResolver::isBuiltinModule(importPath)) {
            continue;
        }
        
        // Check if module exists
        ModulePtr importedModule = registry.getModule(importPath);
        if (!importedModule) {
            errors.push_back("Cannot find module: " + importPath);
        }
    }
    
    return errors;
}

// ProjectManager implementation

std::string ProjectManager::findProjectRoot(const std::string& filePath) {
    fs::path current = fs::path(filePath).parent_path();
    
    while (!current.empty() && current.has_parent_path()) {
        if (fs::exists(current / "project.toml")) {
            return current.string();
        }
        current = current.parent_path();
    }
    
    return "";
}

std::shared_ptr<ProjectContext> ProjectManager::getProjectForFile(const std::string& uri) {
    std::string filepath = uri;
    if (filepath.find("file://") == 0) {
        filepath = filepath.substr(7);
    }
    
    std::string projectRoot = findProjectRoot(filepath);
    if (projectRoot.empty()) {
        return nullptr;
    }
    
    // Check if already loaded
    auto it = projects.find(projectRoot);
    if (it != projects.end()) {
        return it->second;
    }
    
    // Load new project
    auto context = std::make_shared<ProjectContext>();
    if (context->loadProject(projectRoot)) {
        projects[projectRoot] = context;
        return context;
    }
    
    return nullptr;
}

void ProjectManager::loadProject(const std::string& rootPath) {
    auto context = std::make_shared<ProjectContext>();
    if (context->loadProject(rootPath)) {
        projects[rootPath] = context;
    }
}

void ProjectManager::clearAll() {
    projects.clear();
}
