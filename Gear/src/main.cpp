#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <cstdlib>
#include <vector>
#include <set>
#include <algorithm>
namespace fs = std::filesystem;

void showHelp() {
  std::cout << "\033[1mGear - Magolor Project Manager v0.2.0\033[0m\n\n";
  std::cout << "\033[1mUSAGE:\033[0m\n";
  std::cout << "    gear [COMMAND] [OPTIONS]\n\n";
  std::cout << "\033[1mCOMMANDS:\033[0m\n";
  std::cout << "    init [name]         Initialize a new Magolor project\n";
  std::cout << "    build               Build the current project\n";
  std::cout << "    run                 Build and run the project\n";
  std::cout << "    clean               Remove build artifacts\n";
  std::cout << "    check               Check code for errors without building\n";
  std::cout << "    add <package>       Add a dependency to the project\n";
  std::cout << "    install             Install dependencies\n";
  std::cout << "    new <name>          Create a new Magolor project in a directory\n";
  std::cout << "    help                Show this help message\n\n";
  std::cout << "\033[1mOPTIONS:\033[0m\n";
  std::cout << "    --release           Build in release mode (optimized)\n";
  std::cout << "    --verbose           Show detailed build information\n";
}

bool is_directory_empty(const fs::path &path) {
  if (!fs::exists(path) || !fs::is_directory(path)) {
    std::cerr << "\033[1;31merror\033[0m: " << path
              << " is not a valid directory or does not exist." << std::endl;
    return false;
  }
  return fs::is_empty(path);
}

bool writeToFile(const std::string &filePath, const std::string &content) {
  try {
    fs::path p(filePath);
    if (p.has_parent_path()) {
      fs::create_directories(p.parent_path());
    }
    std::ofstream outFile(p);
    if (!outFile.is_open()) {
      return false;
    }
    outFile << content;
    outFile.close();
    return true;
  } catch (const std::exception &e) {
    std::cerr << "\033[1;31merror\033[0m: " << e.what() << std::endl;
    return false;
  }
}

void initProject(std::string name, const std::string& targetDir = ".") {
  std::cout << "\033[1;32m     Creating\033[0m " << name << " package\n";

  fs::path projDir = (targetDir == ".") ? fs::current_path() : fs::path(targetDir);
  
  if (targetDir != "." && fs::exists(projDir)) {
    std::cerr << "\033[1;31merror\033[0m: directory '" << targetDir << "' already exists\n";
    return;
  }
  
  if (targetDir != ".") {
    fs::create_directory(projDir);
  } else if (!is_directory_empty(projDir)) {
    std::cerr << "\033[1;31merror\033[0m: directory not empty\n";
    return;
  }

  // Create folders including modules directory
  fs::create_directory(projDir / "src");
  fs::create_directory(projDir / "src" / "modules");
  fs::create_directory(projDir / "target");

  // --- project.toml ---
  std::string tomlContent = "[project]\n"
                            "name = \"" + name + "\"\n"
                            "version = \"0.1.0\"\n"
                            "authors = [\"Your Name <you@example.com>\"]\n"
                            "description = \"A Magolor project\"\n"
                            "license = \"MIT\"\n\n"
                            "[dependencies]\n"
                            "# Add dependencies here\n"
                            "# example = \"1.0.0\"\n\n"
                            "[build]\n"
                            "optimization = \"2\"\n";

  if (!writeToFile((projDir / "project.toml").string(), tomlContent)) {
    std::cerr << "\033[1;31merror\033[0m: failed to create project.toml\n";
    return;
  }

  // --- src/main.mg (Hello World) ---
  std::string mainMgContent = "using Std.IO;\n"
                              "using modules.utils;\n\n"
                              "fn main() {\n"
                              "    Std.print(\"Hello, Magolor!\\n\");\n"
                              "    Std.print(\"Welcome to your new project: " + name + "\\n\");\n"
                              "    greet(\"" + name + "\");\n"
                              "}\n";

  if (!writeToFile((projDir / "src" / "main.mg").string(), mainMgContent)) {
    std::cerr << "\033[1;31merror\033[0m: failed to create src/main.mg\n";
    return;
  }

  // --- src/modules/utils.mg (Example module) ---
  std::string utilsMgContent = "using Std.IO;\n\n"
                              "pub fn greet(name: string) {\n"
                              "    Std.print($\"Greetings from {name}!\\n\");\n"
                              "}\n\n"
                              "pub fn add(a: int, b: int) -> int {\n"
                              "    return a + b;\n"
                              "}\n";

  if (!writeToFile((projDir / "src" / "modules" / "utils.mg").string(), utilsMgContent)) {
    std::cerr << "\033[1;31merror\033[0m: failed to create src/modules/utils.mg\n";
    return;
  }

  // --- .gitignore ---
  std::string gitignoreContent = "# Build artifacts\n"
                                 "target/\n"
                                 ".magolor/\n\n"
                                 "# OS files\n"
                                 ".DS_Store\n"
                                 "Thumbs.db\n\n"
                                 "# IDE\n"
                                 ".vscode/\n"
                                 ".idea/\n"
                                 "*.swp\n";

  if (!writeToFile((projDir / ".gitignore").string(), gitignoreContent)) {
    std::cerr << "\033[1;31merror\033[0m: failed to create .gitignore\n";
    return;
  }

  // --- README.md ---
  std::string readmeContent = "# " + name + "\n\n"
                             "A Magolor project\n\n"
                             "## Project Structure\n\n"
                             "```\n"
                             "├── src/\n"
                             "│   ├── main.mg          # Entry point\n"
                             "│   └── modules/         # Your modules\n"
                             "│       └── utils.mg     # Example module\n"
                             "├── target/              # Build output\n"
                             "└── project.toml         # Project configuration\n"
                             "```\n\n"
                             "## Building\n\n"
                             "```bash\n"
                             "gear build\n"
                             "```\n\n"
                             "## Running\n\n"
                             "```bash\n"
                             "gear run\n"
                             "```\n\n"
                             "## Module System\n\n"
                             "Create modules in `src/` or subdirectories:\n\n"
                             "```magolor\n"
                             "// src/modules/math.mg\n"
                             "pub fn add(a: int, b: int) -> int {\n"
                             "    return a + b;\n"
                             "}\n"
                             "```\n\n"
                             "Import and use in main.mg:\n\n"
                             "```magolor\n"
                             "using modules.math;\n\n"
                             "fn main() {\n"
                             "    let result = add(5, 3);\n"
                             "}\n"
                             "```\n";
  
  writeToFile((projDir / "README.md").string(), readmeContent);

  std::cout << "\033[1;32m     Created\033[0m binary (application) package\n";
  std::cout << "\033[1;36m        Note\033[0m: Multi-file project with module support\n";
}

std::vector<std::string> collectSourceFiles(const std::string& srcDir) {
  std::vector<std::string> files;
  std::set<std::string> uniqueFiles;
  
  if (!fs::exists(srcDir)) {
    return files;
  }
  
  // Recursively find all .mg files
  for (const auto& entry : fs::recursive_directory_iterator(srcDir)) {
    if (entry.is_regular_file() && entry.path().extension() == ".mg") {
      std::string filepath = entry.path().string();
      
      // Avoid duplicates
      if (uniqueFiles.find(filepath) == uniqueFiles.end()) {
        uniqueFiles.insert(filepath);
        files.push_back(filepath);
      }
    }
  }
  
  // Sort to ensure consistent build order (main.mg should be last for linking)
  std::sort(files.begin(), files.end(), [](const std::string& a, const std::string& b) {
    // main.mg should come last
    bool aIsMain = a.find("main.mg") != std::string::npos;
    bool bIsMain = b.find("main.mg") != std::string::npos;
    
    if (aIsMain && !bIsMain) return false;
    if (!aIsMain && bIsMain) return true;
    
    return a < b;
  });
  
  return files;
}


void buildProject(bool verbose = false) {
  if (!fs::exists("project.toml")) {
    std::cerr << "\033[1;31merror\033[0m: could not find project.toml\n";
    std::cerr << "  \033[1;34m= help:\033[0m initialize a project with 'gear init'\n";
    return;
  }

  // Parse project name
  std::ifstream toml("project.toml");
  std::string line, projectName;
  while (std::getline(toml, line)) {
    if (line.find("name =") != std::string::npos) {
      size_t start = line.find('"') + 1;
      size_t end = line.rfind('"');
      projectName = line.substr(start, end - start);
      break;
    }
  }

  if (projectName.empty()) {
    std::cerr << "\033[1;31merror\033[0m: could not determine project name\n";
    return;
  }

  std::vector<std::string> sourceFiles = collectSourceFiles("src");

  if (sourceFiles.empty()) {
    std::cerr << "\033[1;31merror\033[0m: no source files found in src/\n";
    return;
  }

  if (verbose) {
    std::cout << "\033[1;32m   Building\033[0m " << projectName << "\n";
    std::cout << "\033[1;32m   Compiling\033[0m " << sourceFiles.size() << " files\n";
    for (auto& f : sourceFiles) {
      std::cout << "             " << f << "\n";
    }
  }

  // Build command
  std::string buildCmd = "magolor build-project";

  for (const auto& file : sourceFiles) {
    buildCmd += " " + file;
  }

  if (verbose) {
    buildCmd += " --verbose";
  }

  int result = std::system(buildCmd.c_str());
  if (result != 0) {
    std::cerr << "\033[1;31merror\033[0m: build failed\n";
  }
}



void runProject(bool verbose = false) {
  if (!fs::exists("project.toml")) {
    std::cerr << "\033[1;31merror\033[0m: could not find project.toml\n";
    return;
  }

  // Parse project name
  std::ifstream toml("project.toml");
  std::string line, projectName;
  while (std::getline(toml, line)) {
    if (line.find("name =") != std::string::npos) {
      size_t start = line.find('"') + 1;
      size_t end = line.rfind('"');
      projectName = line.substr(start, end - start);
      break;
    }
  }

  if (projectName.empty()) {
    std::cerr << "\033[1;31merror\033[0m: could not determine project name\n";
    return;
  }

  // ALWAYS build
  buildProject(verbose);

  std::string exePath = "target/" + projectName;
  if (!fs::exists(exePath)) {
    std::cerr << "\033[1;31merror\033[0m: build succeeded but binary not found\n";
    return;
  }

  if (verbose) {
    std::cout << "\033[1;32m    Running\033[0m " << exePath << "\n\n";
  } else {
    std::cout << "\033[1;32m    Running\033[0m `" << exePath << "`\n";
  }

  std::system(("./" + exePath).c_str());
}


void cleanProject() {
  std::cout << "\033[1;32m    Cleaning\033[0m build artifacts\n";
  
  if (fs::exists("target")) {
    fs::remove_all("target");
    fs::create_directory("target");
    std::cout << "\033[1;32m     Removed\033[0m target/ directory\n";
  }
  
  if (fs::exists(".magolor")) {
    fs::remove_all(".magolor");
    std::cout << "\033[1;32m     Removed\033[0m .magolor/ directory\n";
  }
}


void checkProject() {
  if (!fs::exists("project.toml")) {
    std::cerr << "\033[1;31merror\033[0m: could not find project.toml\n";
    return;
  }

  std::vector<std::string> sourceFiles = collectSourceFiles("src");

  if (sourceFiles.empty()) {
    std::cerr << "\033[1;31merror\033[0m: no source files found\n";
    return;
  }

  bool hasErrors = false;

  for (const auto& file : sourceFiles) {
    std::string cmd = "magolor check " + file;
    int result = std::system(cmd.c_str());
    if (result != 0) {
      hasErrors = true;
    }
  }

  if (!hasErrors) {
    std::cout << "\033[1;32m    Finished\033[0m no errors found\n";
  }
}


void addDependency(const std::string& package) {
  if (!fs::exists("project.toml")) {
    std::cerr << "\033[1;31merror\033[0m: could not find project.toml\n";
    return;
  }

  std::cout << "\033[1;32m      Adding\033[0m " << package << " to dependencies\n";
  
  // Read existing project.toml
  std::ifstream inFile("project.toml");
  std::stringstream buffer;
  buffer << inFile.rdbuf();
  std::string content = buffer.str();
  inFile.close();

  // Find [dependencies] section
  size_t depsPos = content.find("[dependencies]");
  if (depsPos == std::string::npos) {
    content += "\n[dependencies]\n";
    depsPos = content.find("[dependencies]");
  }

  // Find next section or end
  size_t nextSection = content.find("\n[", depsPos + 14);
  size_t insertPos = (nextSection == std::string::npos) ? content.length() : nextSection;

  // Add dependency
  std::string depLine = package + " = \"*\"\n";
  content.insert(insertPos, depLine);

  // Write back
  std::ofstream outFile("project.toml");
  outFile << content;
  outFile.close();

  std::cout << "\033[1;32m       Added\033[0m " << package << "\n";
  std::cout << "  \033[1;34m= note:\033[0m run 'gear install' to fetch dependencies\n";
}

void installDependencies() {
  if (!fs::exists("project.toml")) {
    std::cerr << "\033[1;31merror\033[0m: could not find project.toml\n";
    return;
  }
  
  std::cout << "\033[1;32m  Installing\033[0m dependencies...\n";
  
  // Call magolor compiler's package manager
  std::string cmd = "magolor install-deps";
  int result = std::system(cmd.c_str());
  
  if (result != 0) {
    std::cerr << "\033[1;31m       Error\033[0m: failed to install dependencies\n";
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    showHelp();
    return 0;
  }

  std::string command = argv[1];
  bool verbose = false;

  // Check for flags
  for (int i = 2; i < argc; i++) {
    if (std::string(argv[i]) == "--verbose") {
      verbose = true;
    }
  }

  if (command == "help" || command == "--help" || command == "-h") {
    showHelp();
  } else if (command == "init") {
    fs::path current_path = fs::current_path();
    std::string name = (argc >= 3) ? argv[2] : current_path.filename().string();
    initProject(name);
  } else if (command == "new") {
    if (argc < 3) {
      std::cerr << "\033[1;31merror\033[0m: missing project name\n";
      std::cerr << "  \033[1;34m= help:\033[0m use 'gear new <name>'\n";
      return 1;
    }
    std::string name = argv[2];
    initProject(name, name);
  } else if (command == "build") {
    buildProject(verbose);
  } else if (command == "run") {
    runProject(verbose);
  } else if (command == "clean") {
    cleanProject();
  } else if (command == "check") {
    checkProject();
  } else if (command == "add") {
    if (argc < 3) {
      std::cerr << "\033[1;31merror\033[0m: missing package name\n";
      return 1;
    }
    addDependency(argv[2]);
  } else if (command == "install") {
    installDependencies();
  } else {
    std::cerr << "\033[1;31merror\033[0m: unknown command '" << command << "'\n";
    std::cerr << "  \033[1;34m= help:\033[0m use 'gear help' for available commands\n";
    return 1;
  }

  return 0;
}
