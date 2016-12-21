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
    movabs $user_stack, %rax
    movq (%rax), %rax
    push %rax
    push $0x200 #EFLAGS (enable ints)
    push $(USER_CS | 3) #(yes that is the correct GDT entry for DPL = 3 long mode)
    push %rdi
    iretq
