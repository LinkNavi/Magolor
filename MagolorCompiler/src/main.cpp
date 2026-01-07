// Updated main.cpp - C++ transpiler with maximum performance

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

namespace fs = std::filesystem;

std::string readFile(const std::string &path) {
  std::ifstream f(path);
  if (!f)
    throw std::runtime_error("Cannot open file: " + path);
  std::stringstream buf;
  buf << f.rdbuf();
  return buf.str();
}

void printUsage() {
  std::cout << "\033[1mMagolor Compiler v0.2.0 (C++ Backend)\033[0m\n\n";
  std::cout << "\033[1mUSAGE:\033[0m\n";
  std::cout << "    magolor [COMMAND] [OPTIONS]\n\n";
  std::cout << "\033[1mCOMMANDS:\033[0m\n";
  std::cout << "    build [file.mg]     Compile source file to executable\n";
  std::cout << "    build-project       Build entire project (uses project.toml)\n";
  std::cout << "    emit-cpp <file>     Output generated C++ code\n";
  std::cout << "    run <file.mg>       Compile and run immediately\n";
  std::cout << "    check <file.mg>     Check for errors without building\n";
  std::cout << "    lsp                 Start Language Server\n";
  std::cout << "    help                Show this help\n\n";
  std::cout << "\033[1mOPTIONS:\033[0m\n";
  std::cout << "    -o <file>          Specify output file name\n";
  std::cout << "    --verbose          Show detailed compilation steps\n";
  std::cout << "    --debug            Compile with debug symbols (O0)\n";
  std::cout << "    --release          Maximum optimization (O3, default)\n";
  std::cout << "    --emit-cpp         Also save generated C++ file\n";
}

bool ensureRuntimeBuilt() {
    std::string runtimeDir = "./runtime";
    std::string runtimeLib = runtimeDir + "/libmagolor.a";
    std::string runtimeSrc = runtimeDir + "/runtime.c";
    
    // Check if library exists and is newer than source
    if (fs::exists(runtimeLib) && fs::exists(runtimeSrc)) {
        auto libTime = fs::last_write_time(runtimeLib);
        auto srcTime = fs::last_write_time(runtimeSrc);
        if (libTime >= srcTime) {
            return true; // Already built and up to date
        }
    }
    
    std::cout << "\033[1;32m   Building\033[0m runtime library...\n";
    
    // Build the runtime with optimization
    std::string buildCmd = "cd " + runtimeDir + " && "
                          "gcc -c runtime.c -o runtime.o -O3 -march=native -flto -Wall && "
                          "ar rcs libmagolor.a runtime.o && "
                          "rm runtime.o 2>&1";
    
    int result = std::system(buildCmd.c_str());
    if (result != 0) {
        std::cerr << "\033[1;31merror\033[0m: Failed to build runtime library\n";
        return false;
    }
    
    std::cout << "\033[1;32m   Finished\033[0m runtime library\n";
    return true;
}

Program compileFile(const std::string &filepath, const std::string &packageName,
                    bool &hasErrors, bool verbose = false) {
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
    module->buildSymbolTable();

    ModuleRegistry::instance().registerModule(module);

    return prog;
}

Program mergePrograms(const std::vector<Program> &programs) {
  Program merged;

  for (const auto &prog : programs) {
    merged.usings.insert(merged.usings.end(), prog.usings.begin(),
                         prog.usings.end());
    merged.classes.insert(merged.classes.end(), prog.classes.begin(),
                          prog.classes.end());
    merged.functions.insert(merged.functions.end(), prog.functions.begin(),
                            prog.functions.end());
  }

  return merged;
}

bool compileWithCpp(const Program& prog, const std::string& outputFile, 
                    bool verbose, bool debug, bool emitCpp) {
    if (verbose) {
        std::cout << "\033[1;32m   Generating\033[0m C++ code\n";
    }

    if (!ensureRuntimeBuilt()) {
        return false;
    }
    
    // Generate C++ code
    CodeGen codegen;
    std::string cppCode = codegen.generate(prog);
    
    // Write to temporary C++ file
    std::string cppFile = outputFile + ".cpp";
    std::ofstream out(cppFile);
    if (!out) {
        std::cerr << "\033[1;31merror\033[0m: Failed to write C++ file\n";
        return false;
    }
    out << cppCode;
    out.close();
    
    if (emitCpp) {
        std::string savedCpp = outputFile + "_gen.cpp";
        fs::copy(cppFile, savedCpp, fs::copy_options::overwrite_existing);
        if (verbose) {
            std::cout << "\033[1;32m       Saved\033[0m " << savedCpp << "\n";
        }
    }
    
    if (verbose) {
        std::cout << "\033[1;32m    Compiling\033[0m to native code\n";
    }
    
    // Get runtime library path
    std::string runtimeLib = fs::absolute("./runtime/libmagolor.a").string();
    
    // Optimization flags
    std::string optFlags;
    if (debug) {
        optFlags = "-O0 -g -DDEBUG";
    } else {
        // Maximum performance
        optFlags = "-O3 -march=native -mtune=native -flto -ffast-math "
                   "-funroll-loops -finline-functions -DNDEBUG";
    }
    
    // Compile command
    std::string compileCmd = "g++ -std=c++17 " + optFlags + " " + 
                            cppFile + " " + runtimeLib + 
                            " -o " + outputFile + " -lm -lpthread 2>&1";
    
    if (verbose) {
        std::cout << "\033[1;36m    Command:\033[0m " << compileCmd << "\n";
    }
    
    FILE* pipe = popen(compileCmd.c_str(), "r");
    if (!pipe) {
        std::cerr << "\033[1;31merror\033[0m: Failed to run compiler\n";
        return false;
    }
    
    char buffer[256];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    int returnCode = pclose(pipe);
    
    if (returnCode != 0) {
        std::cerr << result;
        std::cerr << "\033[1;31merror\033[0m: Compilation failed\n";
        return false;
    }
    
    // Clean up intermediate files unless emitCpp is set
    if (!emitCpp) {
        fs::remove(cppFile);
    }
    
    if (verbose) {
        std::cout << "\033[1;32m   Finished\033[0m " << outputFile << "\n";
        
        // Show binary size
        if (fs::exists(outputFile)) {
            auto size = fs::file_size(outputFile);
            double sizeKB = size / 1024.0;
            std::cout << "\033[1;36m       Size:\033[0m " << std::fixed 
                      << std::setprecision(1) << sizeKB << " KB\n";
        }
    }
    
    return true;
}

int buildProject(bool verbose = false, bool debug = false, bool emitCpp = false) {
  try {
    if (!fs::exists("project.toml")) {
      std::cerr << "\033[1;31merror\033[0m: project.toml not found\n";
      return 1;
    }

    Package pkg = PackageManager::loadFromToml("project.toml");

    if (verbose) {
      std::cout << "\033[1;32mBuilding\033[0m " << pkg.name << " v" << pkg.version << "\n";
    }

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

    auto sourceFiles = PackageManager::collectSourceFiles(pkg, deps);

    if (sourceFiles.empty()) {
      std::cerr << "\033[1;31merror\033[0m: no source files found\n";
      return 1;
    }

    if (verbose) {
      std::cout << "\033[1;32m   Compiling\033[0m " << sourceFiles.size() << " files\n";
    }

    std::vector<Program> appPrograms;
    bool hasErrors = false;

    for (const auto &file : sourceFiles) {
      std::string pkgName = pkg.name;
      std::string relPath = file;
      
      try {
        fs::path absFile = fs::absolute(file);
        fs::path absProj = fs::absolute(".");
        relPath = fs::relative(absFile, absProj).string();
      } catch (...) {
        relPath = file;
      }
      
      auto prog = compileFile(relPath, pkgName, hasErrors, verbose);
      if (hasErrors) break;

      if (PackageManager::isAppSource(file, pkg)) {
        appPrograms.push_back(prog);
      }
    }

    if (hasErrors) {
      std::cerr << "\033[1;31merror\033[0m: compilation failed\n";
      return 1;
    }

    if (appPrograms.empty()) {
      std::cerr << "\033[1;31merror\033[0m: no application source files found\n";
      return 1;
    }

    // Resolve imports
    if (verbose) {
      std::cout << "\033[1;32m  Resolving\033[0m module imports...\n";
    }

    ImportResolver importResolver;
    for (const auto &[name, module] : ModuleRegistry::instance().getModules()) {
      auto result = importResolver.resolve(module);
      if (!result.success) {
        std::cerr << "\033[1;31merror\033[0m: " << result.error << "\n";
        return 1;
      }
    }

    // Type checking
    if (verbose) {
      std::cout << "\033[1;32m  Type checking\033[0m...\n";
    }

    std::string dummySource = "";
    ErrorReporter typeCheckReporter("type-check", dummySource);
    TypeChecker typeChecker(typeCheckReporter, ModuleRegistry::instance());

    for (const auto &[name, module] : ModuleRegistry::instance().getModules()) {
      if (!typeChecker.checkModule(module)) {
        typeCheckReporter.printDiagnostics();
        std::cerr << "\033[1;31merror\033[0m: type checking failed\n";
        return 1;
      }
    }

    // Merge programs
    Program merged = mergePrograms(appPrograms);

    // Create target directory
    fs::create_directories("target");
    std::string exePath = "target/" + pkg.name;

    // Compile with C++
    if (!compileWithCpp(merged, exePath, verbose, debug, emitCpp)) {
      return 1;
    }

    std::cout << "\033[1;32m   Finished\033[0m " << (debug ? "debug" : "release") << " target\n";
    std::cout << "    Binary: " << exePath << "\n";

    return 0;

  } catch (const std::exception &e) {
    std::cerr << "\033[1;31merror\033[0m: " << e.what() << "\n";
    return 1;
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printUsage();
    return 0;
  }

  std::string cmd = argv[1];
  bool verbose = false;
  bool debug = false;
  bool emitCpp = false;
  std::string outputFile;

  // Parse flags
  for (int i = 2; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "--verbose") {
      verbose = true;
    } else if (arg == "--debug") {
      debug = true;
    } else if (arg == "--release") {
      debug = false; // Explicit release mode
    } else if (arg == "--emit-cpp") {
      emitCpp = true;
    } else if (arg == "-o" && i + 1 < argc) {
      outputFile = argv[++i];
    }
  }

  if (cmd == "help" || cmd == "--help" || cmd == "-h") {
    printUsage();
    return 0;
  }

  if (cmd == "lsp") {
    MagolorLanguageServer server;
    server.run();
    return 0;
  }

  if (cmd == "build-project" || cmd == "build") {
    if (argc == 2 || fs::exists("project.toml")) {
      return buildProject(verbose, debug, emitCpp);
    }
  }

  if (argc < 3) {
    std::cerr << "\033[1;31merror\033[0m: missing source file\n";
    return 1;
  }

  std::string srcPath = argv[2];

  try {
    bool hasErrors = false;
    ModuleRegistry::instance().clear();

    Program prog = compileFile(srcPath, "", hasErrors, verbose);
    if (hasErrors) {
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
    
    if (!typeChecker.checkModule(module)) {
      typeCheckReporter.printDiagnostics();
      return 1;
    }

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
      
      std::cout << "\033[1;32m    Finished\033[0m " << outFile << "\n";
      return 0;
    }

    if (cmd == "check") {
      std::cout << "\033[1;32m    Checking\033[0m " << srcPath << "\n";
      std::cout << "\033[1;32m    Finished\033[0m no errors found\n";
      return 0;
    }

    // Compile with C++
    if (!compileWithCpp(prog, exePath, verbose, debug, emitCpp)) {
      return 1;
    }

    if (cmd == "run") {
      if (verbose) {
        std::cout << "\033[1;32mRunning\033[0m " << exePath << "\n\n";
      }
      std::string runCmd = "./" + exePath;
      int result = std::system(runCmd.c_str());
      fs::remove(exePath);
      if (!emitCpp) {
        fs::remove(exePath + ".cpp");
      }
      return result;
    }

    std::cout << "\033[1;32m   Finished\033[0m " << exePath << "\n";
    return 0;

  } catch (const std::exception &e) {
    std::cerr << "\033[1;31merror\033[0m: " << e.what() << "\n";
    return 1;
  }
}
