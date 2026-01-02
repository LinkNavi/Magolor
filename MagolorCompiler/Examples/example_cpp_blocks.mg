using Std.IO;

cimport <stdio.h> (printf);
cimport <math.h> as Math;

fn demonstrateBasicMath() {
    Std.print("=== Basic Math ===\n");
    let result = Math::sqrt(16.0);
    Std.print($"sqrt(16) = {result}\n");
}
class Name {
	pub field: int;
	
	pub fn method() {
		
	}
}
fn demonstrateInlineCpp() {
    Std.print("\n=== Inline C++ Demo ===\n");
    
    // Use @cpp for C++ features not directly available in Magolor
    @cpp {
        // You can use any C++ code here!
        std::vector<int> numbers = {1, 2, 3, 4, 5};
        
        std::cout << "Vector contents: ";
        for (int num : numbers) {
            std::cout << num << " ";
        }
        std::cout << "\n";
        
        // Use C++ STL algorithms
        int sum = 0;
        for (int n : numbers) sum += n;
        std::cout << "Sum: " << sum << "\n";
    }
    
    Std.print("Back in Magolor code!\n");
}

fn demonstrateCppTemplates() {
    Std.print("\n=== C++ Templates ===\n");
    
    @cpp {
        // Use std::map (templates!)
        std::map<std::string, int> ages;
        ages["Alice"] = 30;
        ages["Bob"] = 25;
        ages["Charlie"] = 35;
        
        std::cout << "Ages:\n";
        for (const auto& [name, age] : ages) {
            std::cout << "  " << name << ": " << age << "\n";
        }
    }
}

fn demonstrateMixedCode() {
    Std.print("\n=== Mixed Magolor/C++ ===\n");
    
    // Magolor code
    let count = 5;
    Std.print($"Generating {count} random numbers:\n");
    
    // Switch to C++ for complex operations
    @cpp {
        #include <random>
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1, 100);
        
        for (int i = 0; i < 5; i++) {
            std::cout << "  Random " << i << ": " << dis(gen) << "\n";
        }
    }
    
    // Back to Magolor
    Std.print("Done with random generation!\n");
}

fn demonstrateCppClasses() {
    Std.print("\n=== C++ Classes ===\n");
    
    @cpp {
        // Define and use a C++ class inline
        class Point {
        public:
            double x, y;
            Point(double x, double y) : x(x), y(y) {}
            
            double distance() const {
                return std::sqrt(x*x + y*y);
            }
        };
        
        Point p1(3.0, 4.0);
        Point p2(6.0, 8.0);
        
        std::cout << "Point 1 distance from origin: " << p1.distance() << "\n";
        std::cout << "Point 2 distance from origin: " << p2.distance() << "\n";
    }
}

fn demonstrateCppSmartPointers() {
    Std.print("\n=== C++ Smart Pointers ===\n");
    
    @cpp {
        // Use modern C++ features
        auto data = std::make_unique<std::vector<std::string>>();
        data->push_back("Hello");
        data->push_back("from");
        data->push_back("C++");
        
        std::cout << "Stored strings: ";
        for (const auto& str : *data) {
            std::cout << str << " ";
        }
        std::cout << "\n";
        
        // Automatic cleanup when data goes out of scope!
    }
}

fn main() {
    Std.print("╔════════════════════════════════════╗\n");
    Std.print("║  @cpp Inline C++ Demo              ║\n");
    Std.print("╚════════════════════════════════════╝\n\n");
    
    demonstrateBasicMath();
    demonstrateInlineCpp();
    demonstrateCppTemplates();
    demonstrateMixedCode();
    demonstrateCppClasses();
    demonstrateCppSmartPointers();
    
    Std.print("\n✅ All @cpp features working!\n");
    Std.print("\n💡 Tip: Use @cpp for C++ features not available in Magolor:\n");
    Std.print("   - Templates (vector, map, etc.)\n");
    Std.print("   - Smart pointers\n");
    Std.print("   - Complex STL algorithms\n");
    Std.print("   - Modern C++ features\n");
}
