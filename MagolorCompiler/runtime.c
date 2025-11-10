// runtime.c - Magolor Runtime Library (FIXED)
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

// ============================================
// Global Variables (for static class fields)
// ============================================

// The compiler might generate references to these
// Add any global variables your program needs here

// ============================================
// Entry Point
// ============================================

// Your compiled code will have functions, we just won't call main
// Instead, we'll link everything and you can call specific functions
// Or we can provide a stub main if your code doesn't have one

// If your code HAS a main function, DON'T include this:
/*
int main(int argc, char** argv) {
    printf("Magolor Runtime Ready\n");
    return 0;
}
*/