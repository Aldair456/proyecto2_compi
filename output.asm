section .data
    fmt_int: db "%d", 10, 0
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
    mov eax, 100
    mov [rbp - 4], eax
    mov eax, 50
    mov [rbp - 8], eax
    mov eax, 25
    mov [rbp - 12], eax
    mov eax, 10
    mov [rbp - 16], eax
    mov eax, 5
    mov [rbp - 20], eax
    mov eax, [rbp - 20]
    movsx rax, eax
    push rax
    mov eax, [rbp - 16]
    movsx rax, eax
    push rax
    mov eax, [rbp - 12]
    movsx rax, eax
    push rax
    mov eax, [rbp - 8]
    movsx rax, eax
    push rax
    mov eax, [rbp - 4]
    movsx rax, eax
    pop rbx
    add rax, rbx
    pop rbx
    add rax, rbx
    pop rbx
    add rax, rbx
    pop rbx
    add rax, rbx
    mov [rbp - 24], eax
    mov eax, 2
    push rax
    mov eax, [rbp - 4]
    movsx rax, eax
    pop rbx
    imul rax, rbx
    mov [rbp - 28], eax
    mov eax, [rbp - 20]
    movsx rax, eax
    push rax
    mov eax, [rbp - 4]
    movsx rax, eax
    pop rbx
    xor rdx, rdx
    idiv rbx
    mov [rbp - 32], eax
    mov eax, [rbp - 8]
    movsx rax, eax
    push rax
    mov eax, [rbp - 4]
    movsx rax, eax
    pop rbx
    sub rax, rbx
    mov [rbp - 36], eax
    mov eax, [rbp - 28]
    movsx rax, eax
    push rax
    mov eax, [rbp - 24]
    movsx rax, eax
    pop rbx
    add rax, rbx
    mov [rbp - 40], eax
    mov eax, [rbp - 36]
    movsx rax, eax
    push rax
    mov eax, [rbp - 32]
    movsx rax, eax
    pop rbx
    imul rax, rbx
    mov [rbp - 44], eax
    mov eax, [rbp - 44]
    movsx rax, eax
    push rax
    mov eax, [rbp - 40]
    movsx rax, eax
    pop rbx
    sub rax, rbx
    mov [rbp - 48], eax
    mov eax, 0
    mov [rbp - 52], eax
    mov eax, 0
    mov [rbp - 56], eax
for_start_4:
    mov eax, 10
    push rax
    mov eax, [rbp - 56]
    movsx rax, eax
    pop rbx
    cmp rax, rbx
    setl al
    movzx eax, al
    test rax, rax
    jz for_end_5
    mov eax, [rbp - 56]
    movsx rax, eax
    push rax
    mov eax, [rbp - 52]
    movsx rax, eax
    pop rbx
    add rax, rbx
    mov [rbp - 52], eax
    mov eax, 1
    push rax
    mov eax, [rbp - 56]
    movsx rax, eax
    pop rbx
    add rax, rbx
    mov [rbp - 56], eax
    jmp for_start_4
for_end_5:
    mov eax, 1
    mov [rbp - 60], eax
    mov eax, 1
    mov [rbp - 64], eax
for_start_6:
    mov eax, 5
    push rax
    mov eax, [rbp - 64]
    movsx rax, eax
    pop rbx
    cmp rax, rbx
    setle al
    movzx eax, al
    test rax, rax
    jz for_end_7
    mov eax, [rbp - 64]
    movsx rax, eax
    push rax
    mov eax, [rbp - 60]
    movsx rax, eax
    pop rbx
    imul rax, rbx
    mov [rbp - 60], eax
    mov eax, 1
    push rax
    mov eax, [rbp - 64]
    movsx rax, eax
    pop rbx
    add rax, rbx
    mov [rbp - 64], eax
    jmp for_start_6
for_end_7:
    mov eax, [rbp - 60]
    movsx rax, eax
    push rax
    mov eax, [rbp - 52]
    movsx rax, eax
    push rax
    mov eax, [rbp - 48]
    movsx rax, eax
    pop rbx
    add rax, rbx
    pop rbx
    add rax, rbx
    mov [rbp - 68], eax
    mov eax, [rbp - 68]
    movsx rax, eax
    mov rsp, rbp
    pop rbp
    ret

