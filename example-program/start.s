.text
.globl _start

_start:
	call main

	# exit
	movq $60, %rax
	xorq %rdi, %rdi
	syscall
