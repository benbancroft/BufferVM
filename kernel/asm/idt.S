#include "../h/kernel_as.h"

.section .text
.global idt_load
.global idt_null_handler
.global idt_gpf_handler
.global idt_debug_handler
idt_load:
    cli
    lidt (%rdi)
    ret
idt_null_handler:
    iretq
idt_debug_handler:
    cli
    /*movq $LC2, %rdi
    movq (%rsp), %rsi
    clr %rax       # needed for printf
    call printf*/

    pushq %rax

    movl step_counter, %eax
    testl %eax, %eax
    je .idt_debug_exit

.idt_debug_continue:
    popq %rax
    subl $0x1, step_counter
    iretq
.idt_debug_exit:
    movq 8(%rsp), %rax
    pushq %rdi
    movq idt_stack, %rdi
    movq %rax, (%rdi)
    popq %rdi
    popq %rax
    movq idt_stack, %rsp
    movq $(USER_CS | 3), 8(%rsp)

    iretq

    /*movq $LC3, %rdi
    movq (%rsp), %rsi
    movq 8(%rsp), %rdx
    movq 16(%rsp), %rcx
    movq 24(%rsp), %r8
    movq 32(%rsp), %r9
    clr %rax       # needed for printf
    call printf*/

    movq $0, %rax
    hlt
idt_gpf_handler:
    cli
    movq $LC1, %rdi
    movq 8(%rsp), %rsi
    movl (%rsp), %edx
    clr %rax       # needed for printf
    call printf
    call host_exit

    .section .data
    LC0:
        .ascii "Unhandled interrupt at PC: %p %x\n\0"
    LC1:
        .ascii "GPF interrupt at PC: %p %x\n\0"
    LC2:
        .ascii "Debug exception: %p\n\0"
    LC3:
        .ascii "iretq: %p %p %p %p %p\n\0"


