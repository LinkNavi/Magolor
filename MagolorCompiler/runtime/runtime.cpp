// Magolor Runtime Library
// This provides runtime support for LLVM-compiled code

#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <unordered_map>

// ============================================================================
// String operations
// ============================================================================

extern "C" {

// String concatenation
const char* _mg_string_concat(const char* a, const char* b) {
    if (!a) a = "";
    if (!b) b = "";
    
    size_t len_a = strlen(a);
    size_t len_b = strlen(b);
    char* result = (char*)malloc(len_a + len_b + 1);
    
    memcpy(result, a, len_a);
    memcpy(result + len_a, b, len_b);
    result[len_a + len_b] = '\0';
    
    return result;
}

// Convert integer to string
const char* _mg_int_to_string(int64_t value) {
    char* buffer = (char*)malloc(32);
    snprintf(buffer, 32, "%lld", (long long)value);
    return buffer;
}

// Convert double to string
const char* _mg_double_to_string(double value) {
    char* buffer = (char*)malloc(32);
    snprintf(buffer, 32, "%g", value);
    return buffer;
}

// Convert bool to string
const char* _mg_bool_to_string(bool value) {
    return value ? "true" : "false";
}

// ============================================================================
// I/O operations
// ============================================================================

void print(const char* str) {
    if (str) {
        std::cout << str;
    }
}

void println(const char* str) {
    if (str) {
        std::cout << str << std::endl;
    } else {
        std::cout << std::endl;
    }
}

void eprint(const char* str) {
    if (str) {
        std::cerr << str;
    }
}

void eprintln(const char* str) {
    if (str) {
        std::cerr << str << std::endl;
    } else {
        std::cerr << std::endl;
    }
}

// ============================================================================
// Array operations (simplified - uses runtime heap structures)
// ============================================================================

struct MgArray {
    void* data;
    size_t length;
    size_t capacity;
    size_t element_size;
};

void* _mg_array_new() {
    MgArray* arr = new MgArray();
    arr->data = nullptr;
    arr->length = 0;
    arr->capacity = 0;
    arr->element_size = sizeof(void*);  // Generic pointer size
    return arr;
}

void _mg_array_push(void* arr_ptr, void* element) {
    MgArray* arr = (MgArray*)arr_ptr;
    
    if (arr->length >= arr->capacity) {
        size_t new_capacity = arr->capacity == 0 ? 4 : arr->capacity * 2;
        void* new_data = realloc(arr->data, new_capacity * arr->element_size);
        arr->data = new_data;
        arr->capacity = new_capacity;
    }
    
    void** data = (void**)arr->data;
    data[arr->length] = element;
    arr->length++;
}

void* _mg_array_get(void* arr_ptr, int64_t index) {
    MgArray* arr = (MgArray*)arr_ptr;
    
    if (index < 0 || (size_t)index >= arr->length) {
        std::cerr << "Array index out of bounds: " << index << std::endl;
        exit(1);
    }
    
    void** data = (void**)arr->data;
    return data[index];
}

int64_t _mg_array_length(void* arr_ptr) {
    MgArray* arr = (MgArray*)arr_ptr;
    return arr->length;
}

// ============================================================================
// Map operations
// ============================================================================

void* _mg_map_new() {
    return new std::unordered_map<std::string, void*>();
}

void _mg_map_insert(void* map_ptr, const char* key, void* value) {
    auto* map = (std::unordered_map<std::string, void*>*)map_ptr;
    (*map)[std::string(key)] = value;
}

void* _mg_map_get(void* map_ptr, const char* key) {
    auto* map = (std::unordered_map<std::string, void*>*)map_ptr;
    auto it = map->find(std::string(key));
    if (it != map->end()) {
        return it->second;
    }
    return nullptr;
}

// ============================================================================
// Memory management
// ============================================================================

void* _mg_malloc(size_t size) {
    return malloc(size);
}

void _mg_free(void* ptr) {
    free(ptr);
}

// ============================================================================
// Math operations
// ============================================================================

double _mg_sqrt(double x) {
    return sqrt(x);
}

double _mg_pow(double x, double y) {
    return pow(x, y);
}

int64_t _mg_abs(int64_t x) {
    return x < 0 ? -x : x;
}

// ============================================================================
// Utility functions
// ============================================================================

void _mg_panic(const char* message) {
    std::cerr << "PANIC: " << (message ? message : "unknown error") << std::endl;
    exit(1);
}

void _mg_assert(bool condition, const char* message) {
    if (!condition) {
        _mg_panic(message);
    }
}

} // extern "C"
