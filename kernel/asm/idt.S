#include "../h/kernel_as.h"

.section .text
.global idt_load
.global idt_null_handler
.global idt_gpf_handler
.global idt_debug_handler
.global idt_invalid_opcode_handler
idt_load:
    cli
    lidt (%rdi)
    ret
idt_invalid_opcode_handler:
    cli
    movabs $LC4, %rdi
    movq (%rsp), %rsi
    clr %rax       # needed for printf
    call printf
    call host_exit
idt_null_handler:
    cli
    movabs $LC0, %rdi
    movq 8(%rsp), %rsi
    movl (%rsp), %edx
    clr %rax       # needed for printf
    call printf
    call host_exit
idt_debug_handler:
    #cli

    swapgs

    pushf

    cmpl $0, %gs:CPU_TMP_DCNTR
    je .idt_debug_userland

    cmpl $1, %gs:CPU_TMP_DCNTR
    je .idt_debug_exit

.idt_debug_continue:
    subl $0x1, %gs:CPU_TMP_DCNTR

    popf
    swapgs

    iretq
.idt_debug_exit:
    subl $0x1, %gs:CPU_TMP_DCNTR

    pushq %rax

    movq $(USER_DS | 3), 48(%rsp)
    movq %gs:CPU_TMP_DSP, %rax
    movq %rax, 40(%rsp)
    movw 8(%rsp), %ax
    movq $0, 32(%rsp)   #zero memory to stop extra flags being enabled
    movw %ax, 32(%rsp)
    movq $(USER_CS | 3), 24(%rsp)
    //return pointer should already be on the stack

    swapgs
    popq %rax
    addq $8, %rsp

    iretq
.idt_debug_userland:
    popf
    pushAllRegs(24(%rsp))

    movq (%rsp), %rdi
    movabs $cpu_info+CPU_REG_BASE, %rsi
    call disassemble_instr

    popAllRegs()
    swapgs

    iretq
idt_gpf_handler:
    cli
    movabs $LC1, %rdi
    movq 8(%rsp), %rsi
    movl (%rsp), %edx
    clr %rax       # needed for printf
    call printf
    movq 8(%rsp), %rdi
    movq $5, %rsi
    call disassemble_address
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
    LC4:
        .ascii "Invalid Opcode interrupt at PC: %p\n\0"
