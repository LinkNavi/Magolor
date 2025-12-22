#pragma once
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <fstream>
#include <sstream>
#include "package_registry.hpp"

namespace fs = std::filesystem;

struct Dependency {
    std::string name;
    std::string version;
    std::string source;
    std::string location;
};

struct Package {
    std::string name;
    std::string version;
    std::string description;
    std::vector<std::string> authors;
    std::string license;
    std::map<std::string, std::string> dependencies;
    std::vector<std::string> sourceDirs;
};

class PackageManager {
public:
    static Package loadFromToml(const std::string& path) {
        Package pkg;
        std::ifstream file(path);
        if (!file) {
            throw std::runtime_error("Cannot open project.toml");
        }
        
        std::string line;
        std::string section;
        
        while (std::getline(file, line)) {
            // Skip comments and empty lines
            if (line.empty() || line[0] == '#') continue;
            
            // Trim whitespace
            line.erase(0, line.find_first_not_of(" \t"));
            line.erase(line.find_last_not_of(" \t") + 1);
            
            // Check for section headers
            if (line[0] == '[' && line.back() == ']') {
                section = line.substr(1, line.size() - 2);
                continue;
            }
            
            // Parse key = value
            size_t eq = line.find('=');
            if (eq == std::string::npos) continue;
            
            std::string key = line.substr(0, eq);
            std::string value = line.substr(eq + 1);
            
            // Trim
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            // Remove quotes
            if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.size() - 2);
            }
            
            // Assign values
            if (section.empty() || section == "project") {
                if (key == "name") pkg.name = value;
                else if (key == "version") pkg.version = value;
                else if (key == "description") pkg.description = value;
                else if (key == "license") pkg.license = value;
            } else if (section == "dependencies") {
                pkg.dependencies[key] = value;
            }
        }
        
        // Default source directories
        if (fs::exists("src")) {
            pkg.sourceDirs.push_back("src");
        }
        
        return pkg;
    }
    
    static std::vector<std::string> collectSourceFiles(const Package& pkg, 
                                                       const std::vector<ResolvedPackage>& deps = {}) {
        std::vector<std::string> sources;
        
        // Collect from main package
        for (const auto& dir : pkg.sourceDirs) {
            if (!fs::exists(dir)) continue;
            
            for (const auto& entry : fs::recursive_directory_iterator(dir)) {
                if (entry.is_regular_file() && entry.path().extension() == ".mg") {
                    sources.push_back(entry.path().string());
                }
            }
        }
        
        // Collect from dependencies
        for (const auto& dep : deps) {
            for (const auto& dir : dep.sourceDirs) {
                if (!fs::exists(dir)) continue;
                
                for (const auto& entry : fs::recursive_directory_iterator(dir)) {
                    if (entry.is_regular_file() && entry.path().extension() == ".mg") {
                        sources.push_back(entry.path().string());
                    }
                }
            }
        }
        
        return sources;
    }
    
    static DependencyResolver::ResolveResult installDependencies(Package& pkg) {
        std::cout << "\033[1;32m   Installing\033[0m dependencies for " << pkg.name << "\n";
        
        if (pkg.dependencies.empty()) {
            std::cout << "\033[1;32m    Finished\033[0m no dependencies to install\n";
            return {{}, true, ""};
        }
        
        fs::create_directories(".magolor/packages");
        
        DependencyResolver resolver;
        auto result = resolver.resolveAll(pkg.dependencies);
        
        if (!result.success) {
            std::cerr << "\033[1;31m       Error\033[0m: " << result.error << "\n";
            return result;
        }
        
        std::cout << "\033[1;32m    Finished\033[0m installed " << result.packages.size() << " packages\n";
        
        // Save lock file
        saveLockFile(pkg, result.packages);
        
        return result;
    }
    
    static std::vector<ResolvedPackage> loadFromLockFile() {
        std::vector<ResolvedPackage> packages;
        
        if (!fs::exists(".magolor/lock.toml")) {
            return packages;
        }
        
        std::ifstream file(".magolor/lock.toml");
        std::string line, section;
        ResolvedPackage* current = nullptr;
        
        while (std::getline(file, line)) {
            line.erase(0, line.find_first_not_of(" \t"));
            if (line.empty() || line[0] == '#') continue;
            
            if (line.find("[[package]]") == 0) {
                packages.push_back(ResolvedPackage());
                current = &packages.back();
                continue;
            }
            
            if (!current) continue;
            
            size_t eq = line.find('=');
            if (eq == std::string::npos) continue;
            
            std::string key = line.substr(0, eq);
            std::string value = line.substr(eq + 1);
            
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.size() - 2);
            }
            
            if (key == "name") current->name = value;
            else if (key == "version") current->version = PackageVersion::parse(value);
            else if (key == "location") current->location = value;
        }
        
        // Load source dirs for each package
        for (auto& pkg : packages) {
            if (fs::exists(pkg.location + "/src")) {
                pkg.sourceDirs.push_back(pkg.location + "/src");
            }
        }
        
        return packages;
    }
    
private:
    static void saveLockFile(const Package& pkg, const std::vector<ResolvedPackage>& packages) {
        std::ofstream file(".magolor/lock.toml");
        file << "# This file is automatically generated by Gear\n";
        file << "# Do not edit this file manually\n\n";
        file << "[root]\n";
        file << "name = \"" << pkg.name << "\"\n";
        file << "version = \"" << pkg.version << "\"\n\n";
        
        for (const auto& pkg : packages) {
            file << "[[package]]\n";
            file << "name = \"" << pkg.name << "\"\n";
            file << "version = \"" << pkg.version.toString() << "\"\n";
            file << "location = \"" << pkg.location << "\"\n\n";
        }
        
        std::cout << "\033[1;32m       Saved\033[0m lock file\n";
    }
};
