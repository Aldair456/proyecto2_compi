section .data
    fmt_int: db "%d", 10, 0
    str_const_0: db "%d", 10, 0
    fmt_float: db "%.2f", 10, 0
    fmt_long: db "%ld", 10, 0

section .bss

section .text
    extern printf
    global main

main:
    push rbp
    mov rbp, rsp
    sub rsp, 16
    lea rax, [str_const_0]
    mov rdi, rax
    push rdi
    mov eax, 6
    mov rsi, rax
    pop rdi
    xor rax, rax
    call printf
    mov eax, 0
    mov rsp, rbp
    pop rbp
    ret

