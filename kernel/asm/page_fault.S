#include "../h/kernel_as.h"

#define pushRegs() \
    pushq %rax; \
    pushq %rdi; \
    pushq %rsi; \
    pushq %rdx; \
    pushq %r8; \
    pushq %r9; \
    pushq %r10; \
    pushq %r11;

#define popRegs() \
    popq %r11; \
    popq %r10; \
    popq %r9; \
    popq %r8; \
    popq %rdx; \
    popq %rsi; \
    popq %rdi; \
    popq %rax;

.section .text
.global idt_page_fault_handler
.type idt_page_fault_handler, @function
idt_page_fault_handler:
    cli

    /*movq %cr2, %rdi
    call host_print_var
    movq 8(%rsp), %rdi
    call host_print_var*/

    swapgs
    movq %rcx, %gs:CPU_TMP_R15

    movq (%rsp), %rcx
    btq $0, %rcx
    jnc .handle_page_fault
    btq $2, %rcx
    jc .page_fault_user

    //enter c page fault handler

.handle_page_fault:
    pushRegs()

    movq %cr2, %rdi
    movq 64(%rsp), %rsi
    movq 72(%rsp), %rdx
    call handle_page_fault
    testq %rax, %rax
    je .unhandled_page_fault

    popRegs()

    movq %gs:CPU_TMP_R15, %rcx
    swapgs

    addq $8, %rsp
    /*movq 32(%rsp), %rdi
    call host_print_var
    movq 8(%rsp), %rdi
    call host_print_var*/
    #movq $(USER_CS | 3), 8(%rsp)
    iretq
.unhandled_page_fault:

    popRegs()

    movabs $LC0, %rdi
    movq %cr2, %rsi
    movq 8(%rsp), %rdx
    movq (%rsp), %rcx
    movq 24(%rsp), %r8
    clr %rax       # needed for printf
    call printf
    movq 8(%rsp), %rdi
    movq $5, %rsi
    call disassemble_address
    call host_exit
.page_fault_user:
    //check if pointer same as last fault, and in single step mode - if so not version fault!
    //get pointer version - check against lookup
    //if OK, set single step mode and store current pc - clear flag on page entry
    //jump to pc

    //this is god awful - likely will rewrite this c into assembly
    pushRegs()

    movq %cr2, %rdi
    movq 72(%rsp), %rsi
    call check_version
    testq %rax, %rax
    je .version_error

    popRegs()

    //clone return address on stack for retq
    //we replace error value currently on stack
    movq 8(%rsp), %rcx
    movq %rcx, (%rsp)

    movl $1, %gs:CPU_TMP_DCNTR
    movq 32(%rsp), %rcx
    movq %rcx, %gs:CPU_TMP_DSP

    pushf
    movl (%rsp), %ecx
    orl $0x0100, %ecx
    movl %ecx, (%rsp)
    //restore register
    movq %gs:CPU_TMP_R15, %rcx
    swapgs
    popf

    #allows us to jump without clobbering register
    retq

.version_error:
    movabs $LC1, %rdi
    clr %rax       # needed for printf
    call printf

    call host_exit

.section .data
LC0:
        .ascii "Unhandled Page fault - VA: %p PC: %p Error: %d SP: %p\n\0"
LC1:
    .ascii "Version error\n\0"
