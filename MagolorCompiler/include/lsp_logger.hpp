// lsp_logger.hpp
#pragma once
#include <fstream>
#include <string>

class LSPLogger {
private:
    std::ofstream logFile;
public:
    LSPLogger() {
        logFile.open("/tmp/magolor-lsp.log", std::ios::app);
        logFile << "\n=== LSP Server Started ===" << std::endl;
    }
    ~LSPLogger() {
        logFile << "=== LSP Server Stopped ===" << std::endl;
        logFile.close();
    }
    void log(const std::string& msg) {
        logFile << msg << std::endl;
        logFile.flush();
    }
};

extern LSPLogger logger;
