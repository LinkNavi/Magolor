// Magolor Submodules Demo - All Network submodules
using Std.Network;

// ========== HTTP Submodule ==========
fn demoHTTP() {
    Std.print("=== HTTP Submodule Demo ===");
    
    // Using HTTP.Method enum
    let method = Network.HTTP.Method.POST;
    Std.print("Method: " + Network.HTTP.methodToString(method));
    
    // Using HTTP.Headers
    let headers = Network.HTTP.Headers();
    headers.set("Authorization", "Bearer token123");
    headers.set("X-Custom", "value");
    Std.print("Has Auth: " + Std.toString(headers.has("Authorization")));
    
    // Using ContentType constants
    Std.print("JSON Type: " + Network.HTTP.ContentType.JSON);
}

// ========== Security Submodule ==========
fn demoSecurity() {
    Std.print("\n=== Security Submodule Demo ===");
    
    // XSS protection
    let userInput = "<script>alert('xss')</script>";
    let safe = Network.Security.escapeHtml(userInput);
    Std.print("Escaped: " + safe);
    
    // Generate tokens
    let token = Network.Security.generateToken(32);
    Std.print("Token: " + token);
    
    let csrf = Network.Security.generateCsrfToken();
    Std.print("CSRF: " + csrf);
    
    // Rate limiting
    let limiter = Network.Security.RateLimiter(5, 60); // 5 req/min
    Std.print("Allow request: " + Std.toString(limiter.allow("user1")));
}

// ========== JSON Submodule ==========
fn demoJSON() {
    Std.print("\n=== JSON Submodule Demo ===");
    
    // JSON escape
    let str = "Hello \"World\"\nNew line";
    let escaped = Network.JSON.Parser.escape(str);
    Std.print("Escaped: " + escaped);
    
    // Array builder
    let arr = Network.JSON.ArrayBuilder();
    arr.add("first");
    arr.add(42);
    arr.add(true);
    Std.print("Array: " + arr.build());
}

// ========== Routing Submodule ==========
fn demoRouting() {
    Std.print("\n=== Routing Submodule Demo ===");
    
    // Pattern matching
    let pattern = "/users/:id/posts/:postId";
    let path = "/users/123/posts/456";
    let lal = Network.Routing.matchRoute(pattern, path);
    
    if (lal.matches) {
        Std.print("Matched!");
        Std.print("User ID: " + lal.params["id"]);
        Std.print("Post ID: " + lal.params["postId"]);
    }
}

// ========== TCP Submodule ==========
fn demoTCP() {
    Std.print("\n=== TCP Server Demo ===");
    
    let server = Network.TCP.Server(9000);
    
    if (server.start()) {
        Std.print("TCP Server on :9000");
        Std.print("Waiting for connection...");
        
        let client = server.accept();
        if (client != Network.INVALID_SOCKET) {
            Std.print("Client connected!");
            Network.send(client, "Hello from TCP server\n");
            Network.CLOSE_SOCKET(client);
        }
        
        server.stop();
    }
}

// ========== UDP Submodule ==========
fn demoUDP() {
    Std.print("\n=== UDP Demo ===");
    
    let socket = Network.UDP.Socket();
    
    // Send UDP packet
    if (socket.sendTo("Hello UDP", "127.0.0.1", 8000)) {
        Std.print("UDP packet sent!");
    }
}

// ========== WebSocket Submodule ==========
fn demoWebSocket(clientSocket: int) {
    Std.print("\n=== WebSocket Demo ===");
    
    let ws = Network.WebSocket.Connection(clientSocket);
    
    // Send text frame
    ws.send("Hello WebSocket!");
    
    // Send binary
    let data = [1, 2, 3];
    ws.sendBinary(data);
    
    // Ping
    ws.ping();
    
    ws.close();
}

// ========== Full Server with Submodules ==========
fn main() {
    // Demo each submodule
    demoHTTP();
    demoSecurity();
    demoJSON();
    demoRouting();
    
    // Full server example using submodules
    let server = Network.Server(8080);
    let router = Network.Routing.Router();
    let limiter = Network.Security.RateLimiter(10, 60);
    
    // Add routes with parameter extraction
    router.add("GET", "/users/:id", fn(req, params) {
        if (!limiter.allow(req.remoteIp)) {
            return Network.errorResponse(429, "Too many requests");
        }
        
        let userId = params["id"];
        let safe = Network.Security.escapeHtml(userId);
        
        let arr = Network.JSON.ArrayBuilder();
        arr.add(safe);
        arr.add("user_" + safe);
        
        return Network.jsonResponse("{\"id\":" + safe + ",\"items\":" + arr.build() + "}");
    });
    
    router.add("POST", "/api/:resource", fn(req, params) {
        let resource = params["resource"];
        let token = Network.Security.generateToken(16);
        
        return Network.jsonResponse("{\"resource\":\"" + resource + "\",\"token\":\"" + token + "\"}");
    });
    
    server.use(Network.corsMiddleware());
    server.use(Network.loggerMiddleware());
    
    server.route("*", fn(req) {
        return router.route(req);
    });
    
    Std.print("\n=== Server Started ===");
    Std.print("Submodules demo on http://localhost:8080");
    Std.print("Try: /users/123 or /api/posts");
    
    server.start();
}
