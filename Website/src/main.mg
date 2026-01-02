using Std.IO;
using Std.Network;


fn main() {
    Std.print("Starting simple HTTP server...\n");

    let server = Std.Network.HttpServer(8080);

    // Handle only the root path
    server.get("/", fn(req) {
        return Std.Network.serveFile("public/index.html");
    });

    // 404 for everything else
    server.setNotFound(fn(req) {
        return Std.Network.htmlResponse("<h1>404 - Page Not Found</h1>", Std.Network.Status.NOT_FOUND);
    });

    Std.print("Server is ready! Visit http://localhost:8080/\n");
    let p = createPackage("SlateDB");

    Std.print(p);
    server.start();
}
