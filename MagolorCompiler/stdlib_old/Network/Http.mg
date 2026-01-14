// stdlib/Network/http.mg
// Std.Network.HTTP - HTTP server and client implementation

@link { -lpthread }
@include { <sys/socket.h> <netinet/in.h> <arpa/inet.h> <unistd.h> <fcntl.h> }

// ============================================================================
// HTTP Status Codes
// ============================================================================

pub class Status {
    pub static OK: int = 200;
    pub static CREATED: int = 201;
    pub static NO_CONTENT: int = 204;
    pub static BAD_REQUEST: int = 400;
    pub static NOT_FOUND: int = 404;
    pub static INTERNAL_ERROR: int = 500;
}

// ============================================================================
// HTTP Request
// ============================================================================

pub class Request {
    pub method: string;
    pub path: string;
    pub headers: Map<string, string>;
    pub body: string;
    
    pub fn create() {
        @cpp {
            this->method = "GET";
            this->path = "/";
            this->headers = std::unordered_map<std::string, std::string>();
            this->body = "";
        }
    }
}

// ============================================================================
// HTTP Response
// ============================================================================

pub class Response {
    pub status: int;
    pub headers: Map<string, string>;
    pub body: string;
    
    pub fn create() {
        @cpp {
            this->status = 200;
            this->headers = std::unordered_map<std::string, std::string>();
            this->headers["Content-Type"] = "text/html; charset=utf-8";
            this->body = "";
        }
    }
    
    pub fn toWire() -> string {
        @cpp {
            std::ostringstream ss;
            
            // Status line
            ss << "HTTP/1.1 " << this->status << " ";
            switch (this->status) {
                case 200: ss << "OK"; break;
                case 201: ss << "Created"; break;
                case 400: ss << "Bad Request"; break;
                case 404: ss << "Not Found"; break;
                case 500: ss << "Internal Server Error"; break;
                default: ss << "Unknown"; break;
            }
            ss << "\r\n";
            
            // Headers
            this->headers["Content-Length"] = std::to_string(this->body.length());
            for (const auto& [key, value] : this->headers) {
                ss << key << ": " << value << "\r\n";
            }
            ss << "\r\n";
            
            // Body
            ss << this->body;
            
            return ss.str();
        }
    }
}

// ============================================================================
// Response Builders
// ============================================================================

pub fn htmlResponse(html: string, statusCode: int) -> Response {
    @cpp {
        Response res;
        res.status = statusCode;
        res.headers["Content-Type"] = "text/html; charset=utf-8";
        res.body = html;
        return res;
    }
}

pub fn jsonResponse(json: string, statusCode: int) -> Response {
    @cpp {
        Response res;
        res.status = statusCode;
        res.headers["Content-Type"] = "application/json";
        res.body = json;
        return res;
    }
}

pub fn serveFile(path: string) -> Response {
    @cpp {
        Response res;
        
        // Read file
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            res.status = 404;
            res.headers["Content-Type"] = "text/html";
            res.body = "<h1>404 - File Not Found</h1>";
            return res;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        res.body = buffer.str();
        
        // Determine content type
        std::string ext = path.substr(path.find_last_of('.') + 1);
        if (ext == "html" || ext == "htm") {
            res.headers["Content-Type"] = "text/html; charset=utf-8";
        } else if (ext == "css") {
            res.headers["Content-Type"] = "text/css";
        } else if (ext == "js") {
            res.headers["Content-Type"] = "application/javascript";
        } else if (ext == "json") {
            res.headers["Content-Type"] = "application/json";
        } else {
            res.headers["Content-Type"] = "application/octet-stream";
        }
        
        res.status = 200;
        return res;
    }
}

// ============================================================================
// HTTP Parser
// ============================================================================

pub fn parseRequest(data: string) -> Option<Request> {
    @cpp {
        Request req;
        
        // Split into lines
        std::istringstream stream(data);
        std::string line;
        
        // Parse request line
        if (!std::getline(stream, line)) return std::nullopt;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        
        std::istringstream reqLine(line);
        std::string version;
        reqLine >> req.method >> req.path >> version;
        
        // Parse headers
        while (std::getline(stream, line) && line != "\r" && !line.empty()) {
            if (line.back() == '\r') line.pop_back();
            
            size_t colon = line.find(':');
            if (colon != std::string::npos) {
                std::string key = line.substr(0, colon);
                std::string value = line.substr(colon + 1);
                
                // Trim whitespace
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
                
                req.headers[key] = value;
            }
        }
        
        // Parse body
        std::string bodyData;
        while (std::getline(stream, line)) {
            bodyData += line + "\n";
        }
        if (!bodyData.empty() && bodyData.back() == '\n') {
            bodyData.pop_back();
        }
        req.body = bodyData;
        
        return req;
    }
}

// ============================================================================
// HTTP Server
// ============================================================================

pub class HttpServer {
    pub port: int;
    
    pub fn create(serverPort: int) {
        @cpp {
            this->port = serverPort;
        }
    }
    
    pub fn start() {
        @cpp {
            // Create socket
            int server_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (server_fd < 0) {
                std::cerr << "Failed to create socket\n";
                return;
            }
            
            // Set socket options
            int opt = 1;
            setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
            
            // Bind
            struct sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = INADDR_ANY;
            addr.sin_port = htons(this->port);
            
            if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
                std::cerr << "Failed to bind to port " << this->port << "\n";
                close(server_fd);
                return;
            }
            
            // Listen
            if (listen(server_fd, 128) < 0) {
                std::cerr << "Failed to listen\n";
                close(server_fd);
                return;
            }
            
            std::cout << "Server listening on http://localhost:" << this->port << "\n";
            
            // Accept loop
            while (true) {
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                
                int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
                if (client_fd < 0) {
                    continue;
                }
                
                // Read request
                char buffer[8192];
                ssize_t n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
                if (n > 0) {
                    buffer[n] = '\0';
                    std::string requestData(buffer, n);
                    
                    // Parse request
                    auto reqOpt = parseRequest(requestData);
                    if (reqOpt.has_value()) {
                        Request req = reqOpt.value();
                        
                        // Generate response
                        Response res;
                        res.status = 200;
                        res.body = "<h1>Hello from Magolor!</h1><p>Path: " + req.path + "</p>";
                        res.headers["Content-Type"] = "text/html";
                        
                        // Send response
                        std::string wire = res.toWire();
                        send(client_fd, wire.c_str(), wire.length(), 0);
                    }
                }
                
                close(client_fd);
            }
            
            close(server_fd);
        }
    }
}
