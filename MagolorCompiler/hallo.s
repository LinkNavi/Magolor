    .text
    .intel_syntax noprefix

    .globl MathOperations.add
MathOperations.add:
    push rbp
    mov rbp, rsp
.L_MathOperations_add_0:
    mov rax, rdi
    add rax, rsi
    mov rax, rax
    jmp .L_MathOperations_add_epilogue
.L_MathOperations_add_epilogue:
    mov rsp, rbp
    pop rbp
    ret

    .globl MathOperations.factorial
MathOperations.factorial:
    push rbp
    mov rbp, rsp
.L_MathOperations_factorial_0:
    cmp rdi, 1
    setle al
    movzx rax, al
    test rax, rax
    jnz .L_MathOperations_factorial_1
    jmp .L_MathOperations_factorial_2
.L_MathOperations_factorial_1:
    mov rax, 1
    jmp .L_MathOperations_factorial_epilogue
.L_MathOperations_factorial_2:
.L_MathOperations_factorial_epilogue:
    mov rsp, rbp
    pop rbp
    ret

    .globl MathOperations.divide
MathOperations.divide:
    push rbp
    mov rbp, rsp
.L_MathOperations_divide_0:
    cmp rsi, 0
    sete al
    movzx rax, al
    test rax, rax
    jnz .L_MathOperations_divide_1
    jmp .L_MathOperations_divide_2
.L_MathOperations_divide_1:
    lea rdi, [__str_0]
    call console_print
    mov rbx, rax
    mov rax, 0
    jmp .L_MathOperations_divide_epilogue
.L_MathOperations_divide_2:
.L_MathOperations_divide_epilogue:
    mov rsp, rbp
    pop rbp
    ret

    .globl MathOperations.getCallCount
MathOperations.getCallCount:
    push rbp
    mov rbp, rsp
.L_MathOperations_getCallCount_0:
    mov rax, [MathOperations.callCount]
    jmp .L_MathOperations_getCallCount_epilogue
.L_MathOperations_getCallCount_epilogue:
    mov rsp, rbp
    pop rbp
    ret

    .globl main
main:
    push rbp
    mov rbp, rsp
    sub rsp, 64
.L_main_0:
    lea rdi, [__str_1]
    call console_print
    mov rax, rax
    mov rdi, 5
    mov rsi, 3
    call Calculator.MathOperations.add
    mov rbx, rax
    mov qword ptr [rbp - 8], rbx
    mov rcx, rbx
    mov rdi, [__str_2]
    mov rsi, rcx
    call string_concat_int
    mov rdx, rax
    mov rdi, rdx
    call console_print
    mov rsi, rax
    mov rdi, 10
    mov rsi, 2
    call Calculator.MathOperations.divide
    mov rdi, rax
    mov qword ptr [rbp - 16], rdi
    mov r8, rdi
    mov rdi, [__str_3]
    mov rsi, r8
    call string_concat_int
    mov r9, rax
    mov rdi, r9
    call console_print
    mov r10, rax
    mov rdi, 5
    call Calculator.MathOperations.factorial
    mov r11, rax
    mov qword ptr [rbp - 24], r11
    mov r12, r11
    mov rdi, [__str_4]
    mov rsi, r12
    call string_concat_int
    mov r13, rax
    mov rdi, r13
    call console_print
    mov r14, rax
    mov qword ptr [rbp - 32], 0
    mov qword ptr [rbp - 40], 0
    mov r15, 0
    mov rdi, [__str_5]
    mov rsi, r15
    call string_concat_int
    mov qword ptr [rbp - 8], rax
    mov rdi, qword ptr [rbp - 8]
    call console_print
    mov qword ptr [rbp - 16], rax
    mov qword ptr [rbp - 48], 0
    jmp .L_main_1
.L_main_1:
    mov rax, qword ptr [rbp - 24]
    cmp rax, 3
    setl al
    movzx qword ptr [rbp - 32], al
    mov rax, qword ptr [rbp - 32]
    test rax, rax
    jnz .L_main_2
    jmp .L_main_4
.L_main_2:
    mov qword ptr [rbp - 40], qword ptr [rbp - 48]
    mov rdi, [__str_6]
    mov rsi, qword ptr [rbp - 40]
    call string_concat_int
    mov qword ptr [rbp - 48], rax
    mov rdi, qword ptr [rbp - 48]
    call console_print
    mov qword ptr [rbp - 56], rax
    jmp .L_main_3
.L_main_3:
    jmp .L_main_1
.L_main_4:
    mov qword ptr [rbp - 56], 3
    call Calculator.MathOperations.getCallCount
    mov qword ptr [rbp - 64], rax
    mov qword ptr [rbp - 64], qword ptr [rbp - 64]
    mov rax, qword ptr [rbp - 64]
    mov qword ptr [rbp - 72], rax
    mov rdi, [__str_7]
    mov rsi, qword ptr [rbp - 72]
    call string_concat_int
    mov qword ptr [rbp - 80], rax
    mov rdi, qword ptr [rbp - 80]
    call console_print
    mov qword ptr [rbp - 88], rax
    jmp .L_main_epilogue
.L_main_epilogue:
    mov rsp, rbp
    pop rbp
    ret


    .data
__str_7:
    .asciz "Total math operations: "
__str_1:
    .asciz "=== Calculator Demo ==="
__str_6:
    .asciz "Loop iteration: "
__str_5:
    .asciz "Sum of array: "
__str_2:
    .asciz "5 + 3 = "
__str_0:
    .asciz "Error: Division by zero"
__str_4:
    .asciz "5! = "
__str_3:
    .asciz "10 / 2 = "
MathOperations.callCount:
    .quad 0
