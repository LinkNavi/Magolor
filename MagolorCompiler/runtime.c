// runtime.c - Enhanced Magolor Runtime Library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>

// ============================================
// String Structure for Better Memory Management
// ============================================

typedef struct {
    char* data;
    size_t length;
    size_t capacity;
} MagolorString;

MagolorString* string_new(const char* initial) {
    MagolorString* str = (MagolorString*)malloc(sizeof(MagolorString));
    if (!str) return NULL;
    
    size_t len = initial ? strlen(initial) : 0;
    str->capacity = len + 1;
    str->length = len;
    str->data = (char*)malloc(str->capacity);
    
    if (initial) {
        memcpy(str->data, initial, len);
    }
    str->data[len] = '\0';
    
    return str;
}

void string_free(MagolorString* str) {
    if (str) {
        free(str->data);
        free(str);
    }
}

MagolorString* string_concat(MagolorString* a, MagolorString* b) {
    if (!a || !b) return NULL;
    
    size_t new_len = a->length + b->length;
    MagolorString* result = (MagolorString*)malloc(sizeof(MagolorString));
    result->capacity = new_len + 1;
    result->length = new_len;
    result->data = (char*)malloc(result->capacity);
    
    memcpy(result->data, a->data, a->length);
    memcpy(result->data + a->length, b->data, b->length);
    result->data[new_len] = '\0';
    
    return result;
}

// ============================================
// Array Structure with Bounds Checking
// ============================================

typedef struct {
    void* data;
    size_t length;
    size_t element_size;
    size_t capacity;
} MagolorArray;

MagolorArray* array_new(size_t element_size, size_t initial_capacity) {
    MagolorArray* arr = (MagolorArray*)malloc(sizeof(MagolorArray));
    if (!arr) return NULL;
    
    arr->element_size = element_size;
    arr->capacity = initial_capacity > 0 ? initial_capacity : 16;
    arr->length = 0;
    arr->data = calloc(arr->capacity, element_size);
    
    return arr;
}

void array_free(MagolorArray* arr) {
    if (arr) {
        free(arr->data);
        free(arr);
    }
}

bool array_push(MagolorArray* arr, const void* element) {
    if (!arr || !element) return false;
    
    if (arr->length >= arr->capacity) {
        size_t new_capacity = arr->capacity * 2;
        void* new_data = realloc(arr->data, new_capacity * arr->element_size);
        if (!new_data) return false;
        
        arr->data = new_data;
        arr->capacity = new_capacity;
    }
    
    memcpy((char*)arr->data + (arr->length * arr->element_size), 
           element, arr->element_size);
    arr->length++;
    return true;
}

void* array_get(MagolorArray* arr, size_t index) {
    if (!arr || index >= arr->length) {
        fprintf(stderr, "Array index out of bounds: %zu >= %zu\n", index, arr->length);
        exit(1);
    }
    return (char*)arr->data + (index * arr->element_size);
}

// ============================================
// Console/Print Functions
// ============================================

void print_int(int32_t n) {
    printf("%d\n", n);
}

void print_i64(int64_t n) {
    printf("%lld\n", (long long)n);
}

void print_u32(uint32_t n) {
    printf("%u\n", n);
}

void print_u64(uint64_t n) {
    printf("%llu\n", (unsigned long long)n);
}

void print_f32(float f) {
    printf("%g\n", f);
}

void print_f64(double f) {
    printf("%g\n", f);
}

void print_str(const char* s) {
    if (s) {
        printf("%s\n", s);
    }
}

void print_char(char c) {
    putchar(c);
    putchar('\n');
}

void print_bool(bool b) {
    printf("%s\n", b ? "true" : "false");
}

// No-newline versions
void print_int_nn(int32_t n) {
    printf("%d", n);
}

void print_str_nn(const char* s) {
    if (s) printf("%s", s);
}

void print_f32_nn(float f) {
    printf("%g", f);
}

void print_f64_nn(double f) {
    printf("%g", f);
}

// ============================================
// String Operations
// ============================================

int32_t string_length(const char* s) {
    return s ? (int32_t)strlen(s) : 0;
}

char* string_concat_cstr(const char* a, const char* b) {
    if (!a) a = "";
    if (!b) b = "";
    
    size_t len = strlen(a) + strlen(b) + 1;
    char* result = (char*)malloc(len);
    strcpy(result, a);
    strcat(result, b);
    return result;
}

char* string_substring(const char* s, int32_t start, int32_t length) {
    if (!s) return NULL;
    
    int32_t len = (int32_t)strlen(s);
    if (start < 0 || start >= len) return NULL;
    
    int32_t actual_len = (start + length > len) ? (len - start) : length;
    char* result = (char*)malloc(actual_len + 1);
    memcpy(result, s + start, actual_len);
    result[actual_len] = '\0';
    return result;
}

int32_t string_indexof(const char* haystack, const char* needle) {
    if (!haystack || !needle) return -1;
    
    char* pos = strstr(haystack, needle);
    return pos ? (int32_t)(pos - haystack) : -1;
}

bool string_contains(const char* haystack, const char* needle) {
    return string_indexof(haystack, needle) >= 0;
}

bool string_equals(const char* a, const char* b) {
    if (a == b) return true;
    if (!a || !b) return false;
    return strcmp(a, b) == 0;
}

char* string_to_upper(const char* s) {
    if (!s) return NULL;
    
    size_t len = strlen(s);
    char* result = (char*)malloc(len + 1);
    for (size_t i = 0; i < len; i++) {
        result[i] = (s[i] >= 'a' && s[i] <= 'z') ? (s[i] - 32) : s[i];
    }
    result[len] = '\0';
    return result;
}

char* string_to_lower(const char* s) {
    if (!s) return NULL;
    
    size_t len = strlen(s);
    char* result = (char*)malloc(len + 1);
    for (size_t i = 0; i < len; i++) {
        result[i] = (s[i] >= 'A' && s[i] <= 'Z') ? (s[i] + 32) : s[i];
    }
    result[len] = '\0';
    return result;
}

// ============================================
// Math Functions
// ============================================

int32_t math_abs_i32(int32_t x) {
    return abs(x);
}

int64_t math_abs_i64(int64_t x) {
    return llabs(x);
}

float math_abs_f32(float x) {
    return fabsf(x);
}

double math_abs_f64(double x) {
    return fabs(x);
}

double math_sqrt(double x) {
    return sqrt(x);
}

double math_pow(double x, double y) {
    return pow(x, y);
}

double math_sin(double x) {
    return sin(x);
}

double math_cos(double x) {
    return cos(x);
}

double math_tan(double x) {
    return tan(x);
}

double math_floor(double x) {
    return floor(x);
}

double math_ceil(double x) {
    return ceil(x);
}

double math_round(double x) {
    return round(x);
}

int32_t math_min_i32(int32_t a, int32_t b) {
    return a < b ? a : b;
}

int32_t math_max_i32(int32_t a, int32_t b) {
    return a > b ? a : b;
}

float math_min_f32(float a, float b) {
    return a < b ? a : b;
}

float math_max_f32(float a, float b) {
    return a > b ? a : b;
}

double math_log(double x) {
    return log(x);
}

double math_log10(double x) {
    return log10(x);
}

double math_exp(double x) {
    return exp(x);
}

// ============================================
// Type Conversion Functions
// ============================================

char* int_to_string(int32_t n) {
    char* buf = (char*)malloc(32);
    snprintf(buf, 32, "%d", n);
    return buf;
}

char* i64_to_string(int64_t n) {
    char* buf = (char*)malloc(32);
    snprintf(buf, 32, "%lld", (long long)n);
    return buf;
}

char* float_to_string(double f) {
    char* buf = (char*)malloc(32);
    snprintf(buf, 32, "%g", f);
    return buf;
}

int32_t string_to_int(const char* s) {
    if (!s) return 0;
    return atoi(s);
}

double string_to_float(const char* s) {
    if (!s) return 0.0;
    return atof(s);
}

// ============================================
// Memory Management Helpers
// ============================================

void* magolor_alloc(size_t size) {
    void* ptr = malloc(size);
    if (!ptr && size > 0) {
        fprintf(stderr, "Memory allocation failed: %zu bytes\n", size);
        exit(1);
    }
    return ptr;
}

void* magolor_calloc(size_t count, size_t size) {
    void* ptr = calloc(count, size);
    if (!ptr && count > 0 && size > 0) {
        fprintf(stderr, "Memory allocation failed: %zu * %zu bytes\n", count, size);
        exit(1);
    }
    return ptr;
}

void* magolor_realloc(void* ptr, size_t new_size) {
    void* new_ptr = realloc(ptr, new_size);
    if (!new_ptr && new_size > 0) {
        fprintf(stderr, "Memory reallocation failed: %zu bytes\n", new_size);
        exit(1);
    }
    return new_ptr;
}

void magolor_free(void* ptr) {
    free(ptr);
}

// ============================================
// Time/Clock Functions
// ============================================

int64_t time_now_millis() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

double time_now_seconds() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

void time_sleep_millis(int64_t milliseconds) {
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

// ============================================
// File I/O Helpers
// ============================================

char* file_read_all(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) return NULL;
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char* content = (char*)malloc(size + 1);
    if (!content) {
        fclose(f);
        return NULL;
    }
    
    fread(content, 1, size, f);
    content[size] = '\0';
    fclose(f);
    
    return content;
}

bool file_write_all(const char* filename, const char* content) {
    FILE* f = fopen(filename, "wb");
    if (!f) return false;
    
    size_t len = strlen(content);
    size_t written = fwrite(content, 1, len, f);
    fclose(f);
    
    return written == len;
}

bool file_exists(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}

// ============================================
// Random Number Generation
// ============================================

void random_seed(uint32_t seed) {
    srand(seed);
}

int32_t random_int(int32_t min, int32_t max) {
    return min + rand() % (max - min + 1);
}

double random_float() {
    return (double)rand() / (double)RAND_MAX;
}

// ============================================
// Panic/Assert Functions
// ============================================

void magolor_panic(const char* message) {
    fprintf(stderr, "PANIC: %s\n", message);
    exit(1);
}

void magolor_assert(bool condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "ASSERTION FAILED: %s\n", message);
        exit(1);
    }
}

// ============================================
// Debug Helpers
// ============================================

void debug_print_ptr(const char* name, void* ptr) {
    printf("[DEBUG] %s = %p\n", name, ptr);
}

void debug_print_int(const char* name, int32_t value) {
    printf("[DEBUG] %s = %d\n", name, value);
}

void debug_print_float(const char* name, double value) {
    printf("[DEBUG] %s = %g\n", name, value);
}
