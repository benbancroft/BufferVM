.text
.globl _start

_start:
	call main

    /*movq $0xDEADBEEF, %rax
    movq $43, %rdi
	movq %rdi, (%rax)
	movq $99, %rdi
	movq (%rax), %rdi
	movq %rdi, (%rax)*/

	# exit
	movq $60, %rax
	xorq %rdi, %rdi
	syscall
