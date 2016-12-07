#include "../h/kernel_as.h"

.global idt_page_fault_handler
.type idt_page_fault_handler, @function
idt_page_fault_handler:
    cli

    pushq %rcx

    movq 8(%rsp), %rcx
    btq $2, %rcx
    jc .page_fault_user

    popq %rcx

    movq $LC0, %rdi
    movq %cr2, %rsi
    movq 8(%rsp), %rdx
    movq (%rsp), %rcx
    clr %rax       # needed for printf
    call printf

    call host_exit
.page_fault_user:
    //check if pointer same as last fault, and in single step mode - if so not version fault!
    //get pointer version - check against lookup
    //if OK, set single step mode and store current pc - clear flag on page entry
    //jump to pc

    //clone return address on stack for retq
    //we replace error value currently on stack
    movq 16(%rsp), %rcx
    movq %rcx, 8(%rsp)

    //restore register
    popq %rcx

    #TODO - change to movl?
    movq $1, step_counter
    movq %rsp, idt_stack
    addq $8, (idt_stack)

    /*pushq %rax
    pushq %rdi
    pushq %rsi
    movq step_counter, %rdi
    movq 24(%esp), %rsi
    call host_regs
    popq %rsi
    popq %rdi
    popq %rax*/

    pushf
    movl (%esp), %ecx
    orl $0x0100, %ecx
    movl %ecx, (%esp)
    popf

    #allows us to jump without clobbering register
    retq

.section .data
LC0:
        .ascii "Page fault - VA: %p PC: %p Error: %d\n\0"
LC2:
    .ascii "Stack pointer: %p\n\0"
