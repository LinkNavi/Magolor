#pragma once
#include <string>
#include <vector>
#include <map>
#include <set>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
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
    // Helper: trim whitespace
    static inline std::string trim(const std::string &s) {
        size_t a = s.find_first_not_of(" \t\r\n");
        if (a == std::string::npos) return "";
        size_t b = s.find_last_not_of(" \t\r\n");
        return s.substr(a, b - a + 1);
    }

    // Helper: remove surrounding quotes if present
    static inline std::string unquote(const std::string &s) {
        std::string t = trim(s);
        if (t.size() >= 2 && ((t.front() == '"' && t.back() == '"') || (t.front() == '\'' && t.back() == '\''))) {
            return t.substr(1, t.size() - 2);
        }
        return t;
    }

    // Loads basic fields and dependencies from project.toml
    static Package loadFromToml(const std::string& path) {
        Package pkg;
        std::ifstream file(path);
        if (!file) {
            throw std::runtime_error("Cannot open project.toml");
        }

        std::string line;
        std::string section;

        while (std::getline(file, line)) {
            line = trim(line);
            if (line.empty()) continue;
            if (line[0] == '#') continue;

            // Section header
            if (line.front() == '[' && line.back() == ']') {
                section = trim(line.substr(1, line.size() - 2));
                continue;
            }

            // key = value
            size_t eq = line.find('=');
            if (eq == std::string::npos) continue;

            std::string key = trim(line.substr(0, eq));
            std::string value = trim(line.substr(eq + 1));

            // Handle array for authors: authors = ["Name <email>"]
            if ((section.empty() || section == "project") && key == "authors") {
                // naive array parse: split on commas while removing brackets
                size_t start = value.find('[');
                size_t end = value.rfind(']');
                if (start != std::string::npos && end != std::string::npos && end > start) {
                    std::string inner = value.substr(start + 1, end - start - 1);
                    std::stringstream ss(inner);
                    while (std::getline(ss, line, ',')) {
                        std::string entry = unquote(trim(line));
                        if (!entry.empty()) pkg.authors.push_back(entry);
                    }
                } else {
                    // fallback single value
                    pkg.authors.push_back(unquote(value));
                }
                continue;
            }

            // Remove quotes if present
            std::string v = unquote(value);

            if (section.empty() || section == "project") {
                if (key == "name") pkg.name = v;
                else if (key == "version") pkg.version = v;
                else if (key == "description") pkg.description = v;
                else if (key == "license") pkg.license = v;
            } else if (section == "dependencies") {
                // dependency lines like: pkg = "1.0.0" or pkg = { version = "1.0" }
                // We keep it simple and store the right hand side as string
                pkg.dependencies[key] = v;
            }
        }

        // Default source directories: prefer project-relative src/
        fs::path projectDir = fs::path(path).parent_path();
        fs::path srcRel = projectDir / "src";
        if (fs::exists(srcRel) && fs::is_directory(srcRel)) {
            pkg.sourceDirs.push_back(srcRel.string());
        } else if (fs::exists("src") && fs::is_directory("src")) {
            // fallback to cwd/src
            pkg.sourceDirs.push_back(fs::absolute("src").string());
        }

        return pkg;
    }

    // Returns true if `file` lives under one of the package's sourceDirs
    static bool isAppSource(const std::string& file, const Package& pkg) {
        try {
            fs::path fp = fs::absolute(file);
            for (const auto& dir : pkg.sourceDirs) {
                fs::path dp = fs::absolute(dir);
                // Ensure dp ends with separator for strict prefix check
                std::string dpstr = dp.lexically_normal().string();
                std::string fpstr = fp.lexically_normal().string();

                // Make sure path comparison is platform-consistent
                if (fpstr.size() >= dpstr.size() && fpstr.compare(0, dpstr.size(), dpstr) == 0) {
                    // either equal or has separator after prefix
                    if (fpstr.size() == dpstr.size() || fpstr[dpstr.size()] == fs::path::preferred_separator) {
                        return true;
                    }
                }
            }
        } catch (...) {
            // If any path operation fails, conservatively return false
        }
        return false;
    }

    // Collect source files from project and dependencies.
    // Deduplicated and deterministic (sorted).
    static std::vector<std::string> collectSourceFiles(const Package& pkg,
                                                       const std::vector<ResolvedPackage>& deps = {}) {
        std::set<std::string> unique;
        std::vector<std::string> ordered;

        auto push_if_new = [&](const std::string &p) {
            try {
                std::string ap = fs::absolute(p).lexically_normal().string();
                if (unique.insert(ap).second) {
                    ordered.push_back(ap);
                }
            } catch (...) {
                // fallback to raw path if canonicalization fails
                if (unique.insert(p).second) ordered.push_back(p);
            }
        };

        // Collect from main package source dirs
        for (const auto& dir : pkg.sourceDirs) {
            if (!fs::exists(dir)) continue;
            for (const auto& entry : fs::recursive_directory_iterator(dir)) {
                if (!entry.is_regular_file()) continue;
                if (entry.path().extension() == ".mg") {
                    push_if_new(entry.path().string());
                }
            }
        }

        // Collect from dependencies (keep dependency files after app files)
        for (const auto& dep : deps) {
            for (const auto& dir : dep.sourceDirs) {
                if (!fs::exists(dir)) continue;
                for (const auto& entry : fs::recursive_directory_iterator(dir)) {
                    if (!entry.is_regular_file()) continue;
                    if (entry.path().extension() == ".mg") {
                        push_if_new(entry.path().string());
                    }
                }
            }
        }

        // deterministic ordering (already in traversal order, but ensure stable sort)
        std::sort(ordered.begin(), ordered.end());
        return ordered;
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
        std::string line;
        ResolvedPackage currentPkg;

        bool inPackage = false;
        while (std::getline(file, line)) {
            line = trim(line);
            if (line.empty() || line[0] == '#') continue;

            if (line == "[[package]]") {
                if (inPackage) {
                    packages.push_back(currentPkg);
                    currentPkg = ResolvedPackage();
                }
                inPackage = true;
                continue;
            }

            if (!inPackage) continue;

            size_t eq = line.find('=');
            if (eq == std::string::npos) continue;

            std::string key = trim(line.substr(0, eq));
            std::string value = unquote(trim(line.substr(eq + 1)));

            if (key == "name") currentPkg.name = value;
            else if (key == "version") currentPkg.version = PackageVersion::parse(value);
            else if (key == "location") currentPkg.location = value;
        }

        if (inPackage) {
            packages.push_back(currentPkg);
        }

        // Load source dirs for each package (if they have src/)
        for (auto& pkg : packages) {
            fs::path candidate = fs::path(pkg.location) / "src";
            if (fs::exists(candidate) && fs::is_directory(candidate)) {
                pkg.sourceDirs.push_back(candidate.string());
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

        for (const auto& p : packages) {
            file << "[[package]]\n";
            file << "name = \"" << p.name << "\"\n";
            file << "version = \"" << p.version.toString() << "\"\n";
            file << "location = \"" << p.location << "\"\n\n";
        }

        std::cout << "\033[1;32m       Saved\033[0m lock file\n";
    }
};
