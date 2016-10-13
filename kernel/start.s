        .global _start
        .code64
_start:
    movq $0xc0000000, %rsp
	call kernel_main
	hlt
