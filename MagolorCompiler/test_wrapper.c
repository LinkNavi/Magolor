#include <stdio.h>

// External declarations
extern int MathOperations_add(int a, int b);
extern int MathOperations_factorial(int n);

int main() {
    printf("Testing Magolor compiled functions:\n");
    
    int sum = MathOperations_add(5, 3);
    printf("5 + 3 = %d\n", sum);
    
    // Note: factorial is incomplete in current codegen
    // int fact = MathOperations_factorial(5);
    // printf("5! = %d\n", fact);
    
    return 0;
}
