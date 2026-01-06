// Std.System - System and process operations
// Environment, processes, and system info

using Std.Core.Prelude;

// ============================================================================
// Process control
// ============================================================================

pub fn exit(code: int) {
    @cpp { std::exit(code); }
}

pub fn abort() {
    @cpp { std::abort(); }
}

// ============================================================================
// Environment variables
// ============================================================================

pub fn getEnv(name: string) -> Option<string> {
    @cpp {
        const char* val = std::getenv(name.c_str());
        if (val) return std::make_optional(std::string(val));
        return std::nullopt;
    }
}

pub fn getEnvOr(name: string, defaultVal: string) -> string {
    @cpp {
        const char* val = std::getenv(name.c_str());
        if (val) return std::string(val);
        return defaultVal;
    }
}

pub fn setEnv(name: string, value: string) -> bool {
    @cpp {
        return setenv(name.c_str(), value.c_str(), 1) == 0;
    }
}

pub fn unsetEnv(name: string) -> bool {
    @cpp {
        return unsetenv(name.c_str()) == 0;
    }
}

// ============================================================================
// Command execution
// ============================================================================

pub fn execute(command: string) -> int {
    @cpp { return std::system(command.c_str()); }
}

pub fn executeCapture(command: string) -> Option<string> {
    @cpp {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) return std::nullopt;
        
        std::string result;
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        
        int status = pclose(pipe);
        if (status != 0) return std::nullopt;
        return result;
    }
}

pub class CommandResult {
    pub exitCode: int;
    pub stdout: string;
    pub stderr: string;
    pub success: bool;
}

pub fn run(command: string) -> CommandResult {
    @cpp {
        CommandResult result;
        result.exitCode = 0;
        result.stdout = "";
        result.stderr = "";
        result.success = false;
        
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) return result;
        
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result.stdout += buffer;
        }
        
        result.exitCode = pclose(pipe);
        result.success = (result.exitCode == 0);
        return result;
    }
}

// ============================================================================
// System information
// ============================================================================

pub fn platform() -> string {
    @cpp {
        #if defined(__APPLE__)
        return "macos";
        #elif defined(__linux__)
        return "linux";
        #else
        return "unknown";
        #endif
    }
}

pub fn arch() -> string {
    @cpp {
        #if defined(__x86_64__)
        return "x86_64";
        #elif defined(__aarch64__)
        return "arm64";
        #else
        return "unknown";
        #endif
    }
}

pub fn cpuCount() -> int {
    @cpp {
        return std::thread::hardware_concurrency();
    }
}

pub fn hostname() -> Option<string> {
    @cpp {
        char hostname[256];
        if (gethostname(hostname, sizeof(hostname)) == 0) {
            return std::make_optional(std::string(hostname));
        }
        return std::nullopt;
    }
}

pub fn username() -> Option<string> {
    @cpp {
        const char* user = std::getenv("USER");
        if (user) return std::make_optional(std::string(user));
        return std::nullopt;
    }
}

pub fn homeDir() -> Option<string> {
    @cpp {
        const char* home = std::getenv("HOME");
        if (home) return std::make_optional(std::string(home));
        return std::nullopt;
    }
}

pub fn pid() -> int {
    @cpp { return getpid(); }
}

pub fn ppid() -> int {
    @cpp { return getppid(); }
}
