.intel_syntax noprefix
    .text
    .globl  test
    .type   test, @function
test:
    push rbp
    mov rbp, rsp
    xor eax, eax

1:
    add rax, rdi
    dec rdi
    cmp rdi, 0
    jne 1b

    leave
    ret

    .align 8
    .globl testCase
testCase:
    .quad 3
    .quad test
    .quad 2

