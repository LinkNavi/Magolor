    .text
    .intel_syntax noprefix
    .globl main
main:
    push rbp
    mov rbp, rsp
    sub rsp, 16
.L_main_0:
    lea rdi, [__str_0]
    call console_print
    mov rdi, 42
    call Test.Simple.returnParam
    mov rbx, rax
    mov qword ptr [rbp - 8], rbx
    lea rdi, [__str_1]
    call console_print
    mov rcx, rax
    lea rdi, [__str_2]
    call console_print
    mov rdx, rax
    mov rdi, 5
    mov rsi, 3
    call Test.Simple.addTwo
    mov rsi, rax
    mov qword ptr [rbp - 16], rsi
    lea rdi, [__str_3]
    call console_print
    mov rdi, rax
    lea rdi, [__str_4]
    call console_print
    mov r8, rax
    jmp .L_main_epilogue
.L_main_epilogue:
    mov rsp, rbp
    pop rbp
    ret

    .globl Test.Simple.returnParam
Test.Simple.returnParam:
    push rbp
    mov rbp, rsp
    sub rsp, 16
.L_Test_Simple_returnParam_0:
    mov qword ptr [rbp - 8], rdi
    mov rax, rdi
    mov rax, rax
    jmp .L_Test_Simple_returnParam_epilogue
.L_Test_Simple_returnParam_epilogue:
    mov rsp, rbp
    pop rbp
    ret

    .globl Test.Simple.addTwo
Test.Simple.addTwo:
    push rbp
    mov rbp, rsp
    sub rsp, 16
.L_Test_Simple_addTwo_0:
    mov qword ptr [rbp - 8], rdi
    mov qword ptr [rbp - 16], rsi
    mov rcx, rax
    add rcx, rbx
    mov rax, rcx
    jmp .L_Test_Simple_addTwo_epilogue
.L_Test_Simple_addTwo_epilogue:
    mov rsp, rbp
    pop rbp
    ret


    .data
__str_1:
    .asciz "Result should be 42:"
__str_4:
    .asciz "Done"
__str_3:
    .asciz "Result should be 8:"
__str_0:
    .asciz "Test 1: Return parameter"
__str_2:
    .asciz "Test 2: Add two numbers"
