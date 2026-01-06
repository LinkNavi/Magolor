// Std.File - File system operations
// File I/O, paths, and directory operations

using Std.Core.Prelude;

// ============================================================================
// File existence and type checks
// ============================================================================

pub fn exists(path: string) -> bool {
    @cpp { return std::filesystem::exists(path); }
}

pub fn isFile(path: string) -> bool {
    @cpp { return std::filesystem::is_regular_file(path); }
}

pub fn isDirectory(path: string) -> bool {
    @cpp { return std::filesystem::is_directory(path); }
}

pub fn isSymlink(path: string) -> bool {
    @cpp { return std::filesystem::is_symlink(path); }
}

pub fn isReadable(path: string) -> bool {
    @cpp {
        auto perms = std::filesystem::status(path).permissions();
        return (perms & std::filesystem::perms::owner_read) != std::filesystem::perms::none;
    }
}

pub fn isWritable(path: string) -> bool {
    @cpp {
        auto perms = std::filesystem::status(path).permissions();
        return (perms & std::filesystem::perms::owner_write) != std::filesystem::perms::none;
    }
}

// ============================================================================
// Simple file I/O
// ============================================================================

pub fn readFile(path: string) -> Option<string> {
    @cpp {
        std::ifstream file(path);
        if (!file) return std::nullopt;
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
}

pub fn writeFile(path: string, content: string) -> bool {
    @cpp {
        std::ofstream file(path);
        if (!file) return false;
        file << content;
        return true;
    }
}

pub fn appendFile(path: string, content: string) -> bool {
    @cpp {
        std::ofstream file(path, std::ios::app);
        if (!file) return false;
        file << content;
        return true;
    }
}

pub fn readLines(path: string) -> Option<Array<string>> {
    @cpp {
        std::ifstream file(path);
        if (!file) return std::nullopt;
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(file, line)) {
            lines.push_back(line);
        }
        return lines;
    }
}

pub fn writeLines(path: string, lines: Array<string>) -> bool {
    @cpp {
        std::ofstream file(path);
        if (!file) return false;
        for (const auto& line : lines) {
            file << line << "\n";
        }
        return true;
    }
}

// ============================================================================
// Binary file I/O
// ============================================================================

pub fn readBytes(path: string) -> Option<Array<int>> {
    @cpp {
        std::ifstream file(path, std::ios::binary);
        if (!file) return std::nullopt;
        std::vector<int> bytes;
        char byte;
        while (file.get(byte)) {
            bytes.push_back(static_cast<unsigned char>(byte));
        }
        return bytes;
    }
}

pub fn writeBytes(path: string, bytes: Array<int>) -> bool {
    @cpp {
        std::ofstream file(path, std::ios::binary);
        if (!file) return false;
        for (int b : bytes) {
            file.put(static_cast<char>(b));
        }
        return true;
    }
}

// ============================================================================
// File metadata
// ============================================================================

pub fn size(path: string) -> Option<int> {
    @cpp {
        try {
            return std::make_optional(static_cast<int>(std::filesystem::file_size(path)));
        } catch (...) {
            return std::nullopt;
        }
    }
}

pub fn modifiedTime(path: string) -> Option<int> {
    @cpp {
        try {
            auto ftime = std::filesystem::last_write_time(path);
            auto sctp = std::chrono::time_point_cast<std::chrono::seconds>(
                std::chrono::file_clock::to_sys(ftime));
            return std::make_optional(static_cast<int>(sctp.time_since_epoch().count()));
        } catch (...) {
            return std::nullopt;
        }
    }
}

// ============================================================================
// File operations
// ============================================================================

pub fn copy(from: string, to: string) -> bool {
    @cpp {
        try {
            std::filesystem::copy(from, to, std::filesystem::copy_options::overwrite_existing);
            return true;
        } catch (...) {
            return false;
        }
    }
}

pub fn move(from: string, to: string) -> bool {
    @cpp {
        try {
            std::filesystem::rename(from, to);
            return true;
        } catch (...) {
            return false;
        }
    }
}

pub fn rename(from: string, to: string) -> bool {
    return move(from, to);
}

pub fn remove(path: string) -> bool {
    @cpp {
        try {
            return std::filesystem::remove(path);
        } catch (...) {
            return false;
        }
    }
}

pub fn removeAll(path: string) -> int {
    @cpp {
        try {
            return static_cast<int>(std::filesystem::remove_all(path));
        } catch (...) {
            return 0;
        }
    }
}

// ============================================================================
// Directory operations
// ============================================================================

pub fn createDir(path: string) -> bool {
    @cpp {
        try {
            return std::filesystem::create_directory(path);
        } catch (...) {
            return false;
        }
    }
}

pub fn createDirs(path: string) -> bool {
    @cpp {
        try {
            return std::filesystem::create_directories(path);
        } catch (...) {
            return false;
        }
    }
}

pub fn listDir(path: string) -> Option<Array<string>> {
    @cpp {
        try {
            std::vector<std::string> entries;
            for (const auto& entry : std::filesystem::directory_iterator(path)) {
                entries.push_back(entry.path().filename().string());
            }
            return entries;
        } catch (...) {
            return std::nullopt;
        }
    }
}

pub fn listDirRecursive(path: string) -> Option<Array<string>> {
    @cpp {
        try {
            std::vector<std::string> entries;
            for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
                entries.push_back(entry.path().string());
            }
            return entries;
        } catch (...) {
            return std::nullopt;
        }
    }
}

pub fn glob(path: string, pattern: string) -> Array<string> {
    @cpp {
        std::vector<std::string> matches;
        try {
            for (const auto& entry : std::filesystem::directory_iterator(path)) {
                std::string name = entry.path().filename().string();
                // Simple wildcard matching
                if (pattern == "*" || name.find(pattern.substr(1)) != std::string::npos) {
                    matches.push_back(entry.path().string());
                }
            }
        } catch (...) {}
        return matches;
    }
}

// ============================================================================
// Path operations
// ============================================================================

pub fn absolutePath(path: string) -> string {
    @cpp { return std::filesystem::absolute(path).string(); }
}

pub fn canonicalPath(path: string) -> Option<string> {
    @cpp {
        try {
            return std::filesystem::canonical(path).string();
        } catch (...) {
            return std::nullopt;
        }
    }
}

pub fn relativePath(path: string, base: string) -> string {
    @cpp { return std::filesystem::relative(path, base).string(); }
}

pub fn parentPath(path: string) -> string {
    @cpp { return std::filesystem::path(path).parent_path().string(); }
}

pub fn fileName(path: string) -> string {
    @cpp { return std::filesystem::path(path).filename().string(); }
}

pub fn stem(path: string) -> string {
    @cpp { return std::filesystem::path(path).stem().string(); }
}

pub fn extension(path: string) -> string {
    @cpp { return std::filesystem::path(path).extension().string(); }
}

pub fn joinPath(parts: Array<string>) -> string {
    @cpp {
        if (parts.empty()) return "";
        std::filesystem::path result = parts[0];
        for (size_t i = 1; i < parts.size(); i++) {
            result /= parts[i];
        }
        return result.string();
    }
}

pub fn normalizePath(path: string) -> string {
    @cpp { return std::filesystem::path(path).lexically_normal().string(); }
}

// ============================================================================
// Temp files
// ============================================================================

pub fn tempDir() -> string {
    @cpp { return std::filesystem::temp_directory_path().string(); }
}

pub fn createTempFile(prefix: string) -> Option<string> {
    @cpp {
        try {
            auto temp = std::filesystem::temp_directory_path() / (prefix + "XXXXXX");
            std::string path = temp.string();
            int fd = mkstemp(&path[0]);
            if (fd == -1) return std::nullopt;
            close(fd);
            return path;
        } catch (...) {
            return std::nullopt;
        }
    }
}

// ============================================================================
// Working directory
// ============================================================================

pub fn cwd() -> string {
    @cpp { return std::filesystem::current_path().string(); }
}

pub fn chdir(path: string) -> bool {
    @cpp {
        try {
            std::filesystem::current_path(path);
            return true;
        } catch (...) {
            return false;
        }
    }
}
