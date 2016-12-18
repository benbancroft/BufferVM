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

    pushq %rax

    swapgs

    movl %gs:CPU_TMP_DCNTR, %eax

    testl %eax, %eax
    je .idt_debug_exit

.idt_debug_continue:
    subl $0x1, %gs:CPU_TMP_DCNTR
    popq %rax

    swapgs
    iretq
.idt_debug_exit:
    movq $(USER_DS | 3), 40(%rsp)
    movq %gs:CPU_TMP_DSP, %rax
    movq %rax, 32(%rsp)
    movq $0x200, 24(%rsp)
    movq $(USER_CS | 3), 16(%rsp)
    //return pointer should already be on the stack

    popq %rax

    swapgs

    iretq
idt_gpf_handler:
    cli
    movabs $LC1, %rdi
    movq 8(%rsp), %rsi
    movl (%rsp), %edx
    clr %rax       # needed for printf
    call printf
    call host_exit

.data
    LC0:
        .ascii "Unhandled interrupt at PC: %p %x\n\0"
    LC1:
        .ascii "GPF interrupt at PC: %p %x\n\0"
    LC2:
        .ascii "Debug exception: %p %d\n\0"
    LC3:
        .ascii "iretq: %p %p %p %p %p\n\0"


