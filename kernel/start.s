        .global _start
        .code64
_start:
    movq $0xc0000000, %rsp
	call kernel_main
	movq $0xDEADBEEF, %rax
	movq $42, (%rax)
	call host_exit
