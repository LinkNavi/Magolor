// ============================================================================
// Comprehensive Web Development Example for Magolor
// Showcases the enhanced cross-platform network runtime
// ============================================================================

using Std.IO;
using Std.Network;

fn main() {
    Std.print("╔════════════════════════════════════════╗\n");
    Std.print("║  Magolor Web Framework Demo           ║\n");
    Std.print("║  Cross-Platform HTTP Server            ║\n");
    Std.print("╚════════════════════════════════════════╝\n\n");

    // Create server
    let server = Std.Network.HttpServer(3000);
    
    // ========================================================================
    // Add Middleware (executed in order for all requests)
    // ========================================================================
    
    // 1. CORS middleware - Allow cross-origin requests
    server.use(Std.Network.corsMiddleware("*"));
    
    // 2. Logger middleware - Log all requests
    server.use(Std.Network.loggerMiddleware());
    
    // 3. Custom authentication middleware example
    server.use(fn(req, res) {
        // Skip auth for public routes
        if (req.path == "/" || req.path == "/login") {
            return true;
        }
        
        // Check for API key
        let apiKey = req.getHeader("X-API-Key");
        if (apiKey == "secret123") {
            return true; // Continue processing
        }
        
        // Unauthorized
        res.statusCode = Std.Network.Status.UNAUTHORIZED;
        res.body = "{\"error\": \"Unauthorized\"}";
        res.setJson();
        return false; // Stop processing
    });
    
    // ========================================================================
    // Basic Routes
    // ========================================================================
    
    server.get("/", fn(req) {
        let html = "<!DOCTYPE html>
<html>
<head>
    <title>Magolor Web Demo</title>
    <style>
        body { 
            font-family: Arial, sans-serif; 
            max-width: 800px; 
            margin: 40px auto; 
            padding: 20px;
            background: #f5f5f5;
        }
        .card {
            background: white;
            padding: 20px;
            margin: 20px 0;
            border-radius: 8px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        h1 { color: #333; }
        code { 
            background: #f0f0f0; 
            padding: 2px 6px; 
            border-radius: 3px;
        }
        .endpoint {
            margin: 10px 0;
            padding: 10px;
            background: #f8f8f8;
            border-left: 4px solid #4CAF50;
        }
    </style>
</head>
<body>
    <h1>🚀 Magolor Web Framework</h1>
    <div class='card'>
        <h2>Available Endpoints</h2>
        
        <div class='endpoint'>
            <strong>GET /</strong> - This page
        </div>
        
        <div class='endpoint'>
            <strong>GET /api/hello</strong> - JSON API example
        </div>
        
        <div class='endpoint'>
            <strong>GET /greet?name=YourName</strong> - Query parameters
        </div>
        
        <div class='endpoint'>
            <strong>POST /api/data</strong> - Form data handling
        </div>
        
        <div class='endpoint'>
            <strong>GET /cookie-demo</strong> - Cookie management
        </div>
        
        <div class='endpoint'>
            <strong>GET /session-demo</strong> - Session management
        </div>
        
        <div class='endpoint'>
            <strong>GET /json-builder</strong> - JSON builder demo
        </div>
        
        <div class='endpoint'>
            <strong>GET /redirect</strong> - Redirect example
        </div>
        
        <div class='endpoint'>
            <strong>GET /files/[path]</strong> - Static file serving
        </div>
    </div>
    
    <div class='card'>
        <h2>Features</h2>
        <ul>
            <li>✅ Cross-platform (Windows, Linux, macOS)</li>
            <li>✅ HTTP/1.1 with all methods</li>
            <li>✅ Cookie management</li>
            <li>✅ Session management</li>
            <li>✅ Middleware system</li>
            <li>✅ CORS support</li>
            <li>✅ JSON builder</li>
            <li>✅ Form data parsing</li>
            <li>✅ Static file serving</li>
            <li>✅ Query string parsing</li>
            <li>✅ URL encoding/decoding</li>
        </ul>
    </div>
</body>
</html>";
        
        return Std.Network.htmlResponse(html);
    });
    
    // ========================================================================
    // JSON API Routes
    // ========================================================================
    
    server.get("/api/hello", fn(req) {
        // Using JsonBuilder for clean JSON construction
        let json = Std.Network.JsonBuilder();
        json.add("message", "Hello from Magolor!");
        json.add("version", "1.0");
        json.add("timestamp", 1234567890);
        json.add("success", true);
        
        return Std.Network.jsonResponse(json.build());
    });
    
    // ========================================================================
    // Query Parameters
    // ========================================================================
    
    server.get("/greet", fn(req) {
        let name = req.getQuery("name");
        
        if (name == "") {
            return Std.Network.htmlResponse("<h1>Hello, Stranger!</h1>");
        }
        
        return Std.Network.htmlResponse($"<h1>Hello, {name}!</h1>");
    });
    
    // ========================================================================
    // POST with Form Data
    // ========================================================================
    
    server.post("/api/data", fn(req) {
        // Check if it's form data
        if (req.isForm()) {
            let username = req.getForm("username");
            let email = req.getForm("email");
            
            let json = Std.Network.JsonBuilder();
            json.add("received", "form data");
            json.add("username", username);
            json.add("email", email);
            
            return Std.Network.jsonResponse(json.build(), Std.Network.Status.CREATED);
        }
        
        // Check if it's JSON
        if (req.isJson()) {
            // In real app, you'd parse the JSON body
            let json = Std.Network.JsonBuilder();
            json.add("received", "json data");
            json.add("bodyLength", 100);
            
            return Std.Network.jsonResponse(json.build());
        }
        
        return Std.Network.errorResponse(
            Std.Network.Status.BAD_REQUEST,
            "Expected form data or JSON"
        );
    });
    
    // ========================================================================
    // Cookie Management
    // ========================================================================
    
    server.get("/cookie-demo", fn(req) {
        let res = Std.Network.HttpResponse();
        
        // Read existing cookie
        let visits = req.getCookie("visits");
        
        // Create a new cookie
        let cookie = Std.Network.Cookie();
        cookie.name = "visits";
        cookie.value = "1";
        cookie.maxAge = 3600; // 1 hour
        cookie.httpOnly = true;
        cookie.secure = false; // Set true in production with HTTPS
        cookie.sameSite = "Lax";
        
        res.setCookie(cookie);
        
        if (visits == "") {
            res.body = "<h1>Welcome! This is your first visit.</h1>";
        } else {
            res.body = $"<h1>Welcome back! Visit count: {visits}</h1>";
        }
        
        return res;
    });
    
    // ========================================================================
    // Session Management
    // ========================================================================
    
    server.get("/session-demo", fn(req) {
        let sessionStore = server.getSessionStore();
        let res = Std.Network.HttpResponse();
        
        // Get session ID from cookie
        let sessionId = req.getCookie("session_id");
        
        if (sessionId == "" || !sessionStore.exists(sessionId)) {
            // Create new session
            sessionId = sessionStore.create();
            sessionStore.set(sessionId, "user", "guest");
            sessionStore.set(sessionId, "login_time", "2024-01-01");
            
            let cookie = Std.Network.Cookie();
            cookie.name = "session_id";
            cookie.value = sessionId;
            cookie.httpOnly = true;
            res.setCookie(cookie);
            
            res.body = "<h1>New session created!</h1>";
        } else {
            // Read from session
            let user = sessionStore.get(sessionId, "user");
            let loginTime = sessionStore.get(sessionId, "login_time");
            
            res.body = $"<h1>Welcome back, {user}!</h1><p>Login time: {loginTime}</p>";
        }
        
        return res;
    });
    
    // ========================================================================
    // JSON Builder Demo
    // ========================================================================
    
    server.get("/json-builder", fn(req) {
        let json = Std.Network.JsonBuilder();
        json.add("status", "success");
        json.add("code", 200);
        json.add("pi", 3.14159);
        json.add("active", true);
        json.add("message", "JsonBuilder makes JSON easy!");
        
        return Std.Network.jsonResponse(json.build());
    });
    
    // ========================================================================
    // Redirect Example
    // ========================================================================
    
    server.get("/redirect", fn(req) {
        return Std.Network.redirectResponse("/", Std.Network.Status.TEMPORARY_REDIRECT);
    });
    
    // ========================================================================
    // Static File Serving
    // ========================================================================
    
    server.get("/files/test.html", fn(req) {
        return Std.Network.serveFile("test.html");
    });
    
    // ========================================================================
    // Content Negotiation
    // ========================================================================
    
    server.get("/data", fn(req) {
        if (req.acceptsJson()) {
            let json = Std.Network.JsonBuilder();
            json.add("format", "json");
            json.add("value", 42);
            return Std.Network.jsonResponse(json.build());
        }
        
        if (req.acceptsHtml()) {
            return Std.Network.htmlResponse("<h1>Data: 42</h1>");
        }
        
        // Default to text
        return Std.Network.textResponse("Data: 42");
    });
    
    // ========================================================================
    // Error Handling
    // ========================================================================
    
    server.get("/error", fn(req) {
        return Std.Network.errorResponse(
            Std.Network.Status.INTERNAL_SERVER_ERROR,
            "This is a demo error"
        );
    });
    
    // ========================================================================
    // Custom 404 Handler
    // ========================================================================
    
    server.setNotFound(fn(req) {
        let html = "<!DOCTYPE html>
<html>
<head>
    <title>404 Not Found</title>
    <style>
        body { 
            font-family: Arial, sans-serif; 
            text-align: center; 
            padding: 50px;
            background: #f5f5f5;
        }
        h1 { color: #e74c3c; }
        a { color: #3498db; }
    </style>
</head>
<body>
    <h1>😕 404 - Page Not Found</h1>
    <p>The page you're looking for doesn't exist.</p>
    <p>Path: " + req.path + "</p>
    <p><a href='/'>← Go Home</a></p>
</body>
</html>";
        
        return Std.Network.htmlResponse(html, Std.Network.Status.NOT_FOUND);
    });
    
    // ========================================================================
    // Start Server
    // ========================================================================
    
    Std.print("\n🌐 Starting server...\n\n");
    Std.print("Try these URLs:\n");
    Std.print("  http://localhost:3000/\n");
    Std.print("  http://localhost:3000/api/hello\n");
    Std.print("  http://localhost:3000/greet?name=Alice\n");
    Std.print("  http://localhost:3000/cookie-demo\n");
    Std.print("  http://localhost:3000/session-demo\n");
    Std.print("\nPress Ctrl+C to stop.\n\n");
    
    server.start();
}
