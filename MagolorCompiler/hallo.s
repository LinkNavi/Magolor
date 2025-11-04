    .text
    .intel_syntax noprefix

    .globl Test.Calculator.subtract
Test.Calculator.subtract:
    push rbp
    mov rbp, rsp
.L_Test_Calculator_subtract_0:
    mov rbx, rax
    add rbx, 1
    mov qword ptr [Test.Calculator.operationCount], rbx
    mov rcx, rdi
    sub rcx, rsi
    mov rax, rcx
    jmp .L_Test_Calculator_subtract_epilogue
.L_Test_Calculator_subtract_epilogue:
    mov rsp, rbp
    pop rbp
    ret

    .globl Test.Calculator.getOperationCount
Test.Calculator.getOperationCount:
    push rbp
    mov rbp, rsp
.L_Test_Calculator_getOperationCount_0:
    mov rax, qword ptr [Test.Calculator.operationCount]
    mov rax, rax
    jmp .L_Test_Calculator_getOperationCount_epilogue
.L_Test_Calculator_getOperationCount_epilogue:
    mov rsp, rbp
    pop rbp
    ret

    .globl Test.Calculator.divide
Test.Calculator.divide:
    push rbp
    mov rbp, rsp
.L_Test_Calculator_divide_0:
    cmp rsi, 0
    sete al
    movzx rax, al
    test rax, rax
    jnz .L_Test_Calculator_divide_1
    jmp .L_Test_Calculator_divide_2
.L_Test_Calculator_divide_1:
    lea rdi, [__str_0]
    call console_print
    mov rbx, rax
    mov rax, 0
    jmp .L_Test_Calculator_divide_epilogue
.L_Test_Calculator_divide_2:
    jmp .L_Test_Calculator_divide_3
.L_Test_Calculator_divide_3:
    mov rdx, rcx
    add rdx, 1
    mov qword ptr [Test.Calculator.operationCount], rdx
    mov rax, rdi
    cqo
    idiv rsi
    mov rsi, rax
    mov rax, rsi
    jmp .L_Test_Calculator_divide_epilogue
.L_Test_Calculator_divide_epilogue:
    mov rsp, rbp
    pop rbp
    ret

    .globl Test.Calculator.multiply
Test.Calculator.multiply:
    push rbp
    mov rbp, rsp
.L_Test_Calculator_multiply_0:
    mov rbx, rax
    add rbx, 1
    mov qword ptr [Test.Calculator.operationCount], rbx
    mov rcx, rdi
    imul rcx, rsi
    mov rax, rcx
    jmp .L_Test_Calculator_multiply_epilogue
.L_Test_Calculator_multiply_epilogue:
    mov rsp, rbp
    pop rbp
    ret

    .globl Test.Calculator.add
Test.Calculator.add:
    push rbp
    mov rbp, rsp
.L_Test_Calculator_add_0:
    mov rbx, rax
    add rbx, 1
    mov qword ptr [Test.Calculator.operationCount], rbx
    mov rcx, rdi
    add rcx, rsi
    mov rax, rcx
    jmp .L_Test_Calculator_add_epilogue
.L_Test_Calculator_add_epilogue:
    mov rsp, rbp
    pop rbp
    ret

    .globl main
main:
    push rbp
    mov rbp, rsp
    sub rsp, 80
.L_main_0:
    lea rdi, [__str_1]
    call console_print
    mov rax, rax
    lea rdi, [__str_2]
    call console_print
    mov rbx, rax
    mov rdi, 10
    mov rsi, 5
    call Test.Calculator.add
    mov rcx, rax
    mov qword ptr [rbp - 8], rcx
    lea rdi, [__str_3]
    call console_print
    mov rdx, rax
    mov rdi, 10
    mov rsi, 5
    call Test.Calculator.subtract
    mov rsi, rax
    mov qword ptr [rbp - 16], rsi
    lea rdi, [__str_4]
    call console_print
    mov rdi, rax
    mov rdi, 10
    mov rsi, 5
    call Test.Calculator.multiply
    mov r8, rax
    mov qword ptr [rbp - 24], r8
    lea rdi, [__str_5]
    call console_print
    mov r9, rax
    mov rdi, 10
    mov rsi, 5
    call Test.Calculator.divide
    mov r10, rax
    mov qword ptr [rbp - 32], r10
    lea rdi, [__str_6]
    call console_print
    mov r11, rax
    lea rdi, [__str_7]
    call console_print
    mov r12, rax
    mov rdi, 10
    mov rsi, 0
    call Test.Calculator.divide
    mov r13, rax
    mov qword ptr [rbp - 40], r13
    lea rdi, [__str_8]
    call console_print
    mov r14, rax
    lea rdi, [__str_9]
    call console_print
    mov r15, rax
    mov qword ptr [rbp - 48], 10
    mov rax, qword ptr [rbp - 8]
    cmp rax, 5
    setg al
    movzx rax, al
    mov rax, qword ptr [rbp - 16]
    test rax, rax
    jnz .L_main_1
    jmp .L_main_2
.L_main_1:
    lea rdi, [__str_10]
    call console_print
    mov rax, rax
    jmp .L_main_3
.L_main_2:
    lea rdi, [__str_11]
    call console_print
    mov rax, rax
    jmp .L_main_3
.L_main_3:
    lea rdi, [__str_12]
    call console_print
    mov rax, rax
    mov qword ptr [rbp - 56], 0
    jmp .L_main_4
.L_main_4:
    mov rax, qword ptr [rbp - 48]
    cmp rax, 3
    setl al
    movzx rax, al
    mov rax, qword ptr [rbp - 56]
    test rax, rax
    jnz .L_main_5
    jmp .L_main_6
.L_main_5:
    lea rdi, [__str_13]
    call console_print
    mov rax, rax
    mov rax, qword ptr [rbp - 72]
    add rax, 1
    mov rax, qword ptr [rbp - 80]
    mov qword ptr [rbp - 56], rax
    jmp .L_main_4
.L_main_6:
    lea rdi, [__str_14]
    call console_print
    mov rax, rax
    mov qword ptr [rbp - 64], 3
.L_main_epilogue:
    mov rsp, rbp
    pop rbp
    ret


    .data
__str_12:
    .asciz "Test 5: While loop"
__str_2:
    .asciz "Test 1: Basic arithmetic"
__str_14:
    .asciz "Test 7: Match statement"
__str_19:
    .asciz "Test 8: Compound assignment"
__str_18:
    .asciz "Other day"
__str_4:
    .asciz "10 - 5 = "
__str_22:
    .asciz "=== Test Complete ==="
__str_16:
    .asciz "Tuesday"
__str_17:
    .asciz "Wednesday"
__str_10:
    .asciz "x is greater than 5"
__str_21:
    .asciz "After -= 3:"
__str_5:
    .asciz "10 * 5 = "
__str_7:
    .asciz "Test 2: Division by zero"
__str_8:
    .asciz "Test 3: Factorial"
__str_9:
    .asciz "Test 4: Control flow"
__str_15:
    .asciz "Monday"
__str_20:
    .asciz "After += 5:"
__str_1:
    .asciz "=== Comprehensive Test ==="
__str_13:
    .asciz "Loop iteration"
__str_0:
    .asciz "Error: Division by zero"
__str_3:
    .asciz "10 + 5 = "
__str_6:
    .asciz "10 / 5 = "
__str_23:
    .asciz "Total operations: "
__str_11:
    .asciz "x is not greater than 5"
Test.Calculator.operationCount:
    .quad 0
