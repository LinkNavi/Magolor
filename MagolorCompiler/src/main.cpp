// Magolor Compiler - LLVM Backend Edition
#include "codegen.hpp"
#include "codegen_llvm.hpp"
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
    std::cout << Color::BOLD << "Magolor Compiler v0.4.0" << Color::RESET << " (LLVM Backend)\n\n";
    
    std::cout << Color::BOLD << "USAGE:" << Color::RESET << "\n";
    std::cout << "    magolor [COMMAND] [OPTIONS] <file.mg>\n\n";
    
    std::cout << Color::BOLD << "COMMANDS:" << Color::RESET << "\n";
    std::cout << "    " << Color::CYAN << "build" << Color::RESET << " <file.mg>        Compile source file to executable\n";
    std::cout << "    " << Color::CYAN << "run" << Color::RESET << " <file.mg>          Compile and run immediately\n";
    std::cout << "    " << Color::CYAN << "check" << Color::RESET << " <file.mg>        Type check without building\n";
    std::cout << "    " << Color::CYAN << "emit-llvm" << Color::RESET << " <file.mg>    Output LLVM IR (.ll file)\n";
    std::cout << "    " << Color::CYAN << "emit-bc" << Color::RESET << " <file.mg>      Output LLVM bitcode (.bc file)\n";
    std::cout << "    " << Color::CYAN << "emit-obj" << Color::RESET << " <file.mg>     Output object file only (.o)\n";
    std::cout << "    " << Color::CYAN << "build-project" << Color::RESET << "         Build multi-file project (for gear)\n";
    std::cout << "    " << Color::CYAN << "clean" << Color::RESET << "                 Clean build artifacts\n";
    std::cout << "    " << Color::CYAN << "lsp" << Color::RESET << "                   Start Language Server\n";
    std::cout << "    " << Color::CYAN << "version" << Color::RESET << "               Show version info\n";
    std::cout << "    " << Color::CYAN << "help" << Color::RESET << "                  Show this help\n\n";
    
    std::cout << Color::BOLD << "OPTIONS:" << Color::RESET << "\n";
    std::cout << "    " << Color::GREEN << "-o" << Color::RESET << " <file>           Specify output file name\n";
    std::cout << "    " << Color::GREEN << "--verbose" << Color::RESET << ", " << Color::GREEN << "-v" << Color::RESET << "      Show detailed compilation steps\n";
    std::cout << "    " << Color::GREEN << "--backend" << Color::RESET << " <name>    Choose backend: llvm (default) or cpp\n";
    std::cout << "    " << Color::GREEN << "--no-color" << Color::RESET << "          Disable colored output\n";
    std::cout << "    " << Color::GREEN << "--timing" << Color::RESET << "            Show compilation timing breakdown\n\n";
    
    std::cout << Color::BOLD << "EXAMPLES:" << Color::RESET << "\n";
    std::cout << "    magolor build hello.mg              # Compile with LLVM\n";
    std::cout << "    magolor run hello.mg                # Quick run\n";
    std::cout << "    magolor emit-llvm hello.mg          # View LLVM IR\n";
    std::cout << "    magolor build hello.mg --backend=cpp  # Use C++ backend\n\n";
    
    std::cout << Color::BOLD << "NOTE:" << Color::RESET << "\n";
    std::cout << "    LLVM backend provides 2-10x better runtime performance\n";
    std::cout << "    C++ backend is kept for debugging and compatibility\n\n";
}

void printVersion() {
    std::cout << Color::BOLD << "Magolor Compiler v0.4.0" << Color::RESET << "\n";
    std::cout << "Build: " << __DATE__ << " " << __TIME__ << "\n";
    std::cout << "Default Backend: LLVM\n";
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
    long long llvmGenTime = 0;
    long long linkTime = 0;
    long long totalTime = 0;
    int lineCount = 0;
    size_t outputSize = 0;
    
    void print() const {
        std::cout << "\n" << Color::BOLD << "Compilation Statistics:" << Color::RESET << "\n";
        std::cout << "  Lines:        " << lineCount << "\n";
        std::cout << "  Lex:          " << formatTime(lexTime) << "\n";
        std::cout << "  Parse:        " << formatTime(parseTime) << "\n";
        std::cout << "  Type check:   " << formatTime(typeCheckTime) << "\n";
        std::cout << "  LLVM gen:     " << formatTime(llvmGenTime) << "\n";
        std::cout << "  Link:         " << formatTime(linkTime) << "\n";
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

// ============================================================================
// LLVM Backend Compilation
// ============================================================================
bool compileWithLLVM(const Program& prog, const std::string& outputFile,
                     CompileStats& stats, bool verbose, bool emitLLVM = false,
                     bool emitBC = false, bool emitObj = false) {
    try {
        if (verbose) {
            std::cout << Color::GREEN << "  Generating" << Color::RESET << " LLVM IR\n";
        }
        
        // Generate LLVM
        auto llvmStart = std::chrono::high_resolution_clock::now();
        LLVMCodeGen codegen;
        
        // Get base name for module
        fs::path inputPath(outputFile);
        std::string moduleName = inputPath.stem().string();
        
        // Generate the module (but codegen keeps ownership)
auto* llvmModule = codegen.generate(prog, moduleName);

if (!llvmModule) {
    std::cerr << Color::RED << "error" << Color::RESET << ": Failed to generate LLVM IR\n";
    return false;
}
        
        auto llvmEnd = std::chrono::high_resolution_clock::now();
        stats.llvmGenTime = std::chrono::duration_cast<std::chrono::milliseconds>(llvmEnd - llvmStart).count();
        
        // Get base name for output files
        std::string baseName = outputFile;
        size_t dotPos = baseName.find_last_of('.');
        if (dotPos != std::string::npos) {
            baseName = baseName.substr(0, dotPos);
        }
        
        // Emit LLVM IR if requested
        if (emitLLVM) {
            std::string llFile = baseName + ".ll";
            if (verbose) {
                std::cout << Color::GREEN << "     Writing" << Color::RESET << " " << llFile << "\n";
            }
            if (!codegen.writeIRToFile(llFile)) {
                std::cerr << Color::RED << "error" << Color::RESET << ": Failed to write LLVM IR\n";
                return false;
            }
        }
        
        // Generate object file
        std::string objFile = baseName + ".o";
        if (verbose) {
            std::cout << Color::GREEN << "  Generating" << Color::RESET << " object file\n";
        }
        
        if (!codegen.writeObjectFile(objFile)) {
            std::cerr << Color::RED << "error" << Color::RESET << ": Failed to write object file\n";
            return false;
        }
        
        if (emitObj) {
            std::cout << Color::GREEN << "   Finished" << Color::RESET << " " << objFile << "\n";
            return true;
        }
        
        // Link
        if (verbose) {
            std::cout << Color::GREEN << "    Linking" << Color::RESET << " executable\n";
        }
        
        auto linkStart = std::chrono::high_resolution_clock::now();
        
        // Collect link flags from the program
        auto linkFlags = codegen.collectLinkFlags(prog);
        std::string linkFlagsStr;
        for (const auto& flag : linkFlags) {
            linkFlagsStr += " " + flag;
        }
        
        // Link command
     std::string linkCmd = "clang++ -no-pie " + objFile + " -o " + outputFile + 
                            " -lm -lpthread" + linkFlagsStr + " 2>&1";
        if (verbose) {
            std::cout << Color::CYAN << "    Command:" << Color::RESET << " " << linkCmd << "\n";
        }
        
        FILE* pipe = popen(linkCmd.c_str(), "r");
        if (!pipe) {
            std::cerr << Color::RED << "error" << Color::RESET << ": Failed to run linker\n";
            return false;
        }
        
        char buffer[256];
        std::string result;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        int returnCode = pclose(pipe);
        
        auto linkEnd = std::chrono::high_resolution_clock::now();
        stats.linkTime = std::chrono::duration_cast<std::chrono::milliseconds>(linkEnd - linkStart).count();
        
        if (returnCode != 0) {
            std::cerr << result;
            std::cerr << Color::RED << "error" << Color::RESET << ": Linking failed\n";
            return false;
        }
        
        // Clean up object file unless --emit-obj
        if (!emitObj && fs::exists(objFile)) {
            fs::remove(objFile);
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
        
    } catch (const std::exception& e) {
        std::cerr << Color::RED << "error" << Color::RESET << ": " << e.what() << "\n";
        return false;
    }
}

// ============================================================================
// C++ Backend Compilation (fallback)
// ============================================================================
bool compileWithCpp(const Program& prog, const std::string& outputFile, 
                    CompileStats &stats, bool verbose, bool emitCpp = false) {
    auto start = std::chrono::high_resolution_clock::now();
    
    if (verbose) {
        std::cout << Color::GREEN << "   Generating" << Color::RESET << " C++ code\n";
    }
    
    // Generate C++ code
    CodeGen codegen;
    std::string cppCode = codegen.generate(prog);
    auto codegenEnd = std::chrono::high_resolution_clock::now();
    stats.llvmGenTime = std::chrono::duration_cast<std::chrono::milliseconds>(codegenEnd - start).count();
    
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
        std::cout << Color::GREEN << "    Compiling" << Color::RESET << " to native code\n";
    }
    
    // Collect link flags from @link blocks
    auto linkFlags = codegen.collectLinkFlags(prog);
    std::string linkFlagsStr;
    for (const auto& flag : linkFlags) {
        linkFlagsStr += " " + flag;
    }
    
    // Compile command
    std::string compileCmd = "g++ -std=c++17 -O2 " + cppFile + " -o " + outputFile + 
                            " -lm -lpthread" + linkFlagsStr + " 2>&1";
    
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
    stats.linkTime = std::chrono::duration_cast<std::chrono::milliseconds>(cppEnd - cppStart).count();
    
    if (returnCode != 0) {
        std::cerr << result;
        std::cerr << Color::RED << "error" << Color::RESET << ": Compilation failed\n";
        return false;
    }
    
    // Clean up intermediate files
    if (!emitCpp) {
        fs::remove(cppFile);
    }
    
    // Get binary size
    if (fs::exists(outputFile)) {
        stats.outputSize = fs::file_size(outputFile);
    }
    
    return true;
}

// ============================================================================
// Project compilation helpers (continued from original)
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
        merged.linkDecls.insert(merged.linkDecls.end(), prog.linkDecls.begin(), prog.linkDecls.end());
        merged.includeDecls.insert(merged.includeDecls.end(), prog.includeDecls.begin(), prog.includeDecls.end());
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

int buildProject(bool verbose = false, bool showTiming = false, const std::string& backend = "llvm") {
    auto totalStart = std::chrono::high_resolution_clock::now();
    CompileStats stats;
    
    auto& stdlibLoader = StdLibLoader::instance();
    if (!stdlibLoader.isInitialized()) {
        std::vector<std::string> searchPaths = {
            "./stdlib",
            "../stdlib",
            "../../stdlib",
            "/usr/local/share/magolor/stdlib",
            std::string(getenv("MAGOLOR_STDLIB_PATH") ?: "")
        };
        
        for (const auto& path : searchPaths) {
            if (!path.empty() && fs::exists(path) && fs::is_directory(path)) {
                stdlibLoader.init(path);
                break;
            }
        }
    }
    
    try {
        if (!fs::exists("project.toml")) {
            std::cerr << Color::RED << "error" << Color::RESET << ": project.toml not found\n";
            return 1;
        }
        
        Package pkg = PackageManager::loadFromToml("project.toml");
        std::cout << Color::GREEN << "Building" << Color::RESET << " " << pkg.name 
                 << " v" << pkg.version << " with " << backend << " backend\n";
        
        ModuleRegistry::instance().clear();
        
        // Load dependencies
        std::vector<ResolvedPackage> deps;
        if (!pkg.dependencies.empty()) {
            deps = PackageManager::loadFromLockFile();
            if (deps.empty()) {
                auto result = PackageManager::installDependencies(pkg);
                if (!result.success) {
                    std::cerr << Color::RED << "error" << Color::RESET 
                             << ": failed to resolve dependencies\n";
                    return 1;
                }
                deps = result.packages;
            }
        }
        
        auto sourceFiles = PackageManager::collectSourceFiles(pkg, deps);
        if (sourceFiles.empty()) {
            std::cerr << Color::RED << "error" << Color::RESET << ": no source files found\n";
            return 1;
        }
        
        std::cout << Color::GREEN << "   Compiling" << Color::RESET << " " 
                 << sourceFiles.size() << " files\n";
        
        std::vector<Program> appPrograms;
        bool hasErrors = false;
        
        for (const auto &file : sourceFiles) {
            auto prog = compileFile(file, pkg.name, hasErrors, stats, verbose);
            if (hasErrors) break;
            
            if (PackageManager::isAppSource(file, pkg)) {
                appPrograms.push_back(prog);
            }
        }
        
        if (hasErrors) {
            return 1;
        }
        
        if (appPrograms.empty()) {
            std::cerr << Color::RED << "error" << Color::RESET 
                     << ": no application source files found\n";
            return 1;
        }
        
        // Resolve imports
        ImportResolver importResolver;
        for (const auto &[name, module] : ModuleRegistry::instance().getModules()) {
            auto result = importResolver.resolve(module);
            if (!result.success) {
                std::cerr << Color::RED << "error" << Color::RESET << ": " << result.error << "\n";
                return 1;
            }
        }
        
        // Type checking
        auto typeCheckStart = std::chrono::high_resolution_clock::now();
        std::string dummySource = "";
        ErrorReporter typeCheckReporter("type-check", dummySource);
        TypeChecker typeChecker(typeCheckReporter, ModuleRegistry::instance());
        
        for (const auto &[name, module] : ModuleRegistry::instance().getModules()) {
            if (!typeChecker.checkModule(module)) {
                typeCheckReporter.printDiagnostics();
                return 1;
            }
        }
        auto typeCheckEnd = std::chrono::high_resolution_clock::now();
        stats.typeCheckTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            typeCheckEnd - typeCheckStart).count();
        
        // Merge programs
        Program merged = mergePrograms(appPrograms);
        
        // Create target directory
        fs::create_directories("target");
        std::string exePath = "target/" + pkg.name;
        
        // Compile
        bool success;
        if (backend == "llvm") {
            success = compileWithLLVM(merged, exePath, stats, verbose);
        } else {
            success = compileWithCpp(merged, exePath, stats, verbose);
        }
        
        if (!success) {
            return 1;
        }
        
        auto totalEnd = std::chrono::high_resolution_clock::now();
        stats.totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            totalEnd - totalStart).count();
        
        std::cout << Color::GREEN << "   Finished" << Color::RESET << " in " 
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
// ===========================================================================

int main(int argc, char *argv[]) {
    // Initialize stdlib loader
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
    bool showTiming = false;
    bool noColor = false;
    bool emitLLVM = false;
    bool emitBC = false;
    bool emitObj = false;
    bool emitCpp = false;
    std::string backend = "llvm";  // Default to LLVM
    std::string outputFile;
    
    // Parse flags
    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--verbose" || arg == "-v") {
            verbose = true;
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
        } else if (arg == "--backend" && i + 1 < argc) {
            backend = argv[++i];
            if (backend != "llvm" && backend != "cpp") {
                std::cerr << Color::RED << "error" << Color::RESET
                          << ": unknown backend '" << backend << "'\n";
                return 1;
            }
        } else if (arg == "-o" && i + 1 < argc) {
            outputFile = argv[++i];
        } else if (arg[0] != '-') {
            // This is likely the input file, skip
            continue;
        } else {
            std::cerr << Color::RED << "error" << Color::RESET
                      << ": unknown option '" << arg << "'\n";
            return 1;
        }
    }

    // Handle trivial commands
    if (cmd == "help") {
        printUsage();
        return 0;
    }

    if (cmd == "version") {
        printVersion();
        return 0;
    }

    if (cmd == "clean") {
        cleanProject();
        return 0;
    }

    if (cmd == "lsp") {
	MagolorLanguageServer server;
        server.run();
    }

    // Project build
    if (cmd == "build-project") {
        return buildProject(verbose, showTiming, backend);
    }

    // Commands that require a source file
    if (argc < 3) {
        std::cerr << Color::RED << "error" << Color::RESET
                  << ": missing input file\n";
        return 1;
    }

    std::string inputFile = argv[argc - 1];
    if (!fs::exists(inputFile)) {
        std::cerr << Color::RED << "error" << Color::RESET
                  << ": file not found: " << inputFile << "\n";
        return 1;
    }

    if (outputFile.empty()) {
        fs::path p(inputFile);
        outputFile = p.stem().string();
    }

    CompileStats stats;
    auto totalStart = std::chrono::high_resolution_clock::now();

    // Compile frontend
    Program prog = compileSingleFile(inputFile, stats, verbose);
    if (prog.functions.empty() && prog.classes.empty()) {
        return 1;
    }

    // Type check
    auto typeCheckStart = std::chrono::high_resolution_clock::now();
    ErrorReporter reporter(inputFile, "");
    TypeChecker typeChecker(reporter, ModuleRegistry::instance());
    if (!typeChecker.checkProgram(prog)) {
        reporter.printDiagnostics();
        return 1;
    }
    auto typeCheckEnd = std::chrono::high_resolution_clock::now();
    stats.typeCheckTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        typeCheckEnd - typeCheckStart).count();

    bool success = false;

    if (cmd == "check") {
        std::cout << Color::GREEN << "OK" << Color::RESET << "\n";
        return 0;
    }

    if (cmd == "emit-llvm") emitLLVM = true;
    if (cmd == "emit-bc")   emitBC   = true;
    if (cmd == "emit-obj")  emitObj  = true;

    if (backend == "llvm") {
        success = compileWithLLVM(
            prog,
            outputFile,
            stats,
            verbose,
            emitLLVM,
            emitBC,
            emitObj
        );
    } else {
        success = compileWithCpp(
            prog,
            outputFile,
            stats,
            verbose,
            emitCpp
        );
    }

    if (!success) {
        return 1;
    }

    // Run if requested
    if (cmd == "run") {
        std::string runCmd = "./" + outputFile;
        if (verbose) {
            std::cout << Color::CYAN << "Running:" << Color::RESET
                      << " " << runCmd << "\n";
        }
        return std::system(runCmd.c_str());
    }

    auto totalEnd = std::chrono::high_resolution_clock::now();
    stats.totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        totalEnd - totalStart).count();

    if (showTiming) {
        stats.print();
    }

    return 0;
}
