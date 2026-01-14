// test_network.mg - Comprehensive Network Module Examples
// Demonstrates various networking capabilities in Magolor

using Std.IO;
using Std.Network;
using Std.String;
using Std.Array;
using Std.Time;

// ============================================================================
// Example 1: Simple HTTP GET Request
// ============================================================================

fn testSimpleGet() {
    Std.IO.println("=== Example 1: Simple HTTP GET ===");
    
    let response = Std.Network.httpGet("https://api.github.com/zen");
    Std.IO.println("GitHub Zen Quote:");
    Std.IO.println(response);
    Std.IO.println("");
}

// ============================================================================
// Example 2: HTTP GET with JSON Response
// ============================================================================

fn testGetJson() {
    Std.IO.println("=== Example 2: HTTP GET with JSON ===");
    
    let response = Std.Network.httpGetJson("https://api.github.com/users/github");
    Std.IO.println("GitHub User Data:");
    Std.IO.println(response);
    Std.IO.println("");
}

// ============================================================================
// Example 3: HTTP POST with JSON Data
// ============================================================================

fn testPostJson() {
    Std.IO.println("=== Example 3: HTTP POST with JSON ===");
    
    let jsonData = "{\"title\":\"Test Post\",\"body\":\"This is a test\",\"userId\":1}";
    let response = Std.Network.httpPostJson("https://jsonplaceholder.typicode.com/posts", jsonData);
    
    Std.IO.println("Created Post Response:");
    Std.IO.println(response);
    Std.IO.println("");
}

// ============================================================================
// Example 4: Simple Download Test
// ============================================================================

fn testSimpleDownload() {
    Std.IO.println("=== Example 4: Simple Download ===");
    
    let content = Std.Network.downloadString("https://httpbin.org/get");
    Std.IO.println("Downloaded content (first 200 chars):");
    Std.IO.println(Std.String.substring(content, 0, 200));
    Std.IO.println("...");
    Std.IO.println("");
}

// ============================================================================
// Example 5: URL Utilities
// ============================================================================

fn testUrlUtilities() {
    Std.IO.println("=== Example 5: URL Utilities ===");
    
    let url = "https://example.com:8080/path/to/page?key=value&foo=bar#section";
    
    Std.IO.print("Original URL: ");
    Std.IO.println(url);
    
    let domain = Std.Network.getDomain(url);
    Std.IO.print("Domain: ");
    Std.IO.println(domain);
    
    let path = Std.Network.getPath(url);
    Std.IO.print("Path: ");
    Std.IO.println(path);
    Std.IO.println("");
    
    // URL encoding
    let text = "Hello World! This needs encoding: @#$%";
    let encoded = Std.Network.urlEncode(text);
    Std.IO.print("Original: ");
    Std.IO.println(text);
    Std.IO.print("Encoded: ");
    Std.IO.println(encoded);
    Std.IO.print("Decoded: ");
    Std.IO.println(Std.Network.urlDecode(encoded));
    Std.IO.println("");
}

// ============================================================================
// Example 6: DNS Resolution
// ============================================================================

fn testDnsResolution() {
    Std.IO.println("=== Example 6: DNS Resolution ===");
    
    let hostname = "github.com";
    let ip = Std.Network.resolveHost(hostname);
    
    Std.IO.print("Hostname: ");
    Std.IO.println(hostname);
    Std.IO.print("IP Address: ");
    Std.IO.println(ip);
    Std.IO.println("");
}

// ============================================================================
// Example 7: Network Utilities
// ============================================================================

fn testNetworkUtilities() {
    Std.IO.println("=== Example 7: Network Utilities ===");
    
    // Get local info
    Std.IO.print("Local Hostname: ");
    Std.IO.println(Std.Network.getHostname());
    Std.IO.print("Local IP: ");
    Std.IO.println(Std.Network.getLocalIP());
    Std.IO.println("");
    
    // Ping test
    let host = "8.8.8.8";
    Std.IO.print("Pinging ");
    Std.IO.print(host);
    Std.IO.println("...");
    
    if (Std.Network.ping(host, 5000)) {
        let time = Std.Network.pingTime(host, 5000);
        Std.IO.print("Host is reachable! Response time: ");
        Std.IO.print(time);
        Std.IO.println("ms");
    } else {
        Std.IO.println("Host is unreachable");
    }
    Std.IO.println("");
}

// ============================================================================
// Example 8: HTTP Status Code Helpers
// ============================================================================

fn testStatusCodes() {
    Std.IO.println("=== Example 8: HTTP Status Code Helpers ===");
    
    let codes = [200, 201, 301, 400, 404, 500, 503];
    
    Std.IO.println("Status Code Analysis:");
    let mut k = 0;
    while (k < Std.Array.length(codes)) {
        let code = codes[k];
        let text = Std.Network.getStatusText(code);
        
        Std.IO.print(Std.String.toString(code));
        Std.IO.print(" ");
        Std.IO.print(text);
        Std.IO.print(" - ");
        
        if (Std.Network.isSuccess(code)) {
            Std.IO.println("Success");
        } else if (Std.Network.isRedirect(code)) {
            Std.IO.println("Redirect");
        } else if (Std.Network.isClientError(code)) {
            Std.IO.println("Client Error");
        } else if (Std.Network.isServerError(code)) {
            Std.IO.println("Server Error");
        }
        
        k = k + 1;
    }
    Std.IO.println("");
}

// ============================================================================
// Example 9: Content Type Detection
// ============================================================================

fn testContentTypeDetection() {
    Std.IO.println("=== Example 9: Content Type Detection ===");
    
    let files = [
        "index.html",
        "style.css",
        "script.js",
        "data.json",
        "image.png",
        "document.pdf",
        "archive.zip",
        "unknown.xyz"
    ];
    
    Std.IO.println("Content Type Detection:");
    let mut m = 0;
    while (m < Std.Array.length(files)) {
        let file = files[m];
        let contentType = Std.Network.detectContentType(file);
        Std.IO.print("  ");
        Std.IO.print(file);
        Std.IO.print(" -> ");
        Std.IO.println(contentType);
        m = m + 1;
    }
    Std.IO.println("");
}

// ============================================================================
// Example 10: Web Scraper
// ============================================================================

fn testWebScraper() {
    Std.IO.println("=== Example 10: Simple Web Scraper ===");
    
    Std.IO.println("Fetching webpage...");
    let html = Std.Network.httpGet("https://example.com");
    
    Std.IO.print("Page size: ");
    Std.IO.print(Std.String.toString(Std.String.length(html)));
    Std.IO.println(" bytes");
    
    // Extract title (simple string search)
    let titleStart = Std.String.indexOf(html, "<title>");
    let titleEnd = Std.String.indexOf(html, "</title>");
    
    if (titleStart != -1) {
        if (titleEnd != -1) {
            let title = Std.String.substring(html, titleStart + 7, titleEnd);
            Std.IO.print("Page title: ");
            Std.IO.println(title);
        }
    }
    Std.IO.println("");
}

// ============================================================================
// Example 11: Rate-Limited API Client
// ============================================================================

fn testRateLimitedRequests() {
    Std.IO.println("=== Example 11: Rate-Limited Requests ===");
    
    Std.IO.println("Making 5 requests with delay...");
    
    let mut n = 1;
    while (n <= 5) {
        Std.IO.print("Request ");
        Std.IO.print(Std.String.toString(n));
        Std.IO.println("...");
        
        let response = Std.Network.httpGet("https://httpbin.org/uuid");
        
        if (Std.String.length(response) > 0) {
            Std.IO.print("  Response received: ");
            Std.IO.print(Std.String.substring(response, 0, 50));
            Std.IO.println("...");
        }
        
        // Wait 1 second between requests
        if (n < 5) {
            Std.IO.println("  Waiting 1 second...");
            Std.Time.sleepSeconds(1);
        }
        
        n = n + 1;
    }
    Std.IO.println("All requests completed!");
    Std.IO.println("");
}

// ============================================================================
// Example 12: Multiple HTTP Methods
// ============================================================================

fn testHttpMethods() {
    Std.IO.println("=== Example 12: HTTP Methods ===");
    
    // GET
    Std.IO.println("Testing GET...");
    let getResp = Std.Network.httpGet("https://httpbin.org/get");
    if (Std.String.length(getResp) > 0) {
        Std.IO.println("  ✓ GET successful");
    }
    
    // POST
    Std.IO.println("Testing POST...");
    let postResp = Std.Network.httpPost("https://httpbin.org/post", "test data");
    if (Std.String.length(postResp) > 0) {
        Std.IO.println("  ✓ POST successful");
    }
    
    // PUT
    Std.IO.println("Testing PUT...");
    let putResp = Std.Network.httpPut("https://httpbin.org/put", "update data");
    if (Std.String.length(putResp) > 0) {
        Std.IO.println("  ✓ PUT successful");
    }
    
    // DELETE
    Std.IO.println("Testing DELETE...");
    let delResp = Std.Network.httpDelete("https://httpbin.org/delete");
    if (Std.String.length(delResp) > 0) {
        Std.IO.println("  ✓ DELETE successful");
    }
    
    Std.IO.println("");
}

// ============================================================================
// Example 13: Error Handling
// ============================================================================

fn testErrorHandling() {
    Std.IO.println("=== Example 13: Error Handling ===");
    
    // Try to fetch from invalid URL
    Std.IO.println("Attempting to fetch from invalid URL...");
    let response = Std.Network.httpGet("https://this-domain-definitely-does-not-exist-12345.com");
    
    if (Std.String.length(response) == 0) {
        Std.IO.println("Request failed (as expected)");
    } else {
        Std.IO.println("Unexpected success");
    }
    Std.IO.println("");
}

// ============================================================================
// Example 14: File Download
// ============================================================================

fn testFileDownload() {
    Std.IO.println("=== Example 14: File Download ===");
    
    // Download a small text file
    let content = Std.Network.downloadString("https://raw.githubusercontent.com/github/gitignore/main/LICENSE");
    
    Std.IO.println("Downloaded content (first 200 chars):");
    Std.IO.println(Std.String.substring(content, 0, 200));
    Std.IO.println("...");
    Std.IO.println("");
}

// ============================================================================
// Example 15: Basic Authentication
// ============================================================================

fn testBasicAuth() {
    Std.IO.println("=== Example 15: Basic Authentication ===");
    
    let response = Std.Network.httpGetWithBasicAuth(
        "https://httpbin.org/basic-auth/user/pass",
        "user",
        "pass"
    );
    
    Std.IO.println("Authenticated Response:");
    if (Std.String.length(response) > 0) {
        Std.IO.println("  ✓ Authentication successful");
    } else {
        Std.IO.println("  ✗ Authentication failed");
    }
    Std.IO.println("");
}

// ============================================================================
// Comprehensive Network Test Suite
// ============================================================================

fn runNetworkTestSuite() {
    Std.IO.println("╔════════════════════════════════════════════════════════╗");
    Std.IO.println("║     Magolor Network Module - Comprehensive Tests      ║");
    Std.IO.println("╚════════════════════════════════════════════════════════╝");
    Std.IO.println("");
    
    let startTime = Std.Time.now();
    
    // Run all tests
    testSimpleGet();
    testGetJson();
    testPostJson();
    testSimpleDownload();
    testUrlUtilities();
    testDnsResolution();
    testNetworkUtilities();
    testStatusCodes();
    testContentTypeDetection();
    testWebScraper();
    testRateLimitedRequests();
    testHttpMethods();
    testErrorHandling();
    testFileDownload();
    testBasicAuth();
    
    let endTime = Std.Time.now();
    let duration = endTime - startTime;
    
    Std.IO.println("╔════════════════════════════════════════════════════════╗");
    Std.IO.println("║              All Tests Completed!                      ║");
    Std.IO.print("║  Total time: ");
    Std.IO.print(Std.String.toString(duration));
    Std.IO.println(" seconds                           ║");
    Std.IO.println("╚════════════════════════════════════════════════════════╝");
}

// ============================================================================
// Main Entry Point
// ============================================================================

fn main() {
    // Run the comprehensive test suite
    runNetworkTestSuite();
    
    // Interactive menu for running individual tests
    Std.IO.println("");
    Std.IO.println("Would you like to run a specific test? (y/n)");
    let choice = Std.IO.readLine();
    
    if (choice == "y") {
        Std.IO.println("");
        Std.IO.println("Available tests:");
        Std.IO.println("  1.  Simple HTTP GET");
        Std.IO.println("  2.  HTTP GET with JSON");
        Std.IO.println("  3.  HTTP POST with JSON");
        Std.IO.println("  4.  Simple Download");
        Std.IO.println("  5.  URL Utilities");
        Std.IO.println("  6.  DNS Resolution");
        Std.IO.println("  7.  Network Utilities");
        Std.IO.println("  8.  HTTP Status Codes");
        Std.IO.println("  9.  Content Type Detection");
        Std.IO.println("  10. Web Scraper");
        Std.IO.println("  11. Rate-Limited Requests");
        Std.IO.println("  12. HTTP Methods");
        Std.IO.println("  13. Error Handling");
        Std.IO.println("  14. File Download");
        Std.IO.println("  15. Basic Authentication");
        Std.IO.println("");
        Std.IO.println("Enter test number (1-15):");
        
        let testNum = Std.String.toInt(Std.IO.readLine());
        Std.IO.println("");
        
        if (testNum == 1) {
            testSimpleGet();
        } else if (testNum == 2) {
            testGetJson();
        } else if (testNum == 3) {
            testPostJson();
        } else if (testNum == 4) {
            testSimpleDownload();
        } else if (testNum == 5) {
            testUrlUtilities();
        } else if (testNum == 6) {
            testDnsResolution();
        } else if (testNum == 7) {
            testNetworkUtilities();
        } else if (testNum == 8) {
            testStatusCodes();
        } else if (testNum == 9) {
            testContentTypeDetection();
        } else if (testNum == 10) {
            testWebScraper();
        } else if (testNum == 11) {
            testRateLimitedRequests();
        } else if (testNum == 12) {
            testHttpMethods();
        } else if (testNum == 13) {
            testErrorHandling();
        } else if (testNum == 14) {
            testFileDownload();
        } else if (testNum == 15) {
            testBasicAuth();
        } else {
            Std.IO.println("Invalid test number!");
        }
    }
    
    Std.IO.println("");
    Std.IO.println("Thank you for testing the Magolor Network Module!");
}
