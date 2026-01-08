using Std.IO;
using Std.Network.Http;  // Must match exactly: Std.Network.Http

fn main() {
    println("Starting simple HTTP server...");
    
    let server = new HttpServer(8080);
    
    println("Server starting on http://localhost:8080");
    println("Press Ctrl+C to stop");
    
    server.start();
}
