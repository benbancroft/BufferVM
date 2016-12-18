#include "../h/kernel_as.h"

.section .text
.global idt_page_fault_handler
.type idt_page_fault_handler, @function
idt_page_fault_handler:
    cli

    swapgs
    movq %rcx, %gs:CPU_TMP_R15

    movq (%rsp), %rcx
    btq $2, %rcx
    jc .page_fault_user

    swapgs

    movabs $LC0, %rdi
    movq %cr2, %rsi
    movq (%rsp), %rdx
    movq (%rsp), %rcx
    movq 24(%rsp), %r8
    clr %rax       # needed for printf
    call printf

    call host_exit
.page_fault_user:
    //check if pointer same as last fault, and in single step mode - if so not version fault!
    //get pointer version - check against lookup
    //if OK, set single step mode and store current pc - clear flag on page entry
    //jump to pc

    //this is god awful - likely will rewrite this c into assembly
    pushq %rax
    pushq %rdi
    pushq %rsi
    pushq %rdx
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11

    movq %cr2, %rdi
    movq 16(%rsp), %rsi
    call check_version
    testq %rax, %rax
    je .version_error

    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rdx
    popq %rsi
    popq %rdi
    popq %rax

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
        .ascii "Page fault - VA: %p PC: %p Error: %d SP: %p\n\0"
LC1:
    .ascii "Version error\n\0"
