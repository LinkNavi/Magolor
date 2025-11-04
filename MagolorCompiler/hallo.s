    .text
    .intel_syntax noprefix
    .globl Calculator.MathOperations.factorial
Calculator.MathOperations.factorial:
    push rbp
    mov rbp, rsp
.L_Calculator_MathOperations_factorial_0:
    cmp rdi, 1
    setle al
    movzx rax, al
    test rax, rax
    jnz .L_Calculator_MathOperations_factorial_1
    jmp .L_Calculator_MathOperations_factorial_2
.L_Calculator_MathOperations_factorial_1:
    mov rax, 1
    jmp .L_Calculator_MathOperations_factorial_epilogue
.L_Calculator_MathOperations_factorial_2:
    jmp .L_Calculator_MathOperations_factorial_3
.L_Calculator_MathOperations_factorial_3:
    mov rbx, rdi
    sub rbx, 1
    mov rdi, rbx
    call Calculator.MathOperations.factorial
    mov rcx, rax
    mov rdx, rdi
    imul rdx, rcx
    mov rax, rdx
    jmp .L_Calculator_MathOperations_factorial_epilogue
.L_Calculator_MathOperations_factorial_epilogue:
    mov rsp, rbp
    pop rbp
    ret

    .globl Calculator.MathOperations.add
Calculator.MathOperations.add:
    push rbp
    mov rbp, rsp
.L_Calculator_MathOperations_add_0:
    mov rbx, rax
    add rbx, 1
    mov qword ptr [Calculator.MathOperations.callCount], rbx
    mov rcx, rdi
    add rcx, rsi
    mov rax, rcx
    jmp .L_Calculator_MathOperations_add_epilogue
.L_Calculator_MathOperations_add_epilogue:
    mov rsp, rbp
    pop rbp
    ret

    .globl Calculator.MathOperations.getCallCount
Calculator.MathOperations.getCallCount:
    push rbp
    mov rbp, rsp
.L_Calculator_MathOperations_getCallCount_0:
    mov rax, qword ptr [Calculator.MathOperations.callCount]
    mov rax, rax
    jmp .L_Calculator_MathOperations_getCallCount_epilogue
.L_Calculator_MathOperations_getCallCount_epilogue:
    mov rsp, rbp
    pop rbp
    ret

    .globl Calculator.MathOperations.divide
Calculator.MathOperations.divide:
    push rbp
    mov rbp, rsp
.L_Calculator_MathOperations_divide_0:
    cmp rsi, 0
    sete al
    movzx rax, al
    test rax, rax
    jnz .L_Calculator_MathOperations_divide_1
    jmp .L_Calculator_MathOperations_divide_2
.L_Calculator_MathOperations_divide_1:
    lea rdi, [__str_0]
    call console_print
    mov rbx, rax
    mov rax, 0
    jmp .L_Calculator_MathOperations_divide_epilogue
.L_Calculator_MathOperations_divide_2:
    jmp .L_Calculator_MathOperations_divide_3
.L_Calculator_MathOperations_divide_3:
    mov rax, rdi
    cqo
    idiv rsi
    mov rcx, rax
    mov rax, rcx
    jmp .L_Calculator_MathOperations_divide_epilogue
.L_Calculator_MathOperations_divide_epilogue:
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
    lea rdi, [__str_2]
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
    lea rdi, [__str_3]
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
    lea rdi, [__str_4]
    mov rsi, r12
    call string_concat_int
    mov r13, rax
    mov rdi, r13
    call console_print
    mov r14, rax
    mov qword ptr [rbp - 32], 0
    mov qword ptr [rbp - 40], 0
    mov r15, 0
    lea rdi, [__str_5]
    mov rsi, r15
    call string_concat_int
    mov rax, rax
    mov rdi, qword ptr [rbp - 8]
    call console_print
    mov rax, rax
    mov qword ptr [rbp - 48], 0
    jmp .L_main_1
.L_main_1:
    mov rax, qword ptr [rbp - 24]
    cmp rax, 3
    setl al
    movzx rax, al
    test rax, rax
    jnz .L_main_2
    jmp .L_main_4
.L_main_2:
    mov rax, qword ptr [rbp - 48]
    lea rdi, [__str_6]
    mov rsi, qword ptr [rbp - 40]
    call string_concat_int
    mov rax, rax
    mov rdi, qword ptr [rbp - 48]
    call console_print
    mov rax, rax
    jmp .L_main_3
.L_main_3:
    mov rax, qword ptr [rbp - 64]
    add rax, 1
    mov rax, qword ptr [rbp - 72]
    mov qword ptr [rbp - 48], rax
    jmp .L_main_1
.L_main_4:
    mov qword ptr [rbp - 56], 3
.L_main_epilogue:
    mov rsp, rbp
    pop rbp
    ret


    .data
__str_8:
    .asciz "Tuesday"
__str_9:
    .asciz "Wednesday"
__str_0:
    .asciz "Error: Division by zero"
__str_4:
    .asciz "5! = "
__str_7:
    .asciz "Monday"
__str_11:
    .asciz "Total math operations: "
__str_3:
    .asciz "10 / 2 = "
__str_2:
    .asciz "5 + 3 = "
__str_5:
    .asciz "Sum of array: "
__str_6:
    .asciz "Loop iteration: "
__str_10:
    .asciz "Other day"
__str_1:
    .asciz "=== Calculator Demo ==="
Calculator.MathOperations.callCount:
    .quad 0
