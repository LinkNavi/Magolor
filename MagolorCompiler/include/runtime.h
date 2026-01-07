/*
 * Magolor Runtime Header
 * ======================
 * Declarations for the Magolor runtime library.
 */

#ifndef MG_RUNTIME_H
#define MG_RUNTIME_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==========================================================================
 * Type Definitions
 * ========================================================================== */

typedef struct mg_header {
    int32_t refcount;
    int32_t type_id;
} mg_header;

typedef struct mg_string {
    mg_header header;
    int32_t length;
    int32_t capacity;
    char* data;
} mg_string;

typedef struct mg_array {
    mg_header header;
    int32_t length;
    int32_t capacity;
    int32_t elem_size;
    void* data;
} mg_array;

typedef struct mg_option {
    mg_header header;
    uint8_t has_value;
    int32_t value_size;
    void* value;
} mg_option;

typedef struct mg_map mg_map;

/* ==========================================================================
 * Memory Management
 * ========================================================================== */

void mg_runtime_init(void);
void mg_runtime_shutdown(void);
void* mg_alloc_simple(int64_t size);
void mg_free_simple(void* ptr);
void mg_incref(void* ptr);
void mg_decref(void* ptr);

/* ==========================================================================
 * String Operations
 * ========================================================================== */

mg_string* mg_string_create(const char* str);
mg_string* mg_string_create_len(const char* str, int32_t len);
void mg_string_destroy(mg_string* s);
mg_string* mg_string_concat(mg_string* a, mg_string* b);
int32_t mg_string_length(mg_string* s);
int32_t mg_string_compare(mg_string* a, mg_string* b);
bool mg_string_equals(mg_string* a, mg_string* b);
mg_string* mg_string_substring(mg_string* s, int32_t start, int32_t length);
int32_t mg_string_index_of(mg_string* s, mg_string* substr);
bool mg_string_starts_with(mg_string* s, mg_string* prefix);
bool mg_string_ends_with(mg_string* s, mg_string* suffix);
bool mg_string_contains(mg_string* s, mg_string* substr);
mg_string* mg_string_trim(mg_string* s);
mg_string* mg_string_to_lower(mg_string* s);
mg_string* mg_string_to_upper(mg_string* s);
mg_string* mg_string_replace(mg_string* s, mg_string* from, mg_string* to);
mg_string* mg_string_repeat(mg_string* s, int32_t count);
char mg_string_char_at(mg_string* s, int32_t index);
mg_array* mg_string_split(mg_string* s, char delim);
mg_string* mg_string_join(mg_array* arr, mg_string* sep);

/* ==========================================================================
 * Array Operations
 * ========================================================================== */

mg_array* mg_array_create(int32_t elem_size, int32_t initial_capacity);
void mg_array_destroy(mg_array* arr);
int32_t mg_array_length(mg_array* arr);
void mg_array_push(mg_array** arr_ptr, void* elem);
void* mg_array_pop(mg_array* arr);
void* mg_array_get(mg_array* arr, int32_t index);
void mg_array_set(mg_array* arr, int32_t index, void* elem);
void mg_array_clear(mg_array* arr);
void mg_array_reverse(mg_array* arr);

/* ==========================================================================
 * Option Operations
 * ========================================================================== */

mg_option* mg_option_some(void* value, int32_t value_size);
mg_option* mg_option_none(void);
void mg_option_destroy(mg_option* opt);
bool mg_option_is_some(mg_option* opt);
bool mg_option_is_none(mg_option* opt);
void* mg_option_unwrap(mg_option* opt);
void* mg_option_unwrap_or(mg_option* opt, void* default_value);

/* ==========================================================================
 * I/O Operations
 * ========================================================================== */

void mg_print(const char* str);
void mg_println(const char* str);
void mg_print_int(int32_t val);
void mg_println_int(int32_t val);
void mg_print_float(double val);
void mg_println_float(double val);
void mg_print_bool(bool val);
void mg_println_bool(bool val);
void mg_eprint(const char* str);
void mg_eprintln(const char* str);
mg_string* mg_readLine(void);
char mg_readChar(void);

/* ==========================================================================
 * Type Conversions
 * ========================================================================== */

mg_string* mg_int_to_string(int32_t val);
mg_string* mg_float_to_string(double val);
mg_string* mg_bool_to_string(bool val);
mg_string* mg_char_to_string(char c);
int32_t mg_string_to_int(mg_string* s);
double mg_string_to_float(mg_string* s);
bool mg_string_to_bool(mg_string* s);
mg_option* mg_parse_int(mg_string* s);
mg_option* mg_parse_float(mg_string* s);

/* ==========================================================================
 * Math Functions
 * ========================================================================== */

int32_t mg_abs_int(int32_t x);
double mg_abs_float(double x);
double mg_sqrt(double x);
double mg_cbrt(double x);
double mg_pow(double base, double exp);
double mg_exp(double x);
double mg_log(double x);
double mg_log10(double x);
double mg_log2(double x);
double mg_sin(double x);
double mg_cos(double x);
double mg_tan(double x);
double mg_asin(double x);
double mg_acos(double x);
double mg_atan(double x);
double mg_atan2(double y, double x);
double mg_floor(double x);
double mg_ceil(double x);
double mg_round(double x);
double mg_fmod(double x, double y);
int32_t mg_min_int(int32_t a, int32_t b);
int32_t mg_max_int(int32_t a, int32_t b);
double mg_min_float(double a, double b);
double mg_max_float(double a, double b);
int32_t mg_clamp_int(int32_t val, int32_t low, int32_t high);
double mg_clamp_float(double val, double low, double high);

/* ==========================================================================
 * Random Number Generation
 * ========================================================================== */

void mg_random_init(void);
int32_t mg_random_int(int32_t min, int32_t max);
double mg_random_float(void);
double mg_random_float_range(double min, double max);
bool mg_random_bool(void);

/* ==========================================================================
 * File System Functions
 * ========================================================================== */

bool mg_file_exists(mg_string* path);
bool mg_file_is_file(mg_string* path);
bool mg_file_is_directory(mg_string* path);
mg_option* mg_file_read(mg_string* path);
bool mg_file_write(mg_string* path, mg_string* content);
bool mg_file_append(mg_string* path, mg_string* content);
bool mg_file_delete(mg_string* path);
bool mg_dir_create(mg_string* path);
bool mg_dir_delete(mg_string* path);
int64_t mg_file_size(mg_string* path);

/* ==========================================================================
 * Time Functions
 * ========================================================================== */

int64_t mg_time_now(void);
int64_t mg_time_now_millis(void);
void mg_time_sleep(int32_t millis);
mg_string* mg_time_format(int64_t timestamp, mg_string* format);

/* ==========================================================================
 * System Functions
 * ========================================================================== */

mg_option* mg_env_get(mg_string* name);
bool mg_env_set(mg_string* name, mg_string* value);
int32_t mg_system_exec(mg_string* command);
void mg_exit(int32_t code);

/* ==========================================================================
 * Map Functions
 * ========================================================================== */

mg_map* mg_map_create(int32_t value_size);
void mg_map_destroy(mg_map* map);
void mg_map_set(mg_map* map, mg_string* key, void* value);
void* mg_map_get(mg_map* map, mg_string* key);
bool mg_map_has(mg_map* map, mg_string* key);
bool mg_map_remove(mg_map* map, mg_string* key);
int32_t mg_map_size(mg_map* map);
void mg_map_clear(mg_map* map);

/* ==========================================================================
 * Panic/Assert
 * ========================================================================== */

void mg_panic(const char* message);
void mg_assert(bool condition, const char* message);

#ifdef __cplusplus
}
#endif

#endif /* MG_RUNTIME_H */
