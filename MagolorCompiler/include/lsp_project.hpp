#pragma once
#include "module.hpp"
#include "parser.hpp"
#include "lexer.hpp"
#include "typechecker.hpp"
#include <memory>
#include <unordered_map>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

struct ProjectContext {
    std::string rootPath;
    std::string projectName;
    std::unordered_map<std::string, ModulePtr> modules;
    ModuleRegistry& registry;
    
    ProjectContext() : registry(ModuleRegistry::instance()) {}
    
    // Load all modules in the project
    bool loadProject(const std::string& projectRoot);
    
    // Reload a single file
    bool reloadFile(const std::string& uri, const std::string& content);
    
    // Get module for a URI
    ModulePtr getModuleForUri(const std::string& uri);
    
    // Find all importable symbols from a module
    std::vector<std::string> getExportedSymbols(const std::string& modulePath);
    
    // Validate imports in a file
    std::vector<std::string> validateImports(const std::string& uri);
};

class ProjectManager {
public:
    static ProjectManager& instance() {
        static ProjectManager pm;
        return pm;
    }
    
    // Get or create project context for a file
    std::shared_ptr<ProjectContext> getProjectForFile(const std::string& uri);
    
    // Explicitly load a project
    void loadProject(const std::string& rootPath);
    
    // Clear all projects
    void clearAll();
    
private:
    std::unordered_map<std::string, std::shared_ptr<ProjectContext>> projects;
    
    std::string findProjectRoot(const std::string& filePath);
};
