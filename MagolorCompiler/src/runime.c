/*
 * Magolor Runtime Library
 * ========================
 * Core runtime support for the Magolor programming language.
 * This library provides memory management, string operations,
 * array handling, I/O, and other fundamental operations.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

/* ==========================================================================
 * Type Definitions
 * ========================================================================== */

// Reference-counted object header
typedef struct mg_header {
    int32_t refcount;
    int32_t type_id;
} mg_header;

// String type
typedef struct mg_string {
    mg_header header;
    int32_t length;
    int32_t capacity;
    char* data;
} mg_string;

// Array type (generic)
typedef struct mg_array {
    mg_header header;
    int32_t length;
    int32_t capacity;
    int32_t elem_size;
    void* data;
} mg_array;

// Option type
typedef struct mg_option {
    mg_header header;
    uint8_t has_value;
    int32_t value_size;
    void* value;
} mg_option;

// Map entry
typedef struct mg_map_entry {
    char* key;
    void* value;
    struct mg_map_entry* next;
} mg_map_entry;

// Hash map type
typedef struct mg_map {
    mg_header header;
    int32_t size;
    int32_t bucket_count;
    int32_t value_size;
    mg_map_entry** buckets;
} mg_map;

// Type IDs
#define MG_TYPE_STRING  1
#define MG_TYPE_ARRAY   2
#define MG_TYPE_OPTION  3
#define MG_TYPE_MAP     4
#define MG_TYPE_OBJECT  5

/* ==========================================================================
 * Memory Management
 * ========================================================================== */

static int64_t mg_total_allocations = 0;
static int64_t mg_total_frees = 0;
static bool mg_runtime_initialized = false;

void mg_runtime_init(void) {
    if (mg_runtime_initialized) return;
    mg_runtime_initialized = true;
    srand((unsigned int)time(NULL));
}

void mg_runtime_shutdown(void) {
    // Could add cleanup/statistics here
}

void* mg_alloc_simple(int64_t size) {
    mg_total_allocations++;
    return malloc(size);
}

void mg_free_simple(void* ptr) {
    if (ptr) {
        mg_total_frees++;
        free(ptr);
    }
}

void mg_incref(void* ptr) {
    if (!ptr) return;
    mg_header* header = (mg_header*)ptr;
    header->refcount++;
}

void mg_decref(void* ptr) {
    if (!ptr) return;
    mg_header* header = (mg_header*)ptr;
    header->refcount--;
    
    if (header->refcount <= 0) {
        switch (header->type_id) {
            case MG_TYPE_STRING: {
                mg_string* str = (mg_string*)ptr;
                free(str->data);
                break;
            }
            case MG_TYPE_ARRAY: {
                mg_array* arr = (mg_array*)ptr;
                free(arr->data);
                break;
            }
            case MG_TYPE_OPTION: {
                mg_option* opt = (mg_option*)ptr;
                if (opt->value) free(opt->value);
                break;
            }
            case MG_TYPE_MAP: {
                mg_map* map = (mg_map*)ptr;
                for (int i = 0; i < map->bucket_count; i++) {
                    mg_map_entry* entry = map->buckets[i];
                    while (entry) {
                        mg_map_entry* next = entry->next;
                        free(entry->key);
                        free(entry->value);
                        free(entry);
                        entry = next;
                    }
                }
                free(map->buckets);
                break;
            }
        }
        free(ptr);
    }
}

/* ==========================================================================
 * String Operations
 * ========================================================================== */

mg_string* mg_string_create(const char* str) {
    if (!str) str = "";
    int32_t len = strlen(str);
    
    mg_string* s = (mg_string*)malloc(sizeof(mg_string));
    s->header.refcount = 1;
    s->header.type_id = MG_TYPE_STRING;
    s->length = len;
    s->capacity = len + 1;
    s->data = (char*)malloc(s->capacity);
    memcpy(s->data, str, len + 1);
    
    return s;
}

mg_string* mg_string_create_len(const char* str, int32_t len) {
    mg_string* s = (mg_string*)malloc(sizeof(mg_string));
    s->header.refcount = 1;
    s->header.type_id = MG_TYPE_STRING;
    s->length = len;
    s->capacity = len + 1;
    s->data = (char*)malloc(s->capacity);
    if (str) memcpy(s->data, str, len);
    s->data[len] = '\0';
    
    return s;
}

void mg_string_destroy(mg_string* s) {
    if (s) {
        free(s->data);
        free(s);
    }
}

mg_string* mg_string_concat(mg_string* a, mg_string* b) {
    if (!a || !b) return mg_string_create("");
    
    int32_t new_len = a->length + b->length;
    mg_string* result = (mg_string*)malloc(sizeof(mg_string));
    result->header.refcount = 1;
    result->header.type_id = MG_TYPE_STRING;
    result->length = new_len;
    result->capacity = new_len + 1;
    result->data = (char*)malloc(result->capacity);
    
    memcpy(result->data, a->data, a->length);
    memcpy(result->data + a->length, b->data, b->length + 1);
    
    return result;
}

int32_t mg_string_length(mg_string* s) {
    return s ? s->length : 0;
}

int32_t mg_string_compare(mg_string* a, mg_string* b) {
    if (!a || !b) return a == b ? 0 : (a ? 1 : -1);
    return strcmp(a->data, b->data);
}

bool mg_string_equals(mg_string* a, mg_string* b) {
    return mg_string_compare(a, b) == 0;
}

mg_string* mg_string_substring(mg_string* s, int32_t start, int32_t length) {
    if (!s || start < 0 || start >= s->length) {
        return mg_string_create("");
    }
    
    if (length < 0 || start + length > s->length) {
        length = s->length - start;
    }
    
    return mg_string_create_len(s->data + start, length);
}

int32_t mg_string_index_of(mg_string* s, mg_string* substr) {
    if (!s || !substr) return -1;
    char* pos = strstr(s->data, substr->data);
    return pos ? (int32_t)(pos - s->data) : -1;
}

bool mg_string_starts_with(mg_string* s, mg_string* prefix) {
    if (!s || !prefix) return false;
    if (prefix->length > s->length) return false;
    return strncmp(s->data, prefix->data, prefix->length) == 0;
}

bool mg_string_ends_with(mg_string* s, mg_string* suffix) {
    if (!s || !suffix) return false;
    if (suffix->length > s->length) return false;
    return strcmp(s->data + s->length - suffix->length, suffix->data) == 0;
}

bool mg_string_contains(mg_string* s, mg_string* substr) {
    return mg_string_index_of(s, substr) >= 0;
}

mg_string* mg_string_trim(mg_string* s) {
    if (!s || s->length == 0) return mg_string_create("");
    
    int32_t start = 0, end = s->length - 1;
    
    while (start <= end && (s->data[start] == ' ' || s->data[start] == '\t' ||
                            s->data[start] == '\n' || s->data[start] == '\r')) {
        start++;
    }
    
    while (end >= start && (s->data[end] == ' ' || s->data[end] == '\t' ||
                            s->data[end] == '\n' || s->data[end] == '\r')) {
        end--;
    }
    
    return mg_string_create_len(s->data + start, end - start + 1);
}

mg_string* mg_string_to_lower(mg_string* s) {
    if (!s) return mg_string_create("");
    
    mg_string* result = mg_string_create_len(s->data, s->length);
    for (int32_t i = 0; i < result->length; i++) {
        if (result->data[i] >= 'A' && result->data[i] <= 'Z') {
            result->data[i] += 32;
        }
    }
    return result;
}

mg_string* mg_string_to_upper(mg_string* s) {
    if (!s) return mg_string_create("");
    
    mg_string* result = mg_string_create_len(s->data, s->length);
    for (int32_t i = 0; i < result->length; i++) {
        if (result->data[i] >= 'a' && result->data[i] <= 'z') {
            result->data[i] -= 32;
        }
    }
    return result;
}

mg_string* mg_string_replace(mg_string* s, mg_string* from, mg_string* to) {
    if (!s || !from || !to || from->length == 0) {
        return s ? mg_string_create(s->data) : mg_string_create("");
    }
    
    // Count occurrences
    int count = 0;
    char* pos = s->data;
    while ((pos = strstr(pos, from->data)) != NULL) {
        count++;
        pos += from->length;
    }
    
    if (count == 0) return mg_string_create(s->data);
    
    // Calculate new length
    int32_t new_len = s->length + count * (to->length - from->length);
    mg_string* result = mg_string_create_len(NULL, new_len);
    
    char* src = s->data;
    char* dst = result->data;
    
    while ((pos = strstr(src, from->data)) != NULL) {
        int32_t prefix_len = pos - src;
        memcpy(dst, src, prefix_len);
        dst += prefix_len;
        memcpy(dst, to->data, to->length);
        dst += to->length;
        src = pos + from->length;
    }
    
    strcpy(dst, src);
    return result;
}

mg_string* mg_string_repeat(mg_string* s, int32_t count) {
    if (!s || count <= 0) return mg_string_create("");
    
    int32_t new_len = s->length * count;
    mg_string* result = mg_string_create_len(NULL, new_len);
    
    char* dst = result->data;
    for (int32_t i = 0; i < count; i++) {
        memcpy(dst, s->data, s->length);
        dst += s->length;
    }
    *dst = '\0';
    
    return result;
}

char mg_string_char_at(mg_string* s, int32_t index) {
    if (!s || index < 0 || index >= s->length) return '\0';
    return s->data[index];
}

mg_array* mg_string_split(mg_string* s, char delim) {
    mg_array* arr = (mg_array*)malloc(sizeof(mg_array));
    arr->header.refcount = 1;
    arr->header.type_id = MG_TYPE_ARRAY;
    arr->elem_size = sizeof(mg_string*);
    arr->length = 0;
    arr->capacity = 8;
    arr->data = malloc(arr->capacity * arr->elem_size);
    
    if (!s || s->length == 0) return arr;
    
    char* start = s->data;
    char* end;
    
    while ((end = strchr(start, delim)) != NULL) {
        mg_string* part = mg_string_create_len(start, end - start);
        
        if (arr->length >= arr->capacity) {
            arr->capacity *= 2;
            arr->data = realloc(arr->data, arr->capacity * arr->elem_size);
        }
        
        ((mg_string**)arr->data)[arr->length++] = part;
        start = end + 1;
    }
    
    // Add last part
    mg_string* part = mg_string_create(start);
    if (arr->length >= arr->capacity) {
        arr->capacity *= 2;
        arr->data = realloc(arr->data, arr->capacity * arr->elem_size);
    }
    ((mg_string**)arr->data)[arr->length++] = part;
    
    return arr;
}

mg_string* mg_string_join(mg_array* arr, mg_string* sep) {
    if (!arr || arr->length == 0) return mg_string_create("");
    
    // Calculate total length
    int32_t total = 0;
    mg_string** strings = (mg_string**)arr->data;
    
    for (int32_t i = 0; i < arr->length; i++) {
        if (strings[i]) total += strings[i]->length;
        if (i > 0 && sep) total += sep->length;
    }
    
    mg_string* result = mg_string_create_len(NULL, total);
    char* dst = result->data;
    
    for (int32_t i = 0; i < arr->length; i++) {
        if (i > 0 && sep) {
            memcpy(dst, sep->data, sep->length);
            dst += sep->length;
        }
        if (strings[i]) {
            memcpy(dst, strings[i]->data, strings[i]->length);
            dst += strings[i]->length;
        }
    }
    *dst = '\0';
    
    return result;
}

/* ==========================================================================
 * Array Operations
 * ========================================================================== */

mg_array* mg_array_create(int32_t elem_size, int32_t initial_capacity) {
    mg_array* arr = (mg_array*)malloc(sizeof(mg_array));
    arr->header.refcount = 1;
    arr->header.type_id = MG_TYPE_ARRAY;
    arr->elem_size = elem_size;
    arr->length = 0;
    arr->capacity = initial_capacity > 0 ? initial_capacity : 8;
    arr->data = malloc(arr->capacity * elem_size);
    return arr;
}

void mg_array_destroy(mg_array* arr) {
    if (arr) {
        free(arr->data);
        free(arr);
    }
}

int32_t mg_array_length(mg_array* arr) {
    return arr ? arr->length : 0;
}

void mg_array_push(mg_array** arr_ptr, void* elem) {
    if (!arr_ptr || !*arr_ptr) return;
    mg_array* arr = *arr_ptr;
    
    if (arr->length >= arr->capacity) {
        arr->capacity *= 2;
        arr->data = realloc(arr->data, arr->capacity * arr->elem_size);
    }
    
    memcpy((char*)arr->data + arr->length * arr->elem_size, elem, arr->elem_size);
    arr->length++;
}

void* mg_array_pop(mg_array* arr) {
    if (!arr || arr->length == 0) return NULL;
    arr->length--;
    return (char*)arr->data + arr->length * arr->elem_size;
}

void* mg_array_get(mg_array* arr, int32_t index) {
    if (!arr || index < 0 || index >= arr->length) return NULL;
    return (char*)arr->data + index * arr->elem_size;
}

void mg_array_set(mg_array* arr, int32_t index, void* elem) {
    if (!arr || index < 0 || index >= arr->length) return;
    memcpy((char*)arr->data + index * arr->elem_size, elem, arr->elem_size);
}

void mg_array_clear(mg_array* arr) {
    if (arr) arr->length = 0;
}

void mg_array_reverse(mg_array* arr) {
    if (!arr || arr->length <= 1) return;
    
    char* temp = (char*)malloc(arr->elem_size);
    char* data = (char*)arr->data;
    
    for (int32_t i = 0; i < arr->length / 2; i++) {
        int32_t j = arr->length - 1 - i;
        memcpy(temp, data + i * arr->elem_size, arr->elem_size);
        memcpy(data + i * arr->elem_size, data + j * arr->elem_size, arr->elem_size);
        memcpy(data + j * arr->elem_size, temp, arr->elem_size);
    }
    
    free(temp);
}

/* ==========================================================================
 * Option Operations
 * ========================================================================== */

mg_option* mg_option_some(void* value, int32_t value_size) {
    mg_option* opt = (mg_option*)malloc(sizeof(mg_option));
    opt->header.refcount = 1;
    opt->header.type_id = MG_TYPE_OPTION;
    opt->has_value = 1;
    opt->value_size = value_size;
    opt->value = malloc(value_size);
    memcpy(opt->value, value, value_size);
    return opt;
}

mg_option* mg_option_none(void) {
    mg_option* opt = (mg_option*)malloc(sizeof(mg_option));
    opt->header.refcount = 1;
    opt->header.type_id = MG_TYPE_OPTION;
    opt->has_value = 0;
    opt->value_size = 0;
    opt->value = NULL;
    return opt;
}

void mg_option_destroy(mg_option* opt) {
    if (opt) {
        if (opt->value) free(opt->value);
        free(opt);
    }
}

bool mg_option_is_some(mg_option* opt) {
    return opt && opt->has_value;
}

bool mg_option_is_none(mg_option* opt) {
    return !opt || !opt->has_value;
}

void* mg_option_unwrap(mg_option* opt) {
    if (!opt || !opt->has_value) {
        fprintf(stderr, "Error: Called unwrap on None value\n");
        exit(1);
    }
    return opt->value;
}

void* mg_option_unwrap_or(mg_option* opt, void* default_value) {
    if (opt && opt->has_value) return opt->value;
    return default_value;
}

/* ==========================================================================
 * I/O Operations
 * ========================================================================== */

void mg_print(const char* str) {
    if (str) printf("%s", str);
}

void mg_println(const char* str) {
    if (str) printf("%s\n", str);
    else printf("\n");
}

void mg_print_int(int32_t val) {
    printf("%d", val);
}

void mg_println_int(int32_t val) {
    printf("%d\n", val);
}

void mg_print_float(double val) {
    printf("%g", val);
}

void mg_println_float(double val) {
    printf("%g\n", val);
}

void mg_print_bool(bool val) {
    printf("%s", val ? "true" : "false");
}

void mg_println_bool(bool val) {
    printf("%s\n", val ? "true" : "false");
}

void mg_eprint(const char* str) {
    if (str) fprintf(stderr, "%s", str);
}

void mg_eprintln(const char* str) {
    if (str) fprintf(stderr, "%s\n", str);
    else fprintf(stderr, "\n");
}

mg_string* mg_readLine(void) {
    char* line = NULL;
    size_t len = 0;
    ssize_t read = getline(&line, &len, stdin);
    
    if (read == -1) {
        free(line);
        return mg_string_create("");
    }
    
    // Remove trailing newline
    if (read > 0 && line[read - 1] == '\n') {
        line[read - 1] = '\0';
        read--;
    }
    
    mg_string* result = mg_string_create(line);
    free(line);
    return result;
}

char mg_readChar(void) {
    return (char)getchar();
}

/* ==========================================================================
 * Type Conversions
 * ========================================================================== */

mg_string* mg_int_to_string(int32_t val) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%d", val);
    return mg_string_create(buffer);
}

mg_string* mg_float_to_string(double val) {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%g", val);
    return mg_string_create(buffer);
}

mg_string* mg_bool_to_string(bool val) {
    return mg_string_create(val ? "true" : "false");
}

mg_string* mg_char_to_string(char c) {
    char buffer[2] = {c, '\0'};
    return mg_string_create(buffer);
}

int32_t mg_string_to_int(mg_string* s) {
    if (!s || s->length == 0) return 0;
    return atoi(s->data);
}

double mg_string_to_float(mg_string* s) {
    if (!s || s->length == 0) return 0.0;
    return atof(s->data);
}

bool mg_string_to_bool(mg_string* s) {
    if (!s) return false;
    return strcmp(s->data, "true") == 0 || strcmp(s->data, "1") == 0;
}

mg_option* mg_parse_int(mg_string* s) {
    if (!s || s->length == 0) return mg_option_none();
    
    char* endptr;
    long val = strtol(s->data, &endptr, 10);
    
    if (endptr == s->data || *endptr != '\0') {
        return mg_option_none();
    }
    
    int32_t result = (int32_t)val;
    return mg_option_some(&result, sizeof(result));
}

mg_option* mg_parse_float(mg_string* s) {
    if (!s || s->length == 0) return mg_option_none();
    
    char* endptr;
    double val = strtod(s->data, &endptr);
    
    if (endptr == s->data || *endptr != '\0') {
        return mg_option_none();
    }
    
    return mg_option_some(&val, sizeof(val));
}

/* ==========================================================================
 * Math Functions
 * ========================================================================== */

int32_t mg_abs_int(int32_t x) {
    return x < 0 ? -x : x;
}

double mg_abs_float(double x) {
    return fabs(x);
}

double mg_sqrt(double x) {
    return sqrt(x);
}

double mg_cbrt(double x) {
    return cbrt(x);
}

double mg_pow(double base, double exp) {
    return pow(base, exp);
}

double mg_exp(double x) {
    return exp(x);
}

double mg_log(double x) {
    return log(x);
}

double mg_log10(double x) {
    return log10(x);
}

double mg_log2(double x) {
    return log2(x);
}

double mg_sin(double x) {
    return sin(x);
}

double mg_cos(double x) {
    return cos(x);
}

double mg_tan(double x) {
    return tan(x);
}

double mg_asin(double x) {
    return asin(x);
}

double mg_acos(double x) {
    return acos(x);
}

double mg_atan(double x) {
    return atan(x);
}

double mg_atan2(double y, double x) {
    return atan2(y, x);
}

double mg_floor(double x) {
    return floor(x);
}

double mg_ceil(double x) {
    return ceil(x);
}

double mg_round(double x) {
    return round(x);
}

double mg_fmod(double x, double y) {
    return fmod(x, y);
}

int32_t mg_min_int(int32_t a, int32_t b) {
    return a < b ? a : b;
}

int32_t mg_max_int(int32_t a, int32_t b) {
    return a > b ? a : b;
}

double mg_min_float(double a, double b) {
    return a < b ? a : b;
}

double mg_max_float(double a, double b) {
    return a > b ? a : b;
}

int32_t mg_clamp_int(int32_t val, int32_t low, int32_t high) {
    return val < low ? low : (val > high ? high : val);
}

double mg_clamp_float(double val, double low, double high) {
    return val < low ? low : (val > high ? high : val);
}

/* ==========================================================================
 * Random Number Generation
 * ========================================================================== */

static bool mg_random_seeded = false;

void mg_random_init(void) {
    if (!mg_random_seeded) {
        srand((unsigned int)time(NULL) ^ (unsigned int)getpid());
        mg_random_seeded = true;
    }
}

int32_t mg_random_int(int32_t min, int32_t max) {
    mg_random_init();
    if (min >= max) return min;
    return min + rand() % (max - min + 1);
}

double mg_random_float(void) {
    mg_random_init();
    return (double)rand() / (double)RAND_MAX;
}

double mg_random_float_range(double min, double max) {
    return min + mg_random_float() * (max - min);
}

bool mg_random_bool(void) {
    return mg_random_int(0, 1) == 1;
}

/* ==========================================================================
 * File System Functions
 * ========================================================================== */

bool mg_file_exists(mg_string* path) {
    if (!path) return false;
    struct stat st;
    return stat(path->data, &st) == 0;
}

bool mg_file_is_file(mg_string* path) {
    if (!path) return false;
    struct stat st;
    if (stat(path->data, &st) != 0) return false;
    return S_ISREG(st.st_mode);
}

bool mg_file_is_directory(mg_string* path) {
    if (!path) return false;
    struct stat st;
    if (stat(path->data, &st) != 0) return false;
    return S_ISDIR(st.st_mode);
}

mg_option* mg_file_read(mg_string* path) {
    if (!path) return mg_option_none();
    
    FILE* f = fopen(path->data, "rb");
    if (!f) return mg_option_none();
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char* buffer = (char*)malloc(size + 1);
    size_t read = fread(buffer, 1, size, f);
    fclose(f);
    
    buffer[read] = '\0';
    mg_string* content = mg_string_create(buffer);
    free(buffer);
    
    return mg_option_some(&content, sizeof(content));
}

bool mg_file_write(mg_string* path, mg_string* content) {
    if (!path || !content) return false;
    
    FILE* f = fopen(path->data, "wb");
    if (!f) return false;
    
    size_t written = fwrite(content->data, 1, content->length, f);
    fclose(f);
    
    return written == (size_t)content->length;
}

bool mg_file_append(mg_string* path, mg_string* content) {
    if (!path || !content) return false;
    
    FILE* f = fopen(path->data, "ab");
    if (!f) return false;
    
    size_t written = fwrite(content->data, 1, content->length, f);
    fclose(f);
    
    return written == (size_t)content->length;
}

bool mg_file_delete(mg_string* path) {
    if (!path) return false;
    return remove(path->data) == 0;
}

bool mg_dir_create(mg_string* path) {
    if (!path) return false;
    return mkdir(path->data, 0755) == 0;
}

bool mg_dir_delete(mg_string* path) {
    if (!path) return false;
    return rmdir(path->data) == 0;
}

int64_t mg_file_size(mg_string* path) {
    if (!path) return -1;
    struct stat st;
    if (stat(path->data, &st) != 0) return -1;
    return st.st_size;
}

/* ==========================================================================
 * Time Functions
 * ========================================================================== */

int64_t mg_time_now(void) {
    return (int64_t)time(NULL);
}

int64_t mg_time_now_millis(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

void mg_time_sleep(int32_t millis) {
    usleep(millis * 1000);
}

mg_string* mg_time_format(int64_t timestamp, mg_string* format) {
    time_t t = (time_t)timestamp;
    struct tm* tm_info = localtime(&t);
    
    char buffer[256];
    const char* fmt = format ? format->data : "%Y-%m-%d %H:%M:%S";
    strftime(buffer, sizeof(buffer), fmt, tm_info);
    
    return mg_string_create(buffer);
}

/* ==========================================================================
 * System Functions
 * ========================================================================== */

mg_option* mg_env_get(mg_string* name) {
    if (!name) return mg_option_none();
    
    char* val = getenv(name->data);
    if (!val) return mg_option_none();
    
    mg_string* result = mg_string_create(val);
    return mg_option_some(&result, sizeof(result));
}

bool mg_env_set(mg_string* name, mg_string* value) {
    if (!name || !value) return false;
    return setenv(name->data, value->data, 1) == 0;
}

int32_t mg_system_exec(mg_string* command) {
    if (!command) return -1;
    return system(command->data);
}

void mg_exit(int32_t code) {
    exit(code);
}

/* ==========================================================================
 * Map Functions
 * ========================================================================== */

static uint32_t mg_hash_string(const char* str) {
    uint32_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

mg_map* mg_map_create(int32_t value_size) {
    mg_map* map = (mg_map*)malloc(sizeof(mg_map));
    map->header.refcount = 1;
    map->header.type_id = MG_TYPE_MAP;
    map->size = 0;
    map->bucket_count = 16;
    map->value_size = value_size;
    map->buckets = (mg_map_entry**)calloc(map->bucket_count, sizeof(mg_map_entry*));
    return map;
}

void mg_map_destroy(mg_map* map) {
    if (!map) return;
    
    for (int i = 0; i < map->bucket_count; i++) {
        mg_map_entry* entry = map->buckets[i];
        while (entry) {
            mg_map_entry* next = entry->next;
            free(entry->key);
            free(entry->value);
            free(entry);
            entry = next;
        }
    }
    
    free(map->buckets);
    free(map);
}

void mg_map_set(mg_map* map, mg_string* key, void* value) {
    if (!map || !key) return;
    
    uint32_t hash = mg_hash_string(key->data);
    int bucket = hash % map->bucket_count;
    
    // Check if key exists
    mg_map_entry* entry = map->buckets[bucket];
    while (entry) {
        if (strcmp(entry->key, key->data) == 0) {
            memcpy(entry->value, value, map->value_size);
            return;
        }
        entry = entry->next;
    }
    
    // Create new entry
    entry = (mg_map_entry*)malloc(sizeof(mg_map_entry));
    entry->key = strdup(key->data);
    entry->value = malloc(map->value_size);
    memcpy(entry->value, value, map->value_size);
    entry->next = map->buckets[bucket];
    map->buckets[bucket] = entry;
    map->size++;
}

void* mg_map_get(mg_map* map, mg_string* key) {
    if (!map || !key) return NULL;
    
    uint32_t hash = mg_hash_string(key->data);
    int bucket = hash % map->bucket_count;
    
    mg_map_entry* entry = map->buckets[bucket];
    while (entry) {
        if (strcmp(entry->key, key->data) == 0) {
            return entry->value;
        }
        entry = entry->next;
    }
    
    return NULL;
}

bool mg_map_has(mg_map* map, mg_string* key) {
    return mg_map_get(map, key) != NULL;
}

bool mg_map_remove(mg_map* map, mg_string* key) {
    if (!map || !key) return false;
    
    uint32_t hash = mg_hash_string(key->data);
    int bucket = hash % map->bucket_count;
    
    mg_map_entry** prev = &map->buckets[bucket];
    mg_map_entry* entry = *prev;
    
    while (entry) {
        if (strcmp(entry->key, key->data) == 0) {
            *prev = entry->next;
            free(entry->key);
            free(entry->value);
            free(entry);
            map->size--;
            return true;
        }
        prev = &entry->next;
        entry = entry->next;
    }
    
    return false;
}

int32_t mg_map_size(mg_map* map) {
    return map ? map->size : 0;
}

void mg_map_clear(mg_map* map) {
    if (!map) return;
    
    for (int i = 0; i < map->bucket_count; i++) {
        mg_map_entry* entry = map->buckets[i];
        while (entry) {
            mg_map_entry* next = entry->next;
            free(entry->key);
            free(entry->value);
            free(entry);
            entry = next;
        }
        map->buckets[i] = NULL;
    }
    map->size = 0;
}

/* ==========================================================================
 * Panic/Assert Functions
 * ========================================================================== */

void mg_panic(const char* message) {
    fprintf(stderr, "PANIC: %s\n", message ? message : "unknown error");
    exit(1);
}

void mg_assert(bool condition, const char* message) {
    if (!condition) {
        mg_panic(message);
    }
}

/* ==========================================================================
 * Entry Point Wrapper
 * ========================================================================== */

// The user's main function
extern int mg_main(void);

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    mg_runtime_init();
    int result = mg_main();
    mg_runtime_shutdown();
    
    return result;
}
