.text
.globl _start

_start:

    /*movq $0, %rdi
    call sbrk
    movq %rax, %r12
    addq $8, %rax
    movq %rax, %rdi
    call sbrk

    movq $42, (%r12)

    movq $42, %r12
    movq $42, %r12
    movq $42, %r12
    movq $42, %r12
    movq $42, %r12
    movq $42, %r12

    movabs $LC0, %rdi
    movq %rax, %rsi
    clr %rax       # needed for printf
    call printf*/

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

.section .data
LC0:
    .ascii "Stack pointer: %p\n\0"
