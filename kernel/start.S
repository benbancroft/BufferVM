#include "h/kernel_as.h"

        .section .text
        .global _start
        .global switch_usermode
        .global test
_start:
    movq %rdx, %rsp
	call kernel_main

	call host_exit
test:
    movq    $1,%rax	        # system call number (write).
    syscall                 # call kernel.
	movq $0xDEADBEEF, %rax
	movq $42, (%rax)

    hlt
switch_usermode:
    push $(USER_DS | 3)
    push %rsi
    push $(0x200 | SINGLE_STEP_F) #EFLAGS (enable ints)
    push $(USER_CS | 3) #(yes that is the correct GDT entry for DPL = 3 long mode)
    push %rdi
.clear_regs:
    movw $0, %ax
    #movw %ax, %gs
    movq $0xDEADBEEF, %rax
    movq $0, %rbx
    movq $0, %rcx
    movq $0, %rdx
    movq $0, %rsi
    movq $0, %rdi
    movq $0, %rbp
    movq $0, %r8
    movq $0, %r9
    movq $0, %r10
    movq $0, %r11
    movq $0, %r12
    movq $0, %r13
    movq $0, %r14
    movq $0, %r15
    swapgs
.enter:
    iretq
