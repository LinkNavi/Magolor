// stdlib/network/router.mg
// Std.Network.Router - Advanced routing and middleware system

// ============================================================================
// Route Handler Types
// ============================================================================

pub class Route {
    pub method: string;
    pub pattern: string;
    pub isRegex: bool;
    
    pub fn create(method: string, pattern: string) {
        @cpp {
            this->method = method;
            this->pattern = pattern;
            this->isRegex = pattern.find('{') != std::string::npos;
        }
    }
    
    pub fn matches(req: Request) -> bool {
        @cpp {
            if (this->method != "*" && this->method != req.method) {
                return false;
            }
            
            if (!this->isRegex) {
                return this->pattern == req.path;
            }
            
            // Simple path parameter matching: /users/{id} matches /users/123
            std::string patternCopy = this->pattern;
            std::string pathCopy = req.path;
            
            // Replace {param} with regex
            size_t pos = 0;
            while ((pos = patternCopy.find('{', pos)) != std::string::npos) {
                size_t end = patternCopy.find('}', pos);
                if (end != std::string::npos) {
                    patternCopy.replace(pos, end - pos + 1, "([^/]+)");
                }
                pos++;
            }
            
            // Simple regex match
            std::regex pattern(patternCopy);
            return std::regex_match(pathCopy, pattern);
        }
    }
    
    pub fn extractParams(path: string) -> Map<string, string> {
        @cpp {
            std::unordered_map<std::string, std::string> params;
            
            if (!this->isRegex) return params;
            
            // Extract parameter names from pattern
            std::vector<std::string> paramNames;
            size_t pos = 0;
            while ((pos = this->pattern.find('{', pos)) != std::string::npos) {
                size_t end = this->pattern.find('}', pos);
                if (end != std::string::npos) {
                    paramNames.push_back(this->pattern.substr(pos + 1, end - pos - 1));
                    pos = end + 1;
                }
            }
            
            // Build regex pattern
            std::string regexPattern = this->pattern;
            pos = 0;
            while ((pos = regexPattern.find('{', pos)) != std::string::npos) {
                size_t end = regexPattern.find('}', pos);
                if (end != std::string::npos) {
                    regexPattern.replace(pos, end - pos + 1, "([^/]+)");
                }
                pos++;
            }
            
            // Match and extract values
            std::regex pattern(regexPattern);
            std::smatch matches;
            if (std::regex_match(path, matches, pattern)) {
                for (size_t i = 0; i < paramNames.size() && i + 1 < matches.size(); i++) {
                    params[paramNames[i]] = matches[i + 1].str();
                }
            }
            
            return params;
        }
    }
}

// ============================================================================
// Router - Advanced request routing
// ============================================================================

pub class Router {
    pub routes: Array<Route>;
    
    pub fn create() {
        @cpp {
            this->routes = std::vector<Route>();
        }
    }
    
    pub fn add(method: string, pattern: string) {
        @cpp {
            Route r;
            r.method = method;
            r.pattern = pattern;
            r.isRegex = pattern.find('{') != std::string::npos;
            this->routes.push_back(r);
        }
    }
    
    pub fn get(pattern: string) {
        @cpp {
            Route r;
            r.method = "GET";
            r.pattern = pattern;
            r.isRegex = pattern.find('{') != std::string::npos;
            this->routes.push_back(r);
        }
    }
    
    pub fn post(pattern: string) {
        @cpp {
            Route r;
            r.method = "POST";
            r.pattern = pattern;
            r.isRegex = pattern.find('{') != std::string::npos;
            this->routes.push_back(r);
        }
    }
    
    pub fn put(pattern: string) {
        @cpp {
            Route r;
            r.method = "PUT";
            r.pattern = pattern;
            r.isRegex = pattern.find('{') != std::string::npos;
            this->routes.push_back(r);
        }
    }
    
    pub fn del(pattern: string) {
        @cpp {
            Route r;
            r.method = "DELETE";
            r.pattern = pattern;
            r.isRegex = pattern.find('{') != std::string::npos;
            this->routes.push_back(r);
        }
    }
    
    pub fn findMatch(req: Request) -> Option<Route> {
        @cpp {
            for (const auto& route : this->routes) {
                if (route.matches(req)) {
                    return route;
                }
            }
            return std::nullopt;
        }
    }
}

// ============================================================================
// Middleware System
// ============================================================================

pub class Context {
    pub req: Request;
    pub res: Response;
    pub params: Map<string, string>;
    pub locals: Map<string, string>;
    
    pub fn create(req: Request) {
        @cpp {
            this->req = req;
            this->res = Response();
            this->res.create();
            this->params = std::unordered_map<std::string, std::string>();
            this->locals = std::unordered_map<std::string, std::string>();
        }
    }
    
    pub fn param(name: string) -> Option<string> {
        @cpp {
            auto it = this->params.find(name);
            if (it != this->params.end()) {
                return it->second;
            }
            return std::nullopt;
        }
    }
    
    pub fn setLocal(key: string, value: string) {
        @cpp {
            this->locals[key] = value;
        }
    }
    
    pub fn getLocal(key: string) -> Option<string> {
        @cpp {
            auto it = this->locals.find(key);
            if (it != this->locals.end()) {
                return it->second;
            }
            return std::nullopt;
        }
    }
}

// ============================================================================
// Static File Server
// ============================================================================

pub fn serveStatic(root: string) -> Response {
    @cpp {
        Response res;
        res.status = 501;
        res.body = "Static file serving not implemented";
        return res;
    }
}

pub class StaticServer {
    pub root: string;
    pub indexFiles: Array<string>;
    
    pub fn create(root: string) {
        @cpp {
            this->root = root;
            this->indexFiles = std::vector<std::string>{"index.html", "index.htm"};
        }
    }
    
    pub fn serve(path: string) -> Response {
        @cpp {
            Response res;
            
            // Security: prevent directory traversal
            if (path.find("..") != std::string::npos) {
                res.status = 403;
                res.body = "<h1>403 - Forbidden</h1>";
                return res;
            }
            
            std::string fullPath = this->root + path;
            
            // Check if path is a directory
            if (std::filesystem::is_directory(fullPath)) {
                // Try index files
                for (const auto& indexFile : this->indexFiles) {
                    std::string indexPath = fullPath + "/" + indexFile;
                    if (std::filesystem::exists(indexPath)) {
                        return HTTP::serveFile(indexPath);
                    }
                }
                
                // No index file found
                res.status = 403;
                res.body = "<h1>403 - Directory listing forbidden</h1>";
                return res;
            }
            
            return HTTP::serveFile(fullPath);
        }
    }
}

// ============================================================================
// CORS Middleware
// ============================================================================

pub class CORS {
    pub allowOrigin: string;
    pub allowMethods: string;
    pub allowHeaders: string;
    
    pub fn create() {
        @cpp {
            this->allowOrigin = "*";
            this->allowMethods = "GET, POST, PUT, DELETE, OPTIONS";
            this->allowHeaders = "Content-Type, Authorization";
        }
    }
    
    pub fn apply(res: Response) {
        @cpp {
            res.headers["Access-Control-Allow-Origin"] = this->allowOrigin;
            res.headers["Access-Control-Allow-Methods"] = this->allowMethods;
            res.headers["Access-Control-Allow-Headers"] = this->allowHeaders;
        }
    }
}

// ============================================================================
// JSON Helper
// ============================================================================

pub fn jsonEncode(map: Map<string, string>) -> string {
    @cpp {
        std::ostringstream json;
        json << "{";
        bool first = true;
        for (const auto& [key, value] : map) {
            if (!first) json << ",";
            first = false;
            json << "\"" << key << "\":\"" << value << "\"";
        }
        json << "}";
        return json.str();
    }
}

pub fn jsonDecode(json: string) -> Map<string, string> {
    @cpp {
        std::unordered_map<std::string, std::string> result;
        
        // Simple JSON parser (supports only string key-value pairs)
        size_t pos = json.find('{');
        if (pos == std::string::npos) return result;
        
        pos++;
        while (pos < json.length()) {
            // Skip whitespace
            while (pos < json.length() && std::isspace(json[pos])) pos++;
            
            // Check for end
            if (json[pos] == '}') break;
            
            // Parse key
            if (json[pos] != '"') break;
            pos++;
            size_t keyStart = pos;
            while (pos < json.length() && json[pos] != '"') pos++;
            std::string key = json.substr(keyStart, pos - keyStart);
            pos++;
            
            // Skip colon
            while (pos < json.length() && (json[pos] == ':' || std::isspace(json[pos]))) pos++;
            
            // Parse value
            if (json[pos] != '"') break;
            pos++;
            size_t valStart = pos;
            while (pos < json.length() && json[pos] != '"') pos++;
            std::string value = json.substr(valStart, pos - valStart);
            pos++;
            
            result[key] = value;
            
            // Skip comma
            while (pos < json.length() && (json[pos] == ',' || std::isspace(json[pos]))) pos++;
        }
        
        return result;
    }
}
