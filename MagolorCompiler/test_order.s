    .text
    .intel_syntax noprefix
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

    .globl main
main:
    push rbp
    mov rbp, rsp
    sub rsp, 16
.L_main_0:
    lea rdi, [__str_0]
    call console_print
    mov rax, rax
    mov rdi, 5
    mov rsi, 3
    call Calculator.MathOperations.add
    mov rbx, rax
    mov qword ptr [rbp - 8], rbx
    mov rcx, rbx
    lea rdi, [__str_1]
    mov rsi, rcx
    call string_concat_int
    mov rdx, rax
    mov rdi, rdx
    call console_print
    mov rsi, rax
    lea rdi, [__str_2]
    call console_print
    mov rdi, rax
    mov rdi, 5
    call Calculator.MathOperations.factorial
    mov r8, rax
    mov qword ptr [rbp - 16], r8
    lea rdi, [__str_3]
    call console_print
    mov r9, rax
    mov r10, qword ptr [rbp - 16]
    lea rdi, [__str_4]
    mov rsi, r10
    call string_concat_int
    mov r11, rax
    mov rdi, r11
    call console_print
    mov r12, rax
    lea rdi, [__str_5]
    call console_print
    mov r13, rax
    jmp .L_main_epilogue
.L_main_epilogue:
    mov rsp, rbp
    pop rbp
    ret


    .data
__str_0:
    .asciz "=== Calculator Demo ==="
__str_1:
    .asciz "5 + 3 = "
__str_4:
    .asciz "5! = "
__str_3:
    .asciz "Factorial computed"
__str_2:
    .asciz "About to call factorial..."
__str_5:
    .asciz "Done!"
Calculator.MathOperations.callCount:
    .quad 0
