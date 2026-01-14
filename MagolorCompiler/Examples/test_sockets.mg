// test_sockets.mg - Socket Programming Example
// Uses only the actually implemented low-level socket functions

using Std.IO;
using Std.Time;

// ============================================================================
// Example 1: Simple Echo Server
// ============================================================================

fn runEchoServer() {
    Std.IO.println("=== Echo Server Example ===");
    Std.IO.println("Starting server on port 8080...");
    
    // Create server socket
    let serverSocket = Std.Network.socketsListen(8080);
    
    if (serverSocket < 0) {
        Std.IO.println("Error: Failed to create server socket");
        return;
    }
    
    Std.IO.println("Server listening on port 8080");
    Std.IO.println("Waiting for client connection...");
    
    // Accept one client connection
    let clientSocket = Std.Network.socketsAccept(serverSocket);
    
    if (clientSocket < 0) {
        Std.IO.println("Error: Failed to accept client");
        Std.Network.socketsClose(serverSocket);
        return;
    }
    
    Std.IO.println("Client connected!");
    
    // Receive message from client
    let message = Std.Network.socketsReceive(clientSocket, 1024);
    Std.IO.print("Received: ");
    Std.IO.println(message);
    
    // Echo it back
    let response = "Echo: " + message;
    Std.Network.socketsSend(clientSocket, response);
    Std.IO.println("Sent echo response");
    
    // Close connections
    Std.Network.socketsClose(clientSocket);
    Std.Network.socketsClose(serverSocket);
    Std.IO.println("Server closed");
    Std.IO.println("");
}

// ============================================================================
// Example 2: Simple Client
// ============================================================================

fn runSimpleClient() {
    Std.IO.println("=== Simple Client Example ===");
    Std.IO.println("Connecting to localhost:8080...");
    
    // Connect to server
    let socket = Std.Network.socketsConnect("127.0.0.1", 8080);
    
    if (socket < 0) {
        Std.IO.println("Error: Failed to connect to server");
        Std.IO.println("Make sure the server is running first!");
        return;
    }
    
    Std.IO.println("Connected to server!");
    
    // Send message
    let message = "Hello from Magolor client!";
    Std.IO.print("Sending: ");
    Std.IO.println(message);
    
    let bytesSent = Std.Network.socketsSend(socket, message);
    Std.IO.print("Sent ");
    Std.IO.print(bytesSent);
    Std.IO.println(" bytes");
    
    // Receive response
    Std.IO.println("Waiting for response...");
    let response = Std.Network.socketsReceive(socket, 1024);
    Std.IO.print("Received: ");
    Std.IO.println(response);
    
    // Close connection
    Std.Network.socketsClose(socket);
    Std.IO.println("Connection closed");
    Std.IO.println("");
}

// ============================================================================
// Example 3: Chat Server (handles one message exchange)
// ============================================================================

fn runChatServer() {
    Std.IO.println("=== Chat Server Example ===");
    Std.IO.println("Starting chat server on port 9000...");
    
    let serverSocket = Std.Network.socketsListen(9000);
    
    if (serverSocket < 0) {
        Std.IO.println("Error: Could not start server");
        return;
    }
    
    Std.IO.println("Chat server ready! Waiting for client...");
    
    let clientSocket = Std.Network.socketsAccept(serverSocket);
    
    if (clientSocket < 0) {
        Std.IO.println("Error: Could not accept client");
        Std.Network.socketsClose(serverSocket);
        return;
    }
    
    Std.IO.println("Client joined the chat!");
    
    // Chat loop (5 messages)
    let mut messageCount = 0;
    while (messageCount < 5) {
        // Receive from client
        Std.IO.println("[Waiting for client message...]");
        let clientMsg = Std.Network.socketsReceive(clientSocket, 1024);
        
        if (Std.String.length(clientMsg) > 0) {
            Std.IO.print("Client: ");
            Std.IO.println(clientMsg);
            
            // Send response
            Std.IO.print("Server> ");
            let serverMsg = Std.IO.readLine();
            Std.Network.socketsSend(clientSocket, serverMsg);
            
            messageCount = messageCount + 1;
        }
    }
    
    Std.IO.println("Chat session ended");
    Std.Network.socketsClose(clientSocket);
    Std.Network.socketsClose(serverSocket);
    Std.IO.println("");
}

// ============================================================================
// Example 4: Port Scanner
// ============================================================================

fn scanPort(host: String, port: Int) -> Bool {
    let socket = Std.Network.socketsConnect(host, port);
    
    if (socket >= 0) {
        Std.Network.socketsClose(socket);
        return true;
    }
    
    return false;
}

fn runPortScanner() {
    Std.IO.println("=== Port Scanner Example ===");
    Std.IO.print("Enter hostname (e.g., 127.0.0.1): ");
    let host = Std.IO.readLine();
    
    Std.IO.println("Scanning common ports...");
    
    let commonPorts = [20, 21, 22, 23, 25, 80, 443, 3306, 5432, 8080];
    let mut i = 0;
    
    while (i < Std.Array.length(commonPorts)) {
        let port = commonPorts[i];
        
        Std.IO.print("Port ");
        Std.IO.print(port);
        Std.IO.print(": ");
        
        if (scanPort(host, port)) {
            Std.IO.println("OPEN");
        } else {
            Std.IO.println("CLOSED");
        }
        
        i = i + 1;
    }
    
    Std.IO.println("Scan complete");
    Std.IO.println("");
}

// ============================================================================
// Example 5: Simple HTTP-like Request
// ============================================================================

fn makeHttpLikeRequest() {
    Std.IO.println("=== HTTP-like Request Example ===");
    Std.IO.println("Connecting to example.com:80...");
    
    let socket = Std.Network.socketsConnect("93.184.216.34", 80);
    
    if (socket < 0) {
        Std.IO.println("Error: Could not connect");
        return;
    }
    
    Std.IO.println("Connected! Sending HTTP request...");
    
    // Build HTTP GET request
    let request = "GET / HTTP/1.0\r\nHost: example.com\r\n\r\n";
    Std.Network.socketsSend(socket, request);
    
    Std.IO.println("Request sent, receiving response...");
    
    // Receive response
    let response = Std.Network.socketsReceive(socket, 4096);
    
    Std.IO.println("Response received:");
    Std.IO.println("----------------------------------------");
    Std.IO.println(response);
    Std.IO.println("----------------------------------------");
    
    Std.Network.socketsClose(socket);
    Std.IO.println("");
}

// ============================================================================
// Example 6: Simple Data Transfer
// ============================================================================

fn runDataTransferServer() {
    Std.IO.println("=== Data Transfer Server ===");
    Std.IO.println("Starting server on port 7000...");
    
    let serverSocket = Std.Network.socketsListen(7000);
    
    if (serverSocket < 0) {
        Std.IO.println("Error: Failed to start server");
        return;
    }
    
    Std.IO.println("Server ready");
    let clientSocket = Std.Network.socketsAccept(serverSocket);
    
    if (clientSocket >= 0) {
        Std.IO.println("Client connected");
        
        // Send a series of messages
        let messages = ["Message 1", "Message 2", "Message 3", "END"];
        let mut i = 0;
        
        while (i < Std.Array.length(messages)) {
            let msg = messages[i];
            Std.IO.print("Sending: ");
            Std.IO.println(msg);
            Std.Network.socketsSend(clientSocket, msg);
            Std.Time.sleep(1000);
            i = i + 1;
        }
        
        Std.Network.socketsClose(clientSocket);
    }
    
    Std.Network.socketsClose(serverSocket);
    Std.IO.println("Transfer complete");
    Std.IO.println("");
}

// ============================================================================
// Example 7: Connection Test
// ============================================================================

fn testConnection(host: String, port: Int) {
    Std.IO.print("Testing connection to ");
    Std.IO.print(host);
    Std.IO.print(":");
    Std.IO.print(port);
    Std.IO.println("...");
    
    let socket = Std.Network.socketsConnect(host, port);
    
    if (socket >= 0) {
        Std.IO.println("✓ Connection successful!");
        Std.Network.socketsClose(socket);
    } else {
        Std.IO.println("✗ Connection failed");
    }
}

fn runConnectionTests() {
    Std.IO.println("=== Connection Tests ===");
    
    // Test localhost
    testConnection("127.0.0.1", 80);
    
    // Test common services
    testConnection("8.8.8.8", 53);
    
    Std.IO.println("");
}

// ============================================================================
// Main Menu
// ============================================================================

fn showMenu() {
    Std.IO.println("╔════════════════════════════════════════════════════════╗");
    Std.IO.println("║         Magolor Socket Programming Examples           ║");
    Std.IO.println("╚════════════════════════════════════════════════════════╝");
    Std.IO.println("");
    Std.IO.println("Choose an example:");
    Std.IO.println("  1. Echo Server (run server first)");
    Std.IO.println("  2. Echo Client (run after starting server)");
    Std.IO.println("  3. Chat Server");
    Std.IO.println("  4. Port Scanner");
    Std.IO.println("  5. HTTP-like Request");
    Std.IO.println("  6. Data Transfer Server");
    Std.IO.println("  7. Connection Tests");
    Std.IO.println("  0. Exit");
    Std.IO.println("");
}

fn main() {
    let mut running = true;
    
    while (running) {
        showMenu();
        Std.IO.print("Enter choice (0-7): ");
        let choice = Std.String.toInt(Std.IO.readLine());
        Std.IO.println("");
        
        if (choice == 1) {
            runEchoServer();
        } else if (choice == 2) {
            runSimpleClient();
        } else if (choice == 3) {
            runChatServer();
        } else if (choice == 4) {
            runPortScanner();
        } else if (choice == 5) {
            makeHttpLikeRequest();
        } else if (choice == 6) {
            runDataTransferServer();
        } else if (choice == 7) {
            runConnectionTests();
        } else if (choice == 0) {
            Std.IO.println("Goodbye!");
            running = false;
        } else {
            Std.IO.println("Invalid choice. Please try again.");
            Std.IO.println("");
        }
    }
}
