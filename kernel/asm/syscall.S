#include "../h/kernel.h"
#include "../h/syscall_as.h"

    .section .text
    .global syscall_entry
    .global syscall_setup
    .global syscall_null_handler
syscall_setup:
    # Set the kernel EFLAGS mask value
    movl $0x00, %eax
    movl $0x00, %edx
	movl $(AMD64_MSR_SFMASK), %ecx
	wrmsr
    # Set the kernel segment registers
    movl $(KERNEL_CS | (USER_CS << 16)), %edx
    movl $(AMD64_MSR_STAR), %ecx
    wrmsr
    # Set the kernel 64-bit entry point
    movq $syscall_entry, %rcx
    movl %ecx, %eax
    shrq $32, %rcx
    movl %ecx, %edx
    movl $(AMD64_MSR_LSTAR), %ecx
    wrmsr
    # Finally enable syscall
    movl $(AMD64_MSR_EFER), %ecx
    rdmsr
    orl $1, %eax
    wrmsr
    #return
    ret
syscall_entry:
    swapgs
    #use %r11 so we do not trash syscall args
    #we are able to trash %rcx and %r11
    movq %rsp, %r11
    movq $kernel_stack, %rsp
    movq (%rsp), %rsp
    pushq %rcx
    pushq %r11
    pushq %rbp

    #bounds check
    cmpq $0, %rax
    jl syscall_entry_bail
    cmpq $(SYSCALL_MAX), %rax
    jge syscall_entry_bail

    #swap to match C amd64 abi
    movq %r10, %rcx

    movq syscall_table(, %rax, 8), %r11
    call *%r11

    movq %rcx, %r10

syscall_entry_bail:
    popq %rbp
    popq %r11
    popq %rcx
    mov %r11, %rsp

    swapgs
    sysretq
syscall_null_handler:
    movq $LC0, %rdi
    movq %rax, %rsi
    clr %rax       # needed for printf
    call printf
    ret
.section .data
LC0:
        .ascii "Syscall: %p\n\0"
