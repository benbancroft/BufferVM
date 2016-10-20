        .section .text
        .global _start
        .global switch_usermode
        .global test
_start:
	call kernel_main

	call host_exit
test:
    movq    $1,%rax	        # system call number (write).
    syscall                 # call kernel.
	movq $0xDEADBEEF, %rax
	movq $42, (%rax)

    hlt
switch_usermode:
    push $0x2B
    movq $0xc0000000, %rax
    push %rax
    push $0x200 #EFLAGS (enable ints)
    push $0x23 #(yes that is the correct GDT entry for DPL = 3 long mode)
    push %rdi
    iretq
