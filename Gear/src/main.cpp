#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;
void showHelp() {
  std::cout << "Gear - Magolor Project Manager\n";
  std::cout << "Usage:\n";
  std::cout << "  gear init      Initialize a new Magolor project\n";
  std::cout << "  gear build     Build the current project\n";
  std::cout << "  gear install   Install dependencies\n";
  std::cout << "  gear help      Show this help message\n";
}

bool is_directory_empty(const fs::path &path) {
  // First, check if the path actually exists and is a directory
  if (!fs::exists(path) || !fs::is_directory(path)) {
    // Handle error: not a directory or doesn't exist
    std::cerr << "Error: " << path
              << " is not a valid directory or does not exist." << std::endl;
    return false; // Or handle as appropriate for your application
  }

  // Use std::filesystem::is_empty to check contents
  return fs::is_empty(path);
}

bool writeToFile(const std::string &filePath, const std::string &content) {
  try {
    fs::path p(filePath);

    // Create parent directories if they don't exist
    if (p.has_parent_path()) {
      fs::create_directories(p.parent_path());
    }

    // Open the file in write mode
    std::ofstream outFile(p);

    if (!outFile.is_open()) {
      return false;
    }

    outFile << content;
    outFile.close();
    return true;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return false;
  }
}

void initProject(std::string name) {
  std::cout << "Initializing a new Magolor project...\n";

  if (!is_directory_empty(".")) {
    std::cerr << "Directory not empty. Aborting initialization.\n";
    return;
  }

  // Create folders
  fs::create_directory("src");
  fs::create_directory("target");

  // --- project.toml ---
  std::string tomlContent = "[project]\n"
                            "name = \"" +
                            name +
                            "\"\n"
                            "version = \"0.1.0\"\n"
                            "authors = [\"Your Name <you@example.com>\"]\n"
                            "description = \"A new Magolor project\"\n\n"
                            "[build]\n"
                            "source = \"src\"\n"
                            "output = \"target\"\n"
                            "target = \"debug\"\n"
                            "compiler_flags = []\n\n"
                            "[dependencies]\n\n"
                            "[metadata]\n"
                            "license = \"MIT\"\n"
                            "magolor_version = \">=0.1.0\"\n";

  if (!writeToFile("project.toml", tomlContent)) {
    std::cerr << "Failed to create project.toml\n";
    return;
  }

  // --- src/main.mg (Hello World) ---
  std::string mainMgContent = "using Std.IO;\n\n"
                              "fn main() {\n"
                              "    Std.print(\"Hello, Magolor world!\\n\");\n"
                              "}\n";

  if (!writeToFile("src/main.mg", mainMgContent)) {
    std::cerr << "Failed to create src/main.mg\n";
    return;
  }

  // --- .gitignore ---
  std::string gitignoreContent = "target/\n"
                                 "*.o\n"
                                 "*.obj\n"
                                 "*.exe\n"
                                 "*.log\n";

  if (!writeToFile(".gitignore", gitignoreContent)) {
    std::cerr << "Failed to create .gitignore!\n";
    return;
  }

  std::cout << "Project initialized successfully!\n";
  std::cout << "  - project.toml created\n";
  std::cout << "  - src/main.mg created (Hello World)\n";
  std::cout << "  - .gitignore created\n";
}

void buildProject() {
  std::cout << "Building project...\n";
  // TODO: integrate compiler, build steps, etc.
}

void installDependencies() {
  std::cout << "Installing dependencies...\n";
  // TODO: download packages, resolve dependencies
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    showHelp();
    return 0;
  }

  std::string command = argv[1];

  if (command == "help") {
    showHelp();
  } else if (command == "init") {
    fs::path current_path = fs::current_path();
    if (!argv[2]) {
      initProject(current_path.string());
    } else {
      std::string name = argv[2];
      initProject(name);
    }

  } else if (command == "build") {
    buildProject();
  } else if (command == "install") {
    installDependencies();
  } else {
    std::cout << "Unknown command: " << command << "\n";
    showHelp();
  }

  return 0;
}
