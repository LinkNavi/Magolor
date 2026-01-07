// Simplified main.cpp - Single file compiler with build-project support
#include "codegen.hpp"
#include "error.hpp"
#include "lexer.hpp"
#include "lsp_server.hpp"
#include "module.hpp"
#include "package.hpp"
#include "parser.hpp"
#include "typechecker.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>

namespace fs = std::filesystem;

// ============================================================================
// Color output helpers
// ============================================================================
namespace Color {
    const char* RESET = "\033[0m";
    const char* BOLD = "\033[1m";
    const char* RED = "\033[1;31m";
    const char* GREEN = "\033[1;32m";
    const char* YELLOW = "\033[1;33m";
    const char* BLUE = "\033[1;34m";
    const char* MAGENTA = "\033[1;35m";
    const char* CYAN = "\033[1;36m";
    const char* DIM = "\033[2m";
}

// ============================================================================
// Utilities
// ============================================================================
std::string readFile(const std::string &path) {
    std::ifstream f(path);
    if (!f)
        throw std::runtime_error("Cannot open file: " + path);
    std::stringstream buf;
    buf << f.rdbuf();
    return buf.str();
}

std::string formatSize(size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB"};
    int unit = 0;
    double size = bytes;
    
    while (size >= 1024 && unit < 3) {
        size /= 1024;
        unit++;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << size << " " << units[unit];
    return oss.str();
}

std::string formatTime(long long ms) {
    if (ms < 1000) {
        return std::to_string(ms) + "ms";
    } else if (ms < 60000) {
        return std::to_string(ms / 1000) + "." + std::to_string((ms % 1000) / 100) + "s";
    } else {
        long long sec = ms / 1000;
        return std::to_string(sec / 60) + "m " + std::to_string(sec % 60) + "s";
    }
}

// ============================================================================
// Usage and help
// ============================================================================
void printUsage() {
    std::cout << Color::BOLD << "Magolor Compiler v0.3.0" << Color::RESET << " (C++ Backend)\n\n";
    
    std::cout << Color::BOLD << "USAGE:" << Color::RESET << "\n";
    std::cout << "    magolor [COMMAND] [OPTIONS] <file.mg>\n\n";
    
    std::cout << Color::BOLD << "COMMANDS:" << Color::RESET << "\n";
    std::cout << "    " << Color::CYAN << "build" << Color::RESET << " <file.mg>        Compile source file to executable\n";
    std::cout << "    " << Color::CYAN << "run" << Color::RESET << " <file.mg>          Compile and run immediately\n";
    std::cout << "    " << Color::CYAN << "check" << Color::RESET << " <file.mg>        Type check without building\n";
    std::cout << "    " << Color::CYAN << "emit-cpp" << Color::RESET << " <file.mg>     Output generated C++ code\n";
    std::cout << "    " << Color::CYAN << "build-project" << Color::RESET << "         Build multi-file project (for gear)\n";
    std::cout << "    " << Color::CYAN << "clean" << Color::RESET << "                 Clean build artifacts\n";
    std::cout << "    " << Color::CYAN << "lsp" << Color::RESET << "                   Start Language Server\n";
    std::cout << "    " << Color::CYAN << "version" << Color::RESET << "               Show version info\n";
    std::cout << "    " << Color::CYAN << "help" << Color::RESET << "                  Show this help\n\n";
    
    std::cout << Color::BOLD << "OPTIONS:" << Color::RESET << "\n";
    std::cout << "    " << Color::GREEN << "-o" << Color::RESET << " <file>           Specify output file name\n";
    std::cout << "    " << Color::GREEN << "--verbose" << Color::RESET << ", " << Color::GREEN << "-v" << Color::RESET << "      Show detailed compilation steps\n";
    std::cout << "    " << Color::GREEN << "--debug" << Color::RESET << "             Compile with debug symbols (O0)\n";
    std::cout << "    " << Color::GREEN << "--release" << Color::RESET << "           Maximum optimization (O3)\n";
    std::cout << "    " << Color::GREEN << "--emit-cpp" << Color::RESET << "          Also save generated C++ file\n";
    std::cout << "    " << Color::GREEN << "--no-color" << Color::RESET << "          Disable colored output\n";
    std::cout << "    " << Color::GREEN << "--timing" << Color::RESET << "            Show compilation timing breakdown\n";
    std::cout << "    " << Color::GREEN << "--opt-level" << Color::RESET << " <0-3>   Set optimization level (default: 1)\n\n";
    
    std::cout << Color::BOLD << "OPTIMIZATION LEVELS:" << Color::RESET << "\n";
    std::cout << "    0 = No optimization (fastest compile)\n";
    std::cout << "    1 = Basic optimization (default, good balance)\n";
    std::cout << "    2 = More optimization (slower compile)\n";
    std::cout << "    3 = Maximum optimization (slowest compile)\n\n";
    
    std::cout << Color::BOLD << "EXAMPLES:" << Color::RESET << "\n";
    std::cout << "    magolor build hello.mg              # Fast compile with -O1\n";
    std::cout << "    magolor build hello.mg --release    # Slow compile with -O3\n";
    std::cout << "    magolor run hello.mg                # Quick run\n";
    std::cout << "    magolor check src/main.mg           # Type check only\n";
    std::cout << "    magolor emit-cpp hello.mg           # Generate C++ code\n\n";
    
    std::cout << Color::BOLD << "NOTE:" << Color::RESET << "\n";
    std::cout << "    For multi-file projects and package management, use " << Color::CYAN << "gear" << Color::RESET << "\n";
    std::cout << "    magolor build-project is kept for gear's internal use\n\n";
}

void printVersion() {
    std::cout << Color::BOLD << "Magolor Compiler v0.3.0" << Color::RESET << "\n";
    std::cout << "Build: " << __DATE__ << " " << __TIME__ << "\n";
    std::cout << "Backend: C++ (no runtime)\n";
    std::cout << "Platform: ";
    #if defined(__APPLE__)
    std::cout << "macOS";
    #elif defined(__linux__)
    std::cout << "Linux";
    #elif defined(_WIN32)
    std::cout << "Windows";
    #else
    std::cout << "Unknown";
    #endif
    std::cout << "\n";
}

// ============================================================================
// Compilation statistics
// ============================================================================
struct CompileStats {
    long long lexTime = 0;
    long long parseTime = 0;
    long long typeCheckTime = 0;
    long long codegenTime = 0;
    long long cppCompileTime = 0;
    long long totalTime = 0;
    int lineCount = 0;
    size_t outputSize = 0;
    
    void print() const {
        std::cout << "\n" << Color::BOLD << "Compilation Statistics:" << Color::RESET << "\n";
        std::cout << "  Lines:        " << lineCount << "\n";
        std::cout << "  Lex:          " << formatTime(lexTime) << "\n";
        std::cout << "  Parse:        " << formatTime(parseTime) << "\n";
        std::cout << "  Type check:   " << formatTime(typeCheckTime) << "\n";
        std::cout << "  Code gen:     " << formatTime(codegenTime) << "\n";
        std::cout << "  C++ compile:  " << formatTime(cppCompileTime) << "\n";
        std::cout << "  " << Color::BOLD << "Total:        " << formatTime(totalTime) << Color::RESET << "\n";
        if (outputSize > 0) {
            std::cout << "  Binary size:  " << formatSize(outputSize) << "\n";
        }
    }
};

// ============================================================================
// Single file compilation
// ============================================================================
Program compileSingleFile(const std::string &filepath, CompileStats &stats, bool verbose = false) {
    if (verbose) {
        std::cout << Color::GREEN << "Compiling" << Color::RESET << " " << filepath << "\n";
    }
    
    std::string source = readFile(filepath);
    stats.lineCount = std::count(source.begin(), source.end(), '\n');
    
    ErrorReporter reporter(filepath, source);
    auto start = std::chrono::high_resolution_clock::now();
    
    // Lex
    Lexer lexer(source, filepath, reporter);
    auto tokens = lexer.tokenize();
    auto lexEnd = std::chrono::high_resolution_clock::now();
    stats.lexTime = std::chrono::duration_cast<std::chrono::milliseconds>(lexEnd - start).count();
    
    if (reporter.hasError()) {
        reporter.printDiagnostics();
        return Program{};
    }
    
    // Parse
    Parser parser(std::move(tokens), filepath, reporter);
    Program prog = parser.parse();
    auto parseEnd = std::chrono::high_resolution_clock::now();
    stats.parseTime = std::chrono::duration_cast<std::chrono::milliseconds>(parseEnd - lexEnd).count();
    
    if (reporter.hasError()) {
        reporter.printDiagnostics();
        return Program{};
    }
    
    return prog;
}

bool compileWithCpp(const Program& prog, const std::string& outputFile, 
                    CompileStats &stats, bool verbose, bool debug, bool emitCpp, int optLevel = 1) {
    auto start = std::chrono::high_resolution_clock::now();
    
    if (verbose) {
        std::cout << Color::GREEN << "   Generating" << Color::RESET << " C++ code\n";
    }
    
    // Generate C++ code
    CodeGen codegen;
    std::string cppCode = codegen.generate(prog);
    auto codegenEnd = std::chrono::high_resolution_clock::now();
    stats.codegenTime = std::chrono::duration_cast<std::chrono::milliseconds>(codegenEnd - start).count();
    
    // Write to temporary C++ file
    std::string cppFile = outputFile + ".cpp";
    std::ofstream out(cppFile);
    if (!out) {
        std::cerr << Color::RED << "error" << Color::RESET << ": Failed to write C++ file\n";
        return false;
    }
    out << cppCode;
    out.close();
    
    if (emitCpp) {
        std::string savedCpp = outputFile + "_gen.cpp";
        fs::copy(cppFile, savedCpp, fs::copy_options::overwrite_existing);
        if (verbose) {
            std::cout << Color::GREEN << "       Saved" << Color::RESET << " " << savedCpp << "\n";
        }
    }
    
    if (verbose) {
        std::cout << Color::GREEN << "    Compiling" << Color::RESET << " to native code (O" << optLevel << ")\n";
    }
    
    // Optimization flags
    std::string optFlags;
    if (debug) {
        optFlags = "-O0 -g -DDEBUG";
    } else {
        switch (optLevel) {
            case 0:
                optFlags = "-O0 -DNDEBUG";
                break;
            case 1:
                optFlags = "-O1 -DNDEBUG";
                break;
            case 2:
                optFlags = "-O2 -DNDEBUG";
                break;
            case 3:
                optFlags = "-O3 -march=native -mtune=native -flto -ffast-math "
                           "-funroll-loops -finline-functions -DNDEBUG";
                break;
            default:
                optFlags = "-O1 -DNDEBUG";
        }
    }
    
    // Compile command
    std::string compileCmd = "g++ -std=c++17 " + optFlags + " " + 
                            cppFile + " -o " + outputFile + " -lm -lpthread 2>&1";
    
    if (verbose) {
        std::cout << Color::CYAN << "    Command:" << Color::RESET << " " << compileCmd << "\n";
    }
    
    auto cppStart = std::chrono::high_resolution_clock::now();
    FILE* pipe = popen(compileCmd.c_str(), "r");
    if (!pipe) {
        std::cerr << Color::RED << "error" << Color::RESET << ": Failed to run compiler\n";
        return false;
    }
    
    char buffer[256];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    int returnCode = pclose(pipe);
    auto cppEnd = std::chrono::high_resolution_clock::now();
    stats.cppCompileTime = std::chrono::duration_cast<std::chrono::milliseconds>(cppEnd - cppStart).count();
    
    if (returnCode != 0) {
        std::cerr << result;
        std::cerr << Color::RED << "error" << Color::RESET << ": Compilation failed\n";
        return false;
    }
    
    // Clean up intermediate files unless emitCpp is set
    if (!emitCpp) {
        fs::remove(cppFile);
    }
    
    // Get binary size
    if (fs::exists(outputFile)) {
        stats.outputSize = fs::file_size(outputFile);
        if (verbose) {
            std::cout << Color::GREEN << "   Finished" << Color::RESET << " " << outputFile 
                     << " (" << formatSize(stats.outputSize) << ")\n";
        }
    }
    
    return true;
}

// ============================================================================
// Project management (kept for gear compatibility)
// ============================================================================
Program compileFile(const std::string &filepath, const std::string &packageName,
                    bool &hasErrors, CompileStats &stats, bool verbose = false) {
    if (verbose) {
        std::cout << Color::GREEN << "Compiling" << Color::RESET << " " << filepath << "\n";
    }
    
    std::string source = readFile(filepath);
    stats.lineCount += std::count(source.begin(), source.end(), '\n');
    
    ErrorReporter reporter(filepath, source);
    auto start = std::chrono::high_resolution_clock::now();
    
    // Lex
    Lexer lexer(source, filepath, reporter);
    auto tokens = lexer.tokenize();
    auto lexEnd = std::chrono::high_resolution_clock::now();
    stats.lexTime += std::chrono::duration_cast<std::chrono::milliseconds>(lexEnd - start).count();
    
    if (reporter.hasError()) {
        reporter.printDiagnostics();
        hasErrors = true;
        return Program{};
    }
    
    // Parse
    Parser parser(std::move(tokens), filepath, reporter);
    Program prog = parser.parse();
    auto parseEnd = std::chrono::high_resolution_clock::now();
    stats.parseTime += std::chrono::duration_cast<std::chrono::milliseconds>(parseEnd - lexEnd).count();
    
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
    module->buildSymbolTable();
    ModuleRegistry::instance().registerModule(module);
    
    return prog;
}

Program mergePrograms(const std::vector<Program> &programs) {
    Program merged;
    for (const auto &prog : programs) {
        merged.usings.insert(merged.usings.end(), prog.usings.begin(), prog.usings.end());
        merged.cppHeaders.insert(merged.cppHeaders.end(), prog.cppHeaders.begin(), prog.cppHeaders.end());
        merged.cimports.insert(merged.cimports.end(), prog.cimports.begin(), prog.cimports.end());
        merged.classes.insert(merged.classes.end(), prog.classes.begin(), prog.classes.end());
        merged.functions.insert(merged.functions.end(), prog.functions.begin(), prog.functions.end());
    }
    return merged;
}

bool cleanProject() {
    if (fs::exists("target")) {
        fs::remove_all("target");
        std::cout << Color::GREEN << "   Cleaned" << Color::RESET << " build artifacts\n";
        return true;
    }
    return false;
}


bool compileWithCppDebug(const Program& prog, const std::string& outputFile, 
                    CompileStats &stats, bool verbose, bool debug, bool emitCpp, int optLevel = 1) {
    auto start = std::chrono::high_resolution_clock::now();
    
    std::cout << Color::CYAN << "[DEBUG]" << Color::RESET << " Starting C++ code generation...\n";
    
    // Generate C++ code
    CodeGen codegen;
    std::string cppCode;
    
    try {
        std::cout << Color::CYAN << "[DEBUG]" << Color::RESET << " Calling CodeGen::generate()...\n";
        cppCode = codegen.generate(prog);
        std::cout << Color::CYAN << "[DEBUG]" << Color::RESET << " Generated " << cppCode.size() << " bytes of C++ code\n";
    } catch (const std::exception& e) {
        std::cerr << Color::RED << "error" << Color::RESET << ": Code generation failed: " << e.what() << "\n";
        return false;
    }
    
    auto codegenEnd = std::chrono::high_resolution_clock::now();
    stats.codegenTime = std::chrono::duration_cast<std::chrono::milliseconds>(codegenEnd - start).count();
    std::cout << Color::CYAN << "[DEBUG]" << Color::RESET << " Code generation took " << stats.codegenTime << "ms\n";
    
    // Write to temporary C++ file
    std::string cppFile = outputFile + ".cpp";
    std::cout << Color::CYAN << "[DEBUG]" << Color::RESET << " Writing to " << cppFile << "...\n";
    
    std::ofstream out(cppFile);
    if (!out) {
        std::cerr << Color::RED << "error" << Color::RESET << ": Failed to write C++ file\n";
        return false;
    }
    out << cppCode;
    out.close();
    
    std::cout << Color::CYAN << "[DEBUG]" << Color::RESET << " C++ file written successfully\n";
    
    if (emitCpp) {
        std::string savedCpp = outputFile + "_gen.cpp";
        fs::copy(cppFile, savedCpp, fs::copy_options::overwrite_existing);
        std::cout << Color::GREEN << "       Saved" << Color::RESET << " " << savedCpp << "\n";
    }
    
    std::cout << Color::CYAN << "[DEBUG]" << Color::RESET << " Preparing compilation command...\n";
    
    // Optimization flags
    std::string optFlags;
    if (debug) {
        optFlags = "-O0 -g -DDEBUG";
    } else {
        switch (optLevel) {
            case 0:
                optFlags = "-O0 -DNDEBUG";
                break;
            case 1:
                optFlags = "-O1 -DNDEBUG";
                break;
            case 2:
                optFlags = "-O2 -DNDEBUG";
                break;
            case 3:
                optFlags = "-O3 -march=native -mtune=native -flto -ffast-math "
                           "-funroll-loops -finline-functions -DNDEBUG";
                break;
            default:
                optFlags = "-O1 -DNDEBUG";
        }
    }
    
    // Compile command - NO RUNTIME LIBRARY NEEDED
    std::string compileCmd = "g++ -std=c++17 " + optFlags + " " + 
                            cppFile + " -o " + outputFile + " -lm -lpthread 2>&1";
    
    std::cout << Color::CYAN << "[DEBUG]" << Color::RESET << " Compile command:\n";
    std::cout << "  " << compileCmd << "\n";
    std::cout << Color::CYAN << "[DEBUG]" << Color::RESET << " Running g++ compiler...\n";
    std::cout << Color::YELLOW << "[WAIT]" << Color::RESET << " This may take a while with high optimization levels...\n";
    
    auto cppStart = std::chrono::high_resolution_clock::now();
    
    // Create a progress indicator
    std::cout << Color::CYAN << "[PROGRESS]" << Color::RESET << " Compiling";
    std::cout.flush();
    
    // Run compilation in background and show progress
    FILE* pipe = popen(compileCmd.c_str(), "r");
    if (!pipe) {
        std::cerr << "\n" << Color::RED << "error" << Color::RESET << ": Failed to run compiler\n";
        return false;
    }
    
    char buffer[256];
    std::string result;
    int lineCount = 0;
    
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
        lineCount++;
        
        // Show progress dots every 5 lines
        if (lineCount % 5 == 0) {
            std::cout << ".";
            std::cout.flush();
        }
        
        // If we're getting a lot of output, print it immediately (probably errors)
        if (lineCount > 20) {
            std::cout << "\n" << Color::YELLOW << "[COMPILER OUTPUT]" << Color::RESET << "\n";
            std::cout << result;
            result.clear();
        }
    }
    
    int returnCode = pclose(pipe);
    std::cout << "\n";
    
    auto cppEnd = std::chrono::high_resolution_clock::now();
    stats.cppCompileTime = std::chrono::duration_cast<std::chrono::milliseconds>(cppEnd - cppStart).count();
    
    std::cout << Color::CYAN << "[DEBUG]" << Color::RESET << " Compiler finished in " << stats.cppCompileTime << "ms\n";
    std::cout << Color::CYAN << "[DEBUG]" << Color::RESET << " Return code: " << returnCode << "\n";
    
    if (returnCode != 0) {
        if (!result.empty()) {
            std::cerr << Color::RED << "[COMPILER ERRORS]" << Color::RESET << "\n";
            std::cerr << result;
        }
        std::cerr << Color::RED << "error" << Color::RESET << ": Compilation failed (code " << returnCode << ")\n";
        
        // Show first 50 lines of generated C++ for debugging
        std::cout << Color::YELLOW << "[DEBUG]" << Color::RESET << " First 50 lines of generated C++:\n";
        std::cout << Color::DIM << "---" << Color::RESET << "\n";
        std::ifstream cppIn(cppFile);
        std::string line;
        int count = 0;
        while (std::getline(cppIn, line) && count < 50) {
            std::cout << std::setw(4) << (count + 1) << " | " << line << "\n";
            count++;
        }
        std::cout << Color::DIM << "---" << Color::RESET << "\n";
        
        return false;
    }
    
    // Clean up intermediate files unless emitCpp is set
    if (!emitCpp) {
        std::cout << Color::CYAN << "[DEBUG]" << Color::RESET << " Cleaning up temporary files...\n";
        fs::remove(cppFile);
    }
    
    // Get binary size
    if (fs::exists(outputFile)) {
        stats.outputSize = fs::file_size(outputFile);
        std::cout << Color::GREEN << "   Finished" << Color::RESET << " " << outputFile 
                 << " (" << formatSize(stats.outputSize) << ")\n";
    }
    
    return true;
}

// REPLACE the existing buildProject() function with this version:
int buildProjectDebug(bool verbose = false, bool debug = false, bool emitCpp = false, 
                 bool showTiming = false, int optLevel = 1) {
    auto totalStart = std::chrono::high_resolution_clock::now();
    CompileStats stats;
    
    std::cout << Color::CYAN << "[DEBUG]" << Color::RESET << " Starting build process...\n";
    
    auto& stdlibLoader = StdLibLoader::instance();
    if (!stdlibLoader.isInitialized()) {
        std::cout << Color::CYAN << "[DEBUG]" << Color::RESET << " Initializing stdlib...\n";
        std::vector<std::string> searchPaths = {
            "./stdlib",
            "../stdlib",
            "../../stdlib",
            std::string(getenv("MAGOLOR_STDLIB_PATH") ?: "")
        };
        
        for (const auto& path : searchPaths) {
            if (!path.empty() && fs::exists(path) && fs::is_directory(path)) {
                std::cout << Color::CYAN << "[DEBUG]" << Color::RESET << " Found stdlib at: " << path << "\n";
                stdlibLoader.init(path);
                break;
            }
        }
        
        if (!stdlibLoader.isInitialized()) {
            std::cerr << Color::YELLOW << "[WARNING]" << Color::RESET << " Could not find stdlib directory!\n";
        }
    }
    
    try {
        if (!fs::exists("project.toml")) {
            std::cerr << Color::RED << "error" << Color::RESET << ": project.toml not found\n";
            return 1;
        }
        
        std::cout << Color::CYAN << "[DEBUG]" << Color::RESET << " Loading project.toml...\n";
        Package pkg = PackageManager::loadFromToml("project.toml");
        std::cout << Color::GREEN << "Building" << Color::RESET << " " << pkg.name 
                 << " v" << pkg.version << "\n";
        
        std::cout << Color::CYAN << "[DEBUG]" << Color::RESET << " Clearing module registry...\n";
        ModuleRegistry::instance().clear();
        
        // Install/load dependencies
        std::vector<ResolvedPackage> deps;
        if (!pkg.dependencies.empty()) {
            std::cout << Color::CYAN << "[DEBUG]" << Color::RESET << " Loading dependencies...\n";
            deps = PackageManager::loadFromLockFile();
            if (deps.empty()) {
                std::cout << Color::CYAN << "[DEBUG]" << Color::RESET << " Installing dependencies...\n";
                auto result = PackageManager::installDependencies(pkg);
                if (!result.success) {
                    std::cerr << Color::RED << "error" << Color::RESET 
                             << ": failed to resolve dependencies\n";
                    return 1;
                }
                deps = result.packages;
            }
        }
        
        std::cout << Color::CYAN << "[DEBUG]" << Color::RESET << " Collecting source files...\n";
        auto sourceFiles = PackageManager::collectSourceFiles(pkg, deps);
        if (sourceFiles.empty()) {
            std::cerr << Color::RED << "error" << Color::RESET << ": no source files found\n";
            return 1;
        }
        
        std::cout << Color::GREEN << "   Compiling" << Color::RESET << " " 
                 << sourceFiles.size() << " files\n";
        
        std::vector<Program> appPrograms;
        bool hasErrors = false;
        
        int fileNum = 0;
        for (const auto &file : sourceFiles) {
            fileNum++;
            std::cout << Color::CYAN << "[" << fileNum << "/" << sourceFiles.size() << "]" 
                     << Color::RESET << " Compiling " << file << "...\n";
            
            std::string pkgName = pkg.name;
            std::string relPath = file;
            
            try {
                fs::path absFile = fs::absolute(file);
                fs::path absProj = fs::absolute(".");
                relPath = fs::relative(absFile, absProj).string();
            } catch (...) {
                relPath = file;
            }
            
            auto prog = compileFile(relPath, pkgName, hasErrors, stats, verbose);
            if (hasErrors) break;
            
            if (PackageManager::isAppSource(file, pkg)) {
                appPrograms.push_back(prog);
            }
            
            std::cout << Color::GREEN << "      [OK]" << Color::RESET << "\n";
        }
        
        if (hasErrors) {
            std::cerr << Color::RED << "error" << Color::RESET << ": compilation failed\n";
            return 1;
        }
        
        if (appPrograms.empty()) {
            std::cerr << Color::RED << "error" << Color::RESET 
                     << ": no application source files found\n";
            return 1;
        }
        
        // Resolve imports
        std::cout << Color::CYAN << "[DEBUG]" << Color::RESET << " Resolving module imports...\n";
        ImportResolver importResolver;
        for (const auto &[name, module] : ModuleRegistry::instance().getModules()) {
            auto result = importResolver.resolve(module);
            if (!result.success) {
                std::cerr << Color::RED << "error" << Color::RESET << ": " << result.error << "\n";
                return 1;
            }
        }
        
        // Type checking
        std::cout << Color::CYAN << "[DEBUG]" << Color::RESET << " Type checking...\n";
        auto typeCheckStart = std::chrono::high_resolution_clock::now();
        std::string dummySource = "";
        ErrorReporter typeCheckReporter("type-check", dummySource);
        TypeChecker typeChecker(typeCheckReporter, ModuleRegistry::instance());
        
        for (const auto &[name, module] : ModuleRegistry::instance().getModules()) {
            if (!typeChecker.checkModule(module)) {
                typeCheckReporter.printDiagnostics();
                std::cerr << Color::RED << "error" << Color::RESET << ": type checking failed\n";
                return 1;
            }
        }
        auto typeCheckEnd = std::chrono::high_resolution_clock::now();
        stats.typeCheckTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            typeCheckEnd - typeCheckStart).count();
        
        // Merge programs
        std::cout << Color::CYAN << "[DEBUG]" << Color::RESET << " Merging programs...\n";
        Program merged = mergePrograms(appPrograms);
        
        // Create target directory
        fs::create_directories("target");
        std::string exePath = "target/" + pkg.name;
        
        // Compile with C++ (using debug version)
        std::cout << Color::CYAN << "[DEBUG]" << Color::RESET << " Starting C++ compilation...\n";
        if (!compileWithCppDebug(merged, exePath, stats, verbose, debug, emitCpp, optLevel)) {
            return 1;
        }
        
        auto totalEnd = std::chrono::high_resolution_clock::now();
        stats.totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            totalEnd - totalStart).count();
        
        std::cout << Color::GREEN << "   Finished" << Color::RESET << " " 
                 << (debug ? "debug" : ("opt-level " + std::to_string(optLevel))) << " target in " 
                 << formatTime(stats.totalTime) << "\n";
        std::cout << "    Binary: " << exePath << " (" << formatSize(stats.outputSize) << ")\n";
        
        if (showTiming) {
            stats.print();
        }
        
        return 0;
    } catch (const std::exception &e) {
        std::cerr << Color::RED << "error" << Color::RESET << ": " << e.what() << "\n";
        return 1;
    }
}

// ============================================================================
// Main entry point
// ============================================================================
int main(int argc, char *argv[]) {
    // Initialize stdlib loader at startup
    auto& stdlibLoader = StdLibLoader::instance();
    if (!stdlibLoader.isInitialized()) {
        std::vector<std::string> searchPaths = {
            "./stdlib",
            "../stdlib",
            "../../stdlib",
            std::string(getenv("MAGOLOR_STDLIB_PATH") ?: "")
        };
        
        for (const auto& path : searchPaths) {
            if (!path.empty() && fs::exists(path) && fs::is_directory(path)) {
                if (getenv("MAGOLOR_VERBOSE")) {
                    std::cerr << "[Main] Initializing stdlib from: " << path << std::endl;
                }
                stdlibLoader.init(path);
                break;
            }
        }
    }
    
    if (argc < 2) {
        printUsage();
        return 0;
    }
    
    std::string cmd = argv[1];
    bool verbose = false;
    bool debug = false;
    bool emitCpp = false;
    bool showTiming = false;
    bool noColor = false;
    int optLevel = 1;
    std::string outputFile;
    
    // Parse flags
    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--verbose" || arg == "-v") {
            verbose = true;
        } else if (arg == "--debug") {
            debug = true;
        } else if (arg == "--release") {
            debug = false;
            optLevel = 3;
        } else if (arg == "--emit-cpp") {
            emitCpp = true;
        } else if (arg == "--timing") {
            showTiming = true;
        } else if (arg == "--no-color") {
            noColor = true;
            Color::RESET = "";
            Color::BOLD = "";
            Color::RED = "";
            Color::GREEN = "";
            Color::YELLOW = "";
            Color::BLUE = "";
            Color::MAGENTA = "";
            Color::CYAN = "";
            Color::DIM = "";
        } else if (arg == "--opt-level" && i + 1 < argc) {
            try {
                optLevel = std::stoi(argv[++i]);
                if (optLevel < 0 || optLevel > 3) {
                    std::cerr << Color::YELLOW << "warning" << Color::RESET 
                             << ": opt-level must be 0-3, using 1\n";
                    optLevel = 1;
                }
            } catch (...) {
                std::cerr << Color::YELLOW << "warning" << Color::RESET 
                         << ": invalid opt-level, using 1\n";
                optLevel = 1;
            }
        } else if (arg == "-o" && i + 1 < argc) {
            outputFile = argv[++i];
        }
    }
    
    // Handle commands
    if (cmd == "help" || cmd == "--help" || cmd == "-h") {
        printUsage();
        return 0;
    }
    
    if (cmd == "version" || cmd == "--version") {
        printVersion();
        return 0;
    }
    
    if (cmd == "lsp") {
        MagolorLanguageServer server;
        server.run();
        return 0;
    }
    
    if (cmd == "clean") {
        return cleanProject() ? 0 : 1;
    }
    
    if (cmd == "build-project") {
        return buildProjectDebug(verbose, debug, emitCpp, showTiming, optLevel);
    }
    
    // All other commands require a source file
    if (argc < 3) {
        std::cerr << Color::RED << "error" << Color::RESET << ": missing source file\n";
        std::cerr << "Usage: magolor " << cmd << " <file.mg>\n";
        return 1;
    }
    
    std::string srcPath = argv[2];
    
    // Check if file exists
    if (!fs::exists(srcPath)) {
        std::cerr << Color::RED << "error" << Color::RESET << ": file not found: " << srcPath << "\n";
        return 1;
    }
    
    try {
        CompileStats stats;
        auto totalStart = std::chrono::high_resolution_clock::now();
        
        // Compile single file
        Program prog = compileSingleFile(srcPath, stats, verbose);
        if (prog.functions.empty() && prog.classes.empty()) {
            return 1;
        }
        
        // Type check
        std::string dummySource = "";
        ErrorReporter typeCheckReporter("type-check", dummySource);
        TypeChecker typeChecker(typeCheckReporter, ModuleRegistry::instance());
        
        auto module = std::make_shared<Module>();
        module->name = "main";
        module->filepath = srcPath;
        module->ast = prog;
        
        auto typeCheckStart = std::chrono::high_resolution_clock::now();
        if (!typeChecker.checkModule(module)) {
            typeCheckReporter.printDiagnostics();
            return 1;
        }
        auto typeCheckEnd = std::chrono::high_resolution_clock::now();
        stats.typeCheckTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            typeCheckEnd - typeCheckStart).count();
        
        // Determine output file
        fs::path srcFsPath(srcPath);
        std::string baseName = srcFsPath.stem().string();
        std::string exePath = outputFile.empty() ? baseName : outputFile;
        
        if (cmd == "emit-cpp") {
            CodeGen codegen;
            std::string cppCode = codegen.generate(prog);
            
            std::string outFile = baseName + "_gen.cpp";
            std::ofstream out(outFile);
            out << cppCode;
            out.close();
            
            std::cout << Color::GREEN << "   Generated" << Color::RESET << " " << outFile << "\n";
            return 0;
        }
        
        if (cmd == "check") {
            std::cout << Color::GREEN << "    Checking" << Color::RESET << " " << srcPath << "\n";
            std::cout << Color::GREEN << "    Finished" << Color::RESET << " no errors found\n";
            return 0;
        }
        
        // Compile with C++
        if (!compileWithCpp(prog, exePath, stats, verbose, debug, emitCpp, optLevel)) {
            return 1;
        }
        
        auto totalEnd = std::chrono::high_resolution_clock::now();
        stats.totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            totalEnd - totalStart).count();
        
        if (cmd == "run") {
            if (verbose) {
                std::cout << Color::GREEN << "    Running" << Color::RESET << " " << exePath << "\n\n";
            }
            std::string runCmd = "./" + exePath;
            int result = std::system(runCmd.c_str());
            fs::remove(exePath);
            if (!emitCpp) {
                fs::remove(exePath + ".cpp");
            }
            return result;
        }
        
        std::cout << Color::GREEN << "   Finished" << Color::RESET << " " << exePath 
                 << " in " << formatTime(stats.totalTime) << "\n";
        
        if (showTiming) {
            stats.print();
        }
        
        return 0;
    } catch (const std::exception &e) {
        std::cerr << Color::RED << "error" << Color::RESET << ": " << e.what() << "\n";
        return 1;
    }
}
