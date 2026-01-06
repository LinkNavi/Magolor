#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// String structure for Magolor
typedef struct {
    int32_t length;
    char* data;
} mg_string;

// Array structure for Magolor
typedef struct {
    int32_t length;
    int32_t capacity;
    void* data;
    size_t element_size;
} mg_array;

// Option structure for Magolor
typedef struct {
    uint8_t has_value;
    void* value;
} mg_option;

// ============================================================================
// String operations
// ============================================================================

mg_string* mg_string_create(const char* str) {
    mg_string* s = (mg_string*)malloc(sizeof(mg_string));
    s->length = strlen(str);
    s->data = (char*)malloc(s->length + 1);
    strcpy(s->data, str);
    return s;
}

void mg_string_destroy(mg_string* s) {
    if (s) {
        free(s->data);
        free(s);
    }
}

mg_string* mg_string_concat(mg_string* a, mg_string* b) {
    mg_string* result = (mg_string*)malloc(sizeof(mg_string));
    result->length = a->length + b->length;
    result->data = (char*)malloc(result->length + 1);
    strcpy(result->data, a->data);
    strcat(result->data, b->data);
    return result;
}

int32_t mg_string_compare(mg_string* a, mg_string* b) {
    return strcmp(a->data, b->data);
}

// ============================================================================
// I/O operations
// ============================================================================

void mg_println(const char* str) {
    printf("%s\n", str);
}

void mg_print(const char* str) {
    printf("%s", str);
}

char* mg_readLine() {
    char* line = NULL;
    size_t len = 0;
    ssize_t read = getline(&line, &len, stdin);
    if (read != -1) {
        // Remove newline if present
        if (line[read - 1] == '\n') {
            line[read - 1] = '\0';
        }
        return line;
    }
    return NULL;
}

// ============================================================================
// Array operations
// ============================================================================

mg_array* mg_array_create(size_t element_size, int32_t initial_capacity) {
    mg_array* arr = (mg_array*)malloc(sizeof(mg_array));
    arr->length = 0;
    arr->capacity = initial_capacity > 0 ? initial_capacity : 8;
    arr->element_size = element_size;
    arr->data = malloc(arr->capacity * element_size);
    return arr;
}

void mg_array_destroy(mg_array* arr) {
    if (arr) {
        free(arr->data);
        free(arr);
    }
}

void mg_array_push(mg_array* arr, void* element) {
    if (arr->length >= arr->capacity) {
        arr->capacity *= 2;
        arr->data = realloc(arr->data, arr->capacity * arr->element_size);
    }
    memcpy((char*)arr->data + (arr->length * arr->element_size), 
           element, arr->element_size);
    arr->length++;
}

void* mg_array_get(mg_array* arr, int32_t index) {
    if (index < 0 || index >= arr->length) {
        return NULL;
    }
    return (char*)arr->data + (index * arr->element_size);
}

// ============================================================================
// Option operations
// ============================================================================

mg_option* mg_option_some(void* value, size_t value_size) {
    mg_option* opt = (mg_option*)malloc(sizeof(mg_option));
    opt->has_value = 1;
    opt->value = malloc(value_size);
    memcpy(opt->value, value, value_size);
    return opt;
}

mg_option* mg_option_none() {
    mg_option* opt = (mg_option*)malloc(sizeof(mg_option));
    opt->has_value = 0;
    opt->value = NULL;
    return opt;
}

void mg_option_destroy(mg_option* opt) {
    if (opt) {
        if (opt->value) {
            free(opt->value);
        }
        free(opt);
    }
}

uint8_t mg_option_is_some(mg_option* opt) {
    return opt->has_value;
}

void* mg_option_unwrap(mg_option* opt) {
    if (!opt->has_value) {
        fprintf(stderr, "Error: Called unwrap on None value\n");
        exit(1);
    }
    return opt->value;
}

// ============================================================================
// Memory management
// ============================================================================

void* mg_alloc(size_t size) {
    return malloc(size);
}

void mg_free(void* ptr) {
    free(ptr);
}

// ============================================================================
// Type conversions
// ============================================================================

char* mg_int_to_string(int32_t value) {
    char* buffer = (char*)malloc(32);
    snprintf(buffer, 32, "%d", value);
    return buffer;
}

char* mg_float_to_string(double value) {
    char* buffer = (char*)malloc(32);
    snprintf(buffer, 32, "%f", value);
    return buffer;
}

char* mg_bool_to_string(uint8_t value) {
    return value ? strdup("true") : strdup("false");
}

int32_t mg_string_to_int(const char* str) {
    return atoi(str);
}

double mg_string_to_float(const char* str) {
    return atof(str);
}
