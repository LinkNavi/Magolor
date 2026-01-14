// Std.File - File system operations

pub fn exists(path: string) -> bool {
    @cpp {
        return std::filesystem::exists(path);
    }
}

pub fn isFile(path: string) -> bool {
    @cpp {
        return std::filesystem::is_regular_file(path);
    }
}

pub fn isDirectory(path: string) -> bool {
    @cpp {
        return std::filesystem::is_directory(path);
    }
}

pub fn size(path: string) -> int {
    @cpp {
        if (!std::filesystem::exists(path)) return -1;
        return static_cast<int64_t>(std::filesystem::file_size(path));
    }
}

pub fn read(path: string) -> Option<string> {
    @cpp {
        std::ifstream file(path);
        if (!file) return std::nullopt;
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
}

pub fn write(path: string, content: string) -> bool {
    @cpp {
        std::ofstream file(path);
        if (!file) return false;
        file << content;
        return true;
    }
}

pub fn append(path: string, content: string) -> bool {
    @cpp {
        std::ofstream file(path, std::ios::app);
        if (!file) return false;
        file << content;
        return true;
    }
}

pub fn remove(path: string) -> bool {
    @cpp {
        return std::filesystem::remove(path);
    }
}

pub fn createDir(path: string) -> bool {
    @cpp {
        return std::filesystem::create_directories(path);
    }
}

pub fn copy(src: string, dst: string) -> bool {
    @cpp {
        try {
            std::filesystem::copy(src, dst, std::filesystem::copy_options::overwrite_existing);
            return true;
        } catch (...) {
            return false;
        }
    }
}

pub fn move(src: string, dst: string) -> bool {
    @cpp {
        try {
            std::filesystem::rename(src, dst);
            return true;
        } catch (...) {
            return false;
        }
    }
}

pub fn absolutePath(path: string) -> string {
    @cpp {
        return std::filesystem::absolute(path).string();
    }
}

pub fn parentDir(path: string) -> string {
    @cpp {
        return std::filesystem::path(path).parent_path().string();
    }
}

pub fn fileName(path: string) -> string {
    @cpp {
        return std::filesystem::path(path).filename().string();
    }
}

pub fn extension(path: string) -> string {
    @cpp {
        return std::filesystem::path(path).extension().string();
    }
}

pub fn tempDir() -> string {
    @cpp {
        return std::filesystem::temp_directory_path().string();
    }
}

pub fn createTempFile(prefix: string) -> Option<string> {
    @cpp {
        std::string dir = std::filesystem::temp_directory_path().string();
        std::string path = dir + "/" + prefix + "_XXXXXX";
        std::vector<char> buf(path.begin(), path.end());
        buf.push_back('\0');
        int fd = mkstemp(buf.data());
        if (fd == -1) return std::nullopt;
        close(fd);
        return std::string(buf.data());
    }
}

pub fn cwd() -> string {
    @cpp {
        return std::filesystem::current_path().string();
    }
}

pub fn setCwd(path: string) -> bool {
    @cpp {
        try {
            std::filesystem::current_path(path);
            return true;
        } catch (...) {
            return false;
        }
    }
}

pub fn listDir(path: string) -> Array<string> {
    @cpp {
        std::vector<std::string> entries;
        if (!std::filesystem::is_directory(path)) return entries;
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            entries.push_back(entry.path().filename().string());
        }
        return entries;
    }
}

pub fn readBytes(path: string) -> Array<int> {
    @cpp {
        std::vector<int64_t> result;
        std::ifstream file(path, std::ios::binary);
        if (!file) return result;
        char byte;
        while (file.get(byte)) {
            result.push_back(static_cast<unsigned char>(byte));
        }
        return result;
    }
}

pub fn writeBytes(path: string, data: Array<int>) -> bool {
    @cpp {
        std::ofstream file(path, std::ios::binary);
        if (!file) return false;
        for (auto b : data) {
            file.put(static_cast<char>(b));
        }
        return true;
    }
}
