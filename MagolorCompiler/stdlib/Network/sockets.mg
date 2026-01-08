// stdlib/network/sockets.mg
// Std.Network.Sockets - Low-level TCP/UDP socket operations

@link { -lpthread }
@include { <sys/socket.h> <netinet/in.h> <arpa/inet.h> <netdb.h> <unistd.h> <fcntl.h> <poll.h> <errno.h> }

// ============================================================================
// Socket Result
// ============================================================================

pub class SocketResult {
    pub success: bool;
    pub error: string;
    pub data: Array<int>;
    
    pub fn create() {
        @cpp {
            this->success = false;
            this->error = "";
            this->data = std::vector<int64_t>();
        }
    }
}

// ============================================================================
// TCP Socket
// ============================================================================

pub class TcpSocket {
    pub fd: int;
    pub connected: bool;
    pub host: string;
    pub port: int;
    
    pub fn create() {
        @cpp {
            this->fd = -1;
            this->connected = false;
            this->host = "";
            this->port = 0;
        }
    }
    
    pub fn connect(host: string, port: int) -> bool {
        @cpp {
            // Create socket
            this->fd = socket(AF_INET, SOCK_STREAM, 0);
            if (this->fd < 0) {
                return false;
            }
            
            // Resolve host
            struct hostent* server = gethostbyname(host.c_str());
            if (!server) {
                close(this->fd);
                this->fd = -1;
                return false;
            }
            
            // Setup address
            struct sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            memcpy(&addr.sin_addr.s_addr, server->h_addr, server->h_length);
            
            // Connect
            if (::connect(this->fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
                close(this->fd);
                this->fd = -1;
                return false;
            }
            
            this->connected = true;
            this->host = host;
            this->port = port;
            return true;
        }
    }
    
    pub fn send(data: Array<int>) -> int {
        @cpp {
            if (!this->connected || this->fd < 0) return -1;
            
            std::vector<unsigned char> bytes(data.size());
            for (size_t i = 0; i < data.size(); i++) {
                bytes[i] = static_cast<unsigned char>(data[i]);
            }
            
            ssize_t sent = ::send(this->fd, bytes.data(), bytes.size(), 0);
            return static_cast<int64_t>(sent);
        }
    }
    
    pub fn sendString(str: string) -> int {
        @cpp {
            if (!this->connected || this->fd < 0) return -1;
            
            ssize_t sent = ::send(this->fd, str.c_str(), str.length(), 0);
            return static_cast<int64_t>(sent);
        }
    }
    
    pub fn recv(maxBytes: int) -> Array<int> {
        @cpp {
            std::vector<int64_t> result;
            if (!this->connected || this->fd < 0) return result;
            
            std::vector<unsigned char> buffer(maxBytes);
            ssize_t n = ::recv(this->fd, buffer.data(), buffer.size(), 0);
            
            if (n > 0) {
                result.resize(n);
                for (ssize_t i = 0; i < n; i++) {
                    result[i] = buffer[i];
                }
            }
            
            return result;
        }
    }
    
    pub fn recvString(maxBytes: int) -> string {
        @cpp {
            if (!this->connected || this->fd < 0) return "";
            
            std::vector<char> buffer(maxBytes);
            ssize_t n = ::recv(this->fd, buffer.data(), buffer.size(), 0);
            
            if (n > 0) {
                return std::string(buffer.data(), n);
            }
            
            return "";
        }
    }
    
    pub fn close() {
        @cpp {
            if (this->fd >= 0) {
                ::close(this->fd);
                this->fd = -1;
            }
            this->connected = false;
        }
    }
    
    pub fn setNonBlocking(enabled: bool) -> bool {
        @cpp {
            if (this->fd < 0) return false;
            
            int flags = fcntl(this->fd, F_GETFL, 0);
            if (flags < 0) return false;
            
            if (enabled) {
                flags |= O_NONBLOCK;
            } else {
                flags &= ~O_NONBLOCK;
            }
            
            return fcntl(this->fd, F_SETFL, flags) >= 0;
        }
    }
    
    pub fn setTimeout(seconds: int) -> bool {
        @cpp {
            if (this->fd < 0) return false;
            
            struct timeval tv;
            tv.tv_sec = seconds;
            tv.tv_usec = 0;
            
            return setsockopt(this->fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) >= 0 &&
                   setsockopt(this->fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) >= 0;
        }
    }
}

// ============================================================================
// TCP Server
// ============================================================================

pub class TcpServer {
    pub fd: int;
    pub port: int;
    pub listening: bool;
    
    pub fn create(port: int) {
        @cpp {
            this->fd = -1;
            this->port = port;
            this->listening = false;
        }
    }
    
    pub fn bind() -> bool {
        @cpp {
            // Create socket
            this->fd = socket(AF_INET, SOCK_STREAM, 0);
            if (this->fd < 0) return false;
            
            // Set socket options
            int opt = 1;
            setsockopt(this->fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
            
            // Bind
            struct sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = INADDR_ANY;
            addr.sin_port = htons(this->port);
            
            if (::bind(this->fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
                close(this->fd);
                this->fd = -1;
                return false;
            }
            
            return true;
        }
    }
    
    pub fn listen(backlog: int) -> bool {
        @cpp {
            if (this->fd < 0) return false;
            
            if (::listen(this->fd, backlog) < 0) {
                return false;
            }
            
            this->listening = true;
            return true;
        }
    }
    
    pub fn accept() -> Option<TcpSocket> {
        @cpp {
            if (!this->listening || this->fd < 0) return std::nullopt;
            
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            
            int client_fd = ::accept(this->fd, (struct sockaddr*)&client_addr, &client_len);
            if (client_fd < 0) return std::nullopt;
            
            TcpSocket client;
            client.fd = client_fd;
            client.connected = true;
            client.host = inet_ntoa(client_addr.sin_addr);
            client.port = ntohs(client_addr.sin_port);
            
            return client;
        }
    }
    
    pub fn close() {
        @cpp {
            if (this->fd >= 0) {
                ::close(this->fd);
                this->fd = -1;
            }
            this->listening = false;
        }
    }
}

// ============================================================================
// UDP Socket
// ============================================================================

pub class UdpAddress {
    pub host: string;
    pub port: int;
    
    pub fn create(host: string, port: int) {
        @cpp {
            this->host = host;
            this->port = port;
        }
    }
}

pub class UdpSocket {
    pub fd: int;
    pub bound: bool;
    pub port: int;
    
    pub fn create() {
        @cpp {
            this->fd = -1;
            this->bound = false;
            this->port = 0;
        }
    }
    
    pub fn bind(port: int) -> bool {
        @cpp {
            // Create socket
            this->fd = socket(AF_INET, SOCK_DGRAM, 0);
            if (this->fd < 0) return false;
            
            // Bind
            struct sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = INADDR_ANY;
            addr.sin_port = htons(port);
            
            if (::bind(this->fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
                close(this->fd);
                this->fd = -1;
                return false;
            }
            
            this->bound = true;
            this->port = port;
            return true;
        }
    }
    
    pub fn sendTo(data: Array<int>, addr: UdpAddress) -> int {
        @cpp {
            if (this->fd < 0) return -1;
            
            std::vector<unsigned char> bytes(data.size());
            for (size_t i = 0; i < data.size(); i++) {
                bytes[i] = static_cast<unsigned char>(data[i]);
            }
            
            struct sockaddr_in dest;
            dest.sin_family = AF_INET;
            dest.sin_port = htons(addr.port);
            inet_aton(addr.host.c_str(), &dest.sin_addr);
            
            ssize_t sent = sendto(this->fd, bytes.data(), bytes.size(), 0,
                                 (struct sockaddr*)&dest, sizeof(dest));
            return static_cast<int64_t>(sent);
        }
    }
    
    pub fn sendStringTo(str: string, addr: UdpAddress) -> int {
        @cpp {
            if (this->fd < 0) return -1;
            
            struct sockaddr_in dest;
            dest.sin_family = AF_INET;
            dest.sin_port = htons(addr.port);
            inet_aton(addr.host.c_str(), &dest.sin_addr);
            
            ssize_t sent = sendto(this->fd, str.c_str(), str.length(), 0,
                                 (struct sockaddr*)&dest, sizeof(dest));
            return static_cast<int64_t>(sent);
        }
    }
    
    pub fn recvFrom(maxBytes: int) -> SocketResult {
        @cpp {
            SocketResult result;
            if (this->fd < 0) {
                result.error = "Socket not initialized";
                return result;
            }
            
            std::vector<unsigned char> buffer(maxBytes);
            struct sockaddr_in src;
            socklen_t src_len = sizeof(src);
            
            ssize_t n = recvfrom(this->fd, buffer.data(), buffer.size(), 0,
                                (struct sockaddr*)&src, &src_len);
            
            if (n < 0) {
                result.error = strerror(errno);
                return result;
            }
            
            result.success = true;
            result.data.resize(n);
            for (ssize_t i = 0; i < n; i++) {
                result.data[i] = buffer[i];
            }
            
            return result;
        }
    }
    
    pub fn close() {
        @cpp {
            if (this->fd >= 0) {
                ::close(this->fd);
                this->fd = -1;
            }
            this->bound = false;
        }
    }
}

// ============================================================================
// DNS Resolution
// ============================================================================

pub fn resolve(hostname: string) -> Option<string> {
    @cpp {
        struct hostent* host = gethostbyname(hostname.c_str());
        if (!host || !host->h_addr_list[0]) {
            return std::nullopt;
        }
        
        char* ip = inet_ntoa(*(struct in_addr*)host->h_addr_list[0]);
        return std::string(ip);
    }
}

pub fn resolveAll(hostname: string) -> Array<string> {
    @cpp {
        std::vector<std::string> result;
        
        struct hostent* host = gethostbyname(hostname.c_str());
        if (!host) return result;
        
        for (int i = 0; host->h_addr_list[i] != nullptr; i++) {
            char* ip = inet_ntoa(*(struct in_addr*)host->h_addr_list[i]);
            result.push_back(std::string(ip));
        }
        
        return result;
    }
}

// ============================================================================
// Network Utilities
// ============================================================================

pub fn isValidIp(ip: string) -> bool {
    @cpp {
        struct sockaddr_in sa;
        return inet_pton(AF_INET, ip.c_str(), &(sa.sin_addr)) == 1;
    }
}

pub fn getHostname() -> Option<string> {
    @cpp {
        char hostname[256];
        if (gethostname(hostname, sizeof(hostname)) == 0) {
            return std::string(hostname);
        }
        return std::nullopt;
    }
}

pub fn getLocalIp() -> Option<string> {
    @cpp {
        // Get hostname
        char hostname[256];
        if (gethostname(hostname, sizeof(hostname)) != 0) {
            return std::nullopt;
        }
        
        // Resolve to IP
        struct hostent* host = gethostbyname(hostname);
        if (!host || !host->h_addr_list[0]) {
            return std::nullopt;
        }
        
        char* ip = inet_ntoa(*(struct in_addr*)host->h_addr_list[0]);
        return std::string(ip);
    }
}
