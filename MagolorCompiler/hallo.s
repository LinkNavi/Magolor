    .text
    .intel_syntax noprefix

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

    .globl main
main:
    push rbp
    mov rbp, rsp
    sub rsp, 64
.L_main_0:
    mov qword ptr [qword ptr [rbp - 8]], 0
    mov qword ptr [qword ptr [rbp - 16]], 0
    mov qword ptr [qword ptr [rbp - 24]], 0
    mov qword ptr [qword ptr [rbp - 32]], 0
    mov qword ptr [qword ptr [rbp - 40]], 0
    mov qword ptr [qword ptr [rbp - 48]], 0
    jmp .L_main_1
.L_main_1:
    cmp qword ptr [rbp - 48], 3
    setl al
    movzx rax, al
    test rax, rax
    jnz .L_main_2
    jmp .L_main_4
.L_main_2:
    jmp .L_main_3
.L_main_3:
    jmp .L_main_1
.L_main_4:
    mov qword ptr [qword ptr [rbp - 56]], 3
    mov qword ptr [qword ptr [rbp - 64]], 0
    jmp .L_main_epilogue
.L_main_epilogue:
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


    .data
MathOperations.callCount:
    .quad 0
