// Example 1: Simple HTTP Server (Fixed for Magolor type system)
// Compile: magolor run simple_server.mg
// Test: curl http://localhost:8080

using Std.IO;
using Std.Network;

fn main() {
    Std.print("Starting simple HTTP server...\n");

    // Create server on port 8080
    let server = Std.Network.HttpServer(8080);


    server.get("/", fn(req) {
        return Std.Network.htmlResponse("<h1>Hello from Magolor!</h1><p>Welcome to the web server.</p>");
    });

// Define an API endpoint
server.get("/api/hello", fn(req) {
    return Std.Network.jsonResponse("{\"message\": \"Hello, World!\", \"status\": \"ok\"}");
});

// Define a route with query parameters
server.get("/greet", fn(req) {
    let name = req.getQuery("name");
    if (name == "") {
        return Std.Network.htmlResponse("<h1>Hello, Stranger!</h1>");
    } else {
    return Std.Network.htmlResponse($"<h1>Hello, {name}!</h1>");
}
});

// Define info route
server.get("/info", fn(req) {
    let html = "<!DOCTYPE html>
    <html>
    <head>
    <title>Server Info</title>
    <style>
    body { font-family: Arial, sans-serif; margin: 40px; }
    h1 { color: #333; }
    .endpoint { background: #f4f4f4; padding: 10px; margin: 10px 0; }
    </style>
    </head>
    <body>
    <h1>Magolor HTTP Server</h1>
    <h2>Available Endpoints:</h2>
    <div class='endpoint'>GET / - Home page</div>
    <div class='endpoint'>GET /api/hello - JSON API</div>
    <div class='endpoint'>GET /greet?name=YourName - Personalized greeting</div>
    <div class='endpoint'>GET /info - This page</div>
    </body>
    </html>";
    return Std.Network.htmlResponse(html);
});

// Custom 404 handler
server.setNotFound(fn(req) {
    return Std.Network.htmlResponse($"<h1>404 - Page Not Found</h1><p>Path '{req.path}' does not exist.</p>", Std.Network.Status.NOT_FOUND);
});

// Start the server (blocking call)
Std.print("Server is ready!\n");
Std.print("Try these URLs:\n");
Std.print("  http://localhost:8080/\n");
Std.print("  http://localhost:8080/api/hello\n");
Std.print("  http://localhost:8080/greet?name=Alice\n");
Std.print("  http://localhost:8080/info\n");
Std.print("\n");

server.start();
}
