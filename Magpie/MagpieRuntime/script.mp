// example_objects.mg - Example script demonstrating JS-like objects

void fn main() {
    // Create object literal
    let var person = {
        name: "Alice",
        age: 30,
        active: true
    };
    
    // Access properties
    print_str(person.name);
    print_int(person.age);
    
    // Modify properties
    person.age = 31;
    person.city = "New York";  // Add new property
    
    // Nested objects
    let var config = {
        server: {
            host: "localhost",
            port: 8080
        },
        debug: true
    };
    
    print_str(config.server.host);
    print_int(config.server.port);
    
    // Object with mixed types
    let var data = {
        count: 42,
        values: [1, 2, 3, 4, 5],
        metadata: {
            version: "1.0",
            author: "System"
        }
    };
    
    print_int(data.values[0]);
    print_str(data.metadata.version);
    
    // Dynamic property access using bracket notation
    let var key = "count";
    print_int(data[key]);
    
    // Object in array
    let var users = [
        { name: "Alice", score: 100 },
        { name: "Bob", score: 85 },
        { name: "Charlie", score: 92 }
    ];
    
    print_str(users[0].name);
    print_int(users[0].score);
    
    // Iterate over array of objects
    foreach (var user in users) {
        print_str(user.name);
        print_int(user.score);
    }
}
