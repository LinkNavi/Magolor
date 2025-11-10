// runtime.c - Magolor Runtime Library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ============================================
// Console Functions
// ============================================

void console_print(const char* msg) {
    if (msg) {
        printf("%s\n", msg);
    }
}

void Console_print(const char* msg) {
    console_print(msg);
}

// Print functions for different types
void print_int(int n) {
    printf("%d\n", n);
}

void print_str(const char* s) {
    printf("%s\n", s ? s : "(null)");
}

void print_f32(float f) {
    printf("%f\n", f);
}

void print_f64(double f) {
    printf("%f\n", f);
}

// ============================================
// String Functions
// ============================================

int String_length(const char* s) {
    return s ? strlen(s) : 0;
}

char* String_concat(const char* a, const char* b) {
    if (!a) a = "";
    if (!b) b = "";
    size_t len = strlen(a) + strlen(b) + 1;
    char* result = malloc(len);
    strcpy(result, a);
    strcat(result, b);
    return result;
}

// ============================================
// Math Functions
// ============================================

int Math_abs(int x) {
    return abs(x);
}

double Math_sqrt(double x) {
    return sqrt(x);
}

double Math_pow(double x, double y) {
    return pow(x, y);
}

// ============================================
// Type Conversion / String Formatting
// ============================================

char* int_to_string(int n) {
    char* buf = malloc(32);
    snprintf(buf, 32, "%d", n);
    return buf;
}

char* float_to_string(double f) {
    char* buf = malloc(32);
    snprintf(buf, 32, "%g", f);
    return buf;
}

char* string_concat_int(const char* s, int n) {
    char* num_str = int_to_string(n);
    char* result = String_concat(s, num_str);
    free(num_str);
    return result;
}
