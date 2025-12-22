#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdlib>
#include "lexer.hpp"
#include "parser.hpp"
#include "codegen.hpp"

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
    std::cout << "Magolor Compiler v0.1.0\n";
    std::cout << "Usage: magolor <command> [options]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  build <file.mg>     Compile source to executable\n";
    std::cout << "  emit <file.mg>      Output generated C++ code\n";
    std::cout << "  run <file.mg>       Compile and run immediately\n";
    std::cout << "  help                Show this help\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage();
        return 1;
    }
    
    std::string cmd = argv[1];
    
    if (cmd == "help" || cmd == "--help" || cmd == "-h") {
        printUsage();
        return 0;
    }
    
    if (argc < 3) {
        std::cerr << "Error: Missing source file\n";
        return 1;
    }
    
    std::string srcPath = argv[2];
    
    try {
        // Read source
        std::string source = readFile(srcPath);
        
        // Lex
        Lexer lexer(source);
        auto tokens = lexer.tokenize();
        
        // Parse
        Parser parser(std::move(tokens));
        Program prog = parser.parse();
        
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
        
        // Write C++ file
        writeFile(cppPath, cppCode);
        
        // Compile with g++
        std::string compileCmd = "g++ -std=c++17 -O2 -o " + exePath + " " + cppPath;
        int result = std::system(compileCmd.c_str());
        
        if (result != 0) {
            std::cerr << "Compilation failed\n";
            return 1;
        }
        
        // Clean up generated C++ file
        fs::remove(cppPath);
        
        if (cmd == "run") {
            std::string runCmd = "./" + exePath;
            result = std::system(runCmd.c_str());
            fs::remove(exePath);
            return result;
        }
        
        std::cout << "Built: " << exePath << "\n";
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
