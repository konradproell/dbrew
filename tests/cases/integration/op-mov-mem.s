//!args=--nobytes
    .intel_syntax noprefix
    .text
    .globl  f1
    .type   f1, @function
f1:
    mov rbx, 0x1234567
    mov [wdata], rbx
    // TODO: add test with movabs + 64bit immediate
    // uncomment if accessing read-only mem range is handled
    // mov rax, [rdata+8]
    mov rax, [wdata]
    ret

