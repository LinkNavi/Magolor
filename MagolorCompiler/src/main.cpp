// Updated main.cpp with LLVM backend support

#include "llvm_codegen.hpp"
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
  std::cout << "\033[1mMagolor Compiler v0.2.0 (LLVM Backend)\033[0m\n\n";
  std::cout << "\033[1mUSAGE:\033[0m\n";
  std::cout << "    magolor [COMMAND] [OPTIONS]\n\n";
  std::cout << "\033[1mCOMMANDS:\033[0m\n";
  std::cout << "    build [file.mg]     Compile source file to executable\n";
  std::cout << "    build-project       Build entire project (uses project.toml)\n";
  std::cout << "    emit-llvm <file>    Output LLVM IR\n";
  std::cout << "    emit-asm <file>     Output assembly\n";
  std::cout << "    run <file.mg>       Compile and run immediately\n";
  std::cout << "    check <file.mg>     Check for errors without building\n";
  std::cout << "    lsp                 Start Language Server\n";
  std::cout << "    help                Show this help\n\n";
  std::cout << "\033[1mOPTIONS:\033[0m\n";
  std::cout << "    -o <file>          Specify output file name\n";
  std::cout << "    --verbose          Show detailed compilation steps\n";
  std::cout << "    --emit-llvm        Also emit LLVM IR file\n";
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

bool compileWithLLVM(const Program& prog, const std::string& outputFile, bool verbose, bool emitIR) {
    if (verbose) {
        std::cout << "\033[1;32m   Generating\033[0m LLVM IR\n";
    }
    
    // Generate LLVM IR
    LLVMCodeGen codegen(outputFile);
    if (!codegen.generate(prog)) {
        std::cerr << "\033[1;31merror\033[0m: LLVM code generation failed\n";
        return false;
    }
    
    // Optionally emit LLVM IR
    if (emitIR) {
        std::string irFile = outputFile + ".ll";
        codegen.emitLLVMIR(irFile);
        if (verbose) {
            std::cout << "\033[1;32m       Saved\033[0m " << irFile << "\n";
        }
    }
    
    // Emit object file
    std::string objFile = outputFile + ".o";
    if (!codegen.emitObjectFile(objFile)) {
        std::cerr << "\033[1;31merror\033[0m: Failed to emit object file\n";
        return false;
    }
    
    if (verbose) {
        std::cout << "\033[1;32m    Linking\033[0m with runtime library\n";
    }
    
    // Compile runtime library
    std::string runtimeObj = "runtime.o";
    std::string compileRuntime = "gcc -c runtime.c -o " + runtimeObj + " 2>&1";
    if (std::system(compileRuntime.c_str()) != 0) {
        std::cerr << "\033[1;31merror\033[0m: Failed to compile runtime library\n";
        return false;
    }
    
    // Link everything together
    std::string linkCmd = "gcc " + objFile + " " + runtimeObj + 
                         " -o " + outputFile + " -lm 2>&1";
    
    FILE* pipe = popen(linkCmd.c_str(), "r");
    if (!pipe) {
        std::cerr << "\033[1;31merror\033[0m: Failed to run linker\n";
        return false;
    }
    
    char buffer[128];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    int returnCode = pclose(pipe);
    
    if (returnCode != 0) {
        std::cerr << result;
        std::cerr << "\033[1;31merror\033[0m: Linking failed\n";
        return false;
    }
    
    // Clean up intermediate files
    fs::remove(objFile);
    fs::remove(runtimeObj);
    
    if (verbose) {
        std::cout << "\033[1;32m   Finished\033[0m " << outputFile << "\n";
    }
    
    return true;
}

int buildProject(bool verbose = false, bool emitIR = false) {
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

    // Compile with LLVM
    if (!compileWithLLVM(merged, exePath, verbose, emitIR)) {
      return 1;
    }

    std::cout << "\033[1;32m   Finished\033[0m release target(s)\n";
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
  bool emitIR = false;

  // Check for flags
  for (int i = 2; i < argc; i++) {
    if (std::string(argv[i]) == "--verbose") {
      verbose = true;
    } else if (std::string(argv[i]) == "--emit-llvm") {
      emitIR = true;
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
      return buildProject(verbose, emitIR);
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
    std::string exePath = baseName;

    if (cmd == "emit-llvm") {
      LLVMCodeGen codegen(baseName);
      codegen.generate(prog);
      codegen.emitLLVMIR(baseName + ".ll");
      std::cout << "\033[1;32m    Finished\033[0m " << baseName << ".ll\n";
      return 0;
    }

    if (cmd == "emit-asm") {
      LLVMCodeGen codegen(baseName);
      codegen.generate(prog);
      codegen.emitAssembly(baseName + ".s");
      std::cout << "\033[1;32m    Finished\033[0m " << baseName << ".s\n";
      return 0;
    }

    if (cmd == "check") {
      std::cout << "\033[1;32m    Checking\033[0m " << srcPath << "\n";
      std::cout << "\033[1;32m    Finished\033[0m no errors found\n";
      return 0;
    }

    // Compile with LLVM
    if (!compileWithLLVM(prog, exePath, verbose, emitIR)) {
      return 1;
    }

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

  } catch (const std::exception &e) {
    std::cerr << "\033[1;31merror\033[0m: " << e.what() << "\n";
    return 1;
  }
}
