#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdlib>
#include "lexer.hpp"
#include "parser.hpp"
#include "codegen.hpp"
#include "error.hpp"
#include "package.hpp"
#include "module.hpp"

namespace fs = std::filesystem;

std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("Cannot open file: " + path);
    std::stringstream buf;
    buf << f.rdbuf();
    return buf.str();
}

void writeFile(const std::string& path, const std::string& content) {
    std::ofstream f(path);
    if (!f) throw std::runtime_error("Cannot write file: " + path);
    f << content;
}

void printUsage() {
    std::cout << "\033[1mMagolor Compiler v0.1.0\033[0m\n\n";
    std::cout << "\033[1mUSAGE:\033[0m\n";
    std::cout << "    magolor [COMMAND] [OPTIONS]\n\n";
    std::cout << "\033[1mCOMMANDS:\033[0m\n";
    std::cout << "    build [file.mg]     Compile source file to executable\n";
    std::cout << "    build-project       Build entire project (uses project.toml)\n";
    std::cout << "    emit <file.mg>      Output generated C++ code\n";
    std::cout << "    run <file.mg>       Compile and run immediately\n";
    std::cout << "    check <file.mg>     Check for errors without building\n";
    std::cout << "    help                Show this help\n\n";
    std::cout << "\033[1mOPTIONS:\033[0m\n";
    std::cout << "    -o <file>          Specify output file name\n";
    std::cout << "    --verbose          Show detailed compilation steps\n";
}

Program compileFile(const std::string& filepath, const std::string& packageName, 
                   bool& hasErrors, bool verbose = false) {
    if (verbose) {
        std::cout << "\033[1;32mCompiling\033[0m " << filepath << "\n";
    }
    
    std::string source = readFile(filepath);
    ErrorReporter reporter(filepath, source);
    
    // Lex
    Lexer lexer(source, filepath, reporter);
    auto tokens = lexer.tokenize();
    
    if (reporter.hasError()) {
        reporter.printDiagnostics();
        hasErrors = true;
        return Program{};
    }
    
    // Parse
    Parser parser(std::move(tokens), filepath, reporter);
    Program prog = parser.parse();
    
    if (reporter.hasError()) {
        reporter.printDiagnostics();
        hasErrors = true;
        return Program{};
    }
    
    // Create module and register it
    auto module = std::make_shared<Module>();
    module->name = ModuleResolver::filePathToModuleName(filepath, packageName);
    module->filepath = filepath;
    module->packageName = packageName;
    module->ast = prog;
    
    // Mark all top-level functions as public by default
    for (auto& fn : module->ast.functions) {
        fn.isPublic = true;  // Default to public
    }
    
    ModuleRegistry::instance().registerModule(module);
    
    return prog;
}

Program mergePrograms(const std::vector<Program>& programs) {
    Program merged;
    
    for (const auto& prog : programs) {
        merged.usings.insert(merged.usings.end(), prog.usings.begin(), prog.usings.end());
        merged.classes.insert(merged.classes.end(), prog.classes.begin(), prog.classes.end());
        merged.functions.insert(merged.functions.end(), prog.functions.begin(), prog.functions.end());
    }
    
    return merged;
}

int buildProject(bool verbose = false) {
    try {
        if (!fs::exists("project.toml")) {
            std::cerr << "\033[1;31merror\033[0m: project.toml not found\n";
            std::cerr << "  \033[1;34m= help:\033[0m Initialize a project with 'gear init'\n";
            return 1;
        }
        
        Package pkg = PackageManager::loadFromToml("project.toml");
        
        if (verbose) {
            std::cout << "\033[1;32mBuilding\033[0m " << pkg.name << " v" << pkg.version << "\n";
        }
        
        // Clear module registry for fresh build
        ModuleRegistry::instance().clear();
        
        // Install/load dependencies
        std::vector<ResolvedPackage> deps;
        if (!pkg.dependencies.empty()) {
            deps = PackageManager::loadFromLockFile();
            if (deps.empty()) {
                auto result = PackageManager::installDependencies(pkg);
                if (!result.success) {
                    std::cerr << "\033[1;31merror\033[0m: failed to resolve dependencies\n";
                    return 1;
                }
                deps = result.packages;
            }
        }
        
        // Collect source files (including from dependencies)
        auto sourceFiles = PackageManager::collectSourceFiles(pkg, deps);
        
        if (sourceFiles.empty()) {
            std::cerr << "\033[1;31merror\033[0m: no source files found\n";
            std::cerr << "  \033[1;34m= help:\033[0m Add .mg files to the src/ directory\n";
            return 1;
        }
        
        // Compile each file and register modules
        std::vector<Program> programs;
        bool hasErrors = false;
        
        // First pass: compile all files and register modules
        for (const auto& file : sourceFiles) {
            // Determine package name from file path
            std::string pkgName = pkg.name;
            for (const auto& dep : deps) {
                if (file.find(dep.location) != std::string::npos) {
                    pkgName = dep.name;
                    break;
                }
            }
            
            auto prog = compileFile(file, pkgName, hasErrors, verbose);
            if (!hasErrors) {
                programs.push_back(prog);
            }
        }
        
        if (hasErrors) {
            std::cerr << "\033[1;31merror\033[0m: compilation failed\n";
            return 1;
        }
        
        // Second pass: resolve imports
        if (verbose) {
            std::cout << "\033[1;32m  Resolving\033[0m module imports...\n";
        }
        
        ImportResolver importResolver;
        for (const auto& [name, module] : ModuleRegistry::instance().getModules()) {
            if (verbose) {
                std::cout << "  Resolving imports for module: " << name << "\n";
            }
            auto result = importResolver.resolve(module);
            if (!result.success) {
                std::cerr << "\033[1;31merror\033[0m: " << result.error << "\n";
                return 1;
            }
            if (verbose && !module->imports.empty()) {
                std::cout << "    Imports: ";
                for (const auto& imp : module->imports) {
                    std::cout << imp << " ";
                }
                std::cout << "\n";
            }
        }
        
        // Third pass: resolve names
        if (verbose) {
            std::cout << "\033[1;32m  Resolving\033[0m names and symbols...\n";
        }
        
        NameResolver nameResolver;
        for (const auto& [name, module] : ModuleRegistry::instance().getModules()) {
            auto result = nameResolver.resolve(module);
            if (!result.success) {
                for (const auto& error : result.errors) {
                    std::cerr << "\033[1;31merror\033[0m: " << error << "\n";
                }
                return 1;
            }
        }
        
        // Merge all programs
        Program merged = mergePrograms(programs);
        
        // Generate C++
        CodeGen codegen;
        std::string cppCode = codegen.generate(merged);
        
        // Create target directory
        fs::create_directories("target");
        
        std::string cppPath = "target/" + pkg.name + ".cpp";
        std::string exePath = "target/" + pkg.name;
        
        writeFile(cppPath, cppCode);
        
        // Compile with g++
        if (verbose) {
            std::cout << "\033[1;32mCompiling\033[0m C++ code\n";
        }
        
        std::string compileCmd = "g++ -std=c++17 -O2 -o " + exePath + " " + cppPath + " 2>&1";
        FILE* pipe = popen(compileCmd.c_str(), "r");
        if (!pipe) {
            std::cerr << "\033[1;31merror\033[0m: failed to run g++\n";
            return 1;
        }
        
        char buffer[128];
        std::string result;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        int returnCode = pclose(pipe);
        
        if (returnCode != 0) {
            std::cerr << result;
            std::cerr << "\033[1;31nerror\033[0m: C++ compilation failed\n";
            return 1;
        }
        
        // Clean up intermediate file
        fs::remove(cppPath);
        
        std::cout << "\033[1;32m   Finished\033[0m release target(s) in 0.5s\n";
        std::cout << "    Binary: " << exePath << "\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\033[1;31merror\033[0m: " << e.what() << "\n";
        return 1;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage();
        return 0;
    }
    
    std::string cmd = argv[1];
    bool verbose = false;
    
    // Check for flags
    for (int i = 2; i < argc; i++) {
        if (std::string(argv[i]) == "--verbose") {
            verbose = true;
        }
    }
    
    if (cmd == "help" || cmd == "--help" || cmd == "-h") {
        printUsage();
        return 0;
    }
    
    if (cmd == "install-deps") {
        try {
            if (!fs::exists("project.toml")) {
                std::cerr << "\033[1;31merror\033[0m: project.toml not found\n";
                return 1;
            }
            
            Package pkg = PackageManager::loadFromToml("project.toml");
            auto result = PackageManager::installDependencies(pkg);
            
            if (!result.success) {
                return 1;
            }
            
            return 0;
        } catch (const std::exception& e) {
            std::cerr << "\033[1;31merror\033[0m: " << e.what() << "\n";
            return 1;
        }
    }
    
    if (cmd == "build-project" || cmd == "build") {
        if (argc == 2 || fs::exists("project.toml")) {
            return buildProject(verbose);
        }
    }
    
    if (argc < 3) {
        std::cerr << "\033[1;31merror\033[0m: missing source file\n";
        std::cerr << "  \033[1;34m= help:\033[0m use 'magolor build <file.mg>'\n";
        return 1;
    }
    
    std::string srcPath = argv[2];
    
    try {
        bool hasErrors = false;
        
        // Clear module registry
        ModuleRegistry::instance().clear();
        
        Program prog = compileFile(srcPath, "", hasErrors, verbose);
        
        if (hasErrors) {
            return 1;
        }
        
        // Generate C++
        CodeGen codegen;
        std::string cppCode = codegen.generate(prog);
        
        // Determine output paths
        fs::path srcFsPath(srcPath);
        std::string baseName = srcFsPath.stem().string();
        std::string cppPath = baseName + ".cpp";
        std::string exePath = baseName;
        
        if (cmd == "emit") {
            std::cout << cppCode;
            return 0;
        }
        
        if (cmd == "check") {
            std::cout << "\033[1;32m    Checking\033[0m " << srcPath << "\n";
            std::cout << "\033[1;32m    Finished\033[0m no errors found\n";
            return 0;
        }
        
        // Write C++ file
        writeFile(cppPath, cppCode);
        
        // Compile with g++
        if (verbose) {
            std::cout << "\033[1;32mCompiling\033[0m C++ code\n";
        }
        
        std::string compileCmd = "g++ -std=c++17 -O2 -o " + exePath + " " + cppPath + " 2>&1";
        FILE* pipe = popen(compileCmd.c_str(), "r");
        if (!pipe) {
            std::cerr << "\033[1;31merror\033[0m: failed to run g++\n";
            return 1;
        }
        
        char buffer[128];
        std::string result;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        int returnCode = pclose(pipe);
        
        if (returnCode != 0) {
            std::cerr << result;
            std::cerr << "\033[1;31merror\033[0m: C++ compilation failed\n";
            return 1;
        }
        
        // Clean up generated C++ file
        fs::remove(cppPath);
        
        if (cmd == "run") {
            if (verbose) {
                std::cout << "\033[1;32mRunning\033[0m " << exePath << "\n\n";
            }
            std::string runCmd = "./" + exePath;
            int result = std::system(runCmd.c_str());
            fs::remove(exePath);
            return result;
        }
        
        std::cout << "\033[1;32m   Finished\033[0m " << exePath << "\n";
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\033[1;31merror\033[0m: " << e.what() << "\n";
        return 1;
    }
}
