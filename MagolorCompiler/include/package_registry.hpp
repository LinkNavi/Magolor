#pragma once
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>

namespace fs = std::filesystem;

enum class PackageSource {
    REGISTRY,   // Official Magolor registry
    GIT,        // Git repository
    LOCAL,      // Local path
    PATH        // System path
};

struct PackageVersion {
    int major;
    int minor;
    int patch;
    
    static PackageVersion parse(const std::string& ver) {
        PackageVersion v{0, 0, 0};
        if (ver == "*") return v;
        
        std::stringstream ss(ver);
        char dot;
        ss >> v.major;
        if (ss >> dot >> v.minor) {
            ss >> dot >> v.patch;
        }
        return v;
    }
    
    std::string toString() const {
        return std::to_string(major) + "." + 
               std::to_string(minor) + "." + 
               std::to_string(patch);
    }
    
    bool satisfies(const PackageVersion& required) const {
        if (required.major == 0) return true; // "*" matches any
        if (major != required.major) return false;
        if (minor < required.minor) return false;
        if (minor == required.minor && patch < required.patch) return false;
        return true;
    }
};

struct ResolvedPackage {
    std::string name;
    PackageVersion version;
    PackageSource source;
    std::string location;      // URL, path, or registry name
    std::vector<std::string> sourceDirs;
    std::map<std::string, std::string> dependencies;
};

class PackageRegistry {
public:
    static PackageRegistry& instance() {
        static PackageRegistry reg;
        return reg;
    }
    
    // Resolve a package dependency
    ResolvedPackage resolve(const std::string& name, const std::string& versionSpec) {
        std::cout << "\033[1;32m   Resolving\033[0m " << name << " " << versionSpec << "\n";
        
        // Check cache first
        std::string cacheKey = name + "@" + versionSpec;
        if (cache.find(cacheKey) != cache.end()) {
            std::cout << "\033[1;32m      Cached\033[0m " << name << "\n";
            return cache[cacheKey];
        }
        
        ResolvedPackage pkg;
        pkg.name = name;
        
        // Determine source type
        if (versionSpec.find("git+") == 0) {
            pkg.source = PackageSource::GIT;
            pkg.location = versionSpec.substr(4);
        } else if (versionSpec.find("path:") == 0) {
            pkg.source = PackageSource::LOCAL;
            pkg.location = versionSpec.substr(5);
        } else {
            pkg.source = PackageSource::REGISTRY;
            pkg.version = PackageVersion::parse(versionSpec);
        }
        
        cache[cacheKey] = pkg;
        return pkg;
    }
    
    // Fetch a package to local cache
    bool fetch(ResolvedPackage& pkg) {
        std::string pkgDir = getCacheDir() + "/" + pkg.name;
        
        switch (pkg.source) {
            case PackageSource::GIT:
                return fetchFromGit(pkg, pkgDir);
            case PackageSource::LOCAL:
                return fetchFromLocal(pkg, pkgDir);
            case PackageSource::REGISTRY:
                return fetchFromRegistry(pkg, pkgDir);
            case PackageSource::PATH:
                pkg.location = pkgDir;
                return true;
        }
        return false;
    }
    
    std::string getCacheDir() {
        return ".magolor/packages";
    }
    
private:
    std::map<std::string, ResolvedPackage> cache;
    
    PackageRegistry() {
        fs::create_directories(getCacheDir());
    }
    
    bool fetchFromGit(ResolvedPackage& pkg, const std::string& targetDir) {
        std::cout << "\033[1;32m  Downloading\033[0m " << pkg.name << " from git\n";
        
        if (fs::exists(targetDir)) {
            std::cout << "\033[1;33m     Updating\033[0m " << pkg.name << "\n";
            std::string cmd = "cd " + targetDir + " && git pull --quiet";
            if (std::system(cmd.c_str()) != 0) {
                std::cerr << "\033[1;31m       Error\033[0m: failed to update " << pkg.name << "\n";
                return false;
            }
        } else {
            std::string cmd = "git clone --quiet " + pkg.location + " " + targetDir;
            if (std::system(cmd.c_str()) != 0) {
                std::cerr << "\033[1;31m       Error\033[0m: failed to clone " << pkg.name << "\n";
                return false;
            }
        }
        
        pkg.location = targetDir;
        
        // Load package info
        if (fs::exists(targetDir + "/project.toml")) {
            loadPackageInfo(pkg, targetDir + "/project.toml");
        }
        
        // Add source directories
        if (fs::exists(targetDir + "/src")) {
            pkg.sourceDirs.push_back(targetDir + "/src");
        }
        
        std::cout << "\033[1;32m   Downloaded\033[0m " << pkg.name << "\n";
        return true;
    }
    
    bool fetchFromLocal(ResolvedPackage& pkg, const std::string& targetDir) {
        std::cout << "\033[1;32m     Linking\033[0m " << pkg.name << " from local path\n";
        
        if (!fs::exists(pkg.location)) {
            std::cerr << "\033[1;31m       Error\033[0m: path does not exist: " << pkg.location << "\n";
            return false;
        }
        
        // Create symlink
        if (fs::exists(targetDir)) {
            fs::remove_all(targetDir);
        }
        
        fs::create_directory_symlink(fs::absolute(pkg.location), targetDir);
        pkg.location = targetDir;
        
        // Load package info
        if (fs::exists(pkg.location + "/project.toml")) {
            loadPackageInfo(pkg, pkg.location + "/project.toml");
        }
        
        if (fs::exists(pkg.location + "/src")) {
            pkg.sourceDirs.push_back(pkg.location + "/src");
        }
        
        std::cout << "\033[1;32m      Linked\033[0m " << pkg.name << "\n";
        return true;
    }
    
    bool fetchFromRegistry(ResolvedPackage& pkg, const std::string& targetDir) {
        std::cout << "\033[1;32m    Fetching\033[0m " << pkg.name << " v" << pkg.version.toString() << "\n";
        
        // Check official registry first
        std::string registryUrl = getRegistryUrl() + "/" + pkg.name;
        
        // For now, fall back to common git repos
        std::string commonRepos[] = {
            "https://github.com/magolor-lang/" + pkg.name,
            "https://github.com/magolor/" + pkg.name,
        };
        
        for (const auto& repo : commonRepos) {
            std::string testCmd = "git ls-remote " + repo + " > /dev/null 2>&1";
            if (std::system(testCmd.c_str()) == 0) {
                pkg.location = repo;
                pkg.source = PackageSource::GIT;
                return fetchFromGit(pkg, targetDir);
            }
        }
        
        std::cerr << "\033[1;31m       Error\033[0m: package '" << pkg.name << "' not found in registry\n";
        std::cerr << "  \033[1;34m= help:\033[0m check package name or use git+https://... for custom repos\n";
        return false;
    }
    
    void loadPackageInfo(ResolvedPackage& pkg, const std::string& tomlPath) {
        std::ifstream file(tomlPath);
        if (!file) return;
        
        std::string line, section;
        while (std::getline(file, line)) {
            line.erase(0, line.find_first_not_of(" \t"));
            if (line.empty() || line[0] == '#') continue;
            
            if (line[0] == '[' && line.back() == ']') {
                section = line.substr(1, line.size() - 2);
                continue;
            }
            
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
            
            if (section.empty() || section == "project") {
                if (key == "version") {
                    pkg.version = PackageVersion::parse(value);
                }
            } else if (section == "dependencies") {
                pkg.dependencies[key] = value;
            }
        }
    }
    
    std::string getRegistryUrl() {
        // Could be configurable via environment variable or config file
        const char* envUrl = std::getenv("MAGOLOR_REGISTRY");
        if (envUrl) return envUrl;
        return "https://registry.magolor-lang.org";
    }
};

class DependencyResolver {
public:
    struct ResolveResult {
        std::vector<ResolvedPackage> packages;
        bool success;
        std::string error;
    };
    
    ResolveResult resolveAll(const std::map<std::string, std::string>& dependencies) {
        ResolveResult result;
        result.success = true;
        
        std::cout << "\033[1;32m   Resolving\033[0m dependencies...\n";
        
        for (const auto& [name, version] : dependencies) {
            if (!resolveDependency(name, version, result)) {
                result.success = false;
                return result;
            }
        }
        
        std::cout << "\033[1;32m    Resolved\033[0m " << result.packages.size() << " packages\n";
        return result;
    }
    
private:
    std::map<std::string, ResolvedPackage> resolved;
    
    bool resolveDependency(const std::string& name, const std::string& version, ResolveResult& result) {
        // Check if already resolved
        std::string key = name + "@" + version;
        if (resolved.find(key) != resolved.end()) {
            return true;
        }
        
        // Resolve this package
        auto pkg = PackageRegistry::instance().resolve(name, version);
        
        // Fetch the package
        if (!PackageRegistry::instance().fetch(pkg)) {
            result.error = "Failed to fetch package: " + name;
            return false;
        }
        
        // Recursively resolve dependencies
        for (const auto& [depName, depVersion] : pkg.dependencies) {
            if (!resolveDependency(depName, depVersion, result)) {
                return false;
            }
        }
        
        resolved[key] = pkg;
        result.packages.push_back(pkg);
        return true;
    }
};
