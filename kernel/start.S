        .global _start
        .code64
_start:
	call kernel_main

	call jump_usermode

	call host_exit
test:
	movq $0xDEADBEEF, %rax
	movq $42, (%rax)

    hlt
jump_usermode:
    push $0x2B
    movq $0xc0000000, %rax
    push %rax
    #push $0x200 #EFLAGS (enable ints)
    pushf
    push $0x23 #(yes that is the correct GDT entry for DPL = 3 long mode)
    push $test
    iretq
