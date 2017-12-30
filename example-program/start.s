.text
.globl _start

_start:

    movq $0, %rdi
    call sbrk
    movq %rax, %r12

    addq $0x2000, %rax
    movq %rax, %r13
    movq %rax, %rdi
    call sbrk
    subq $0x1000, %r13

    /*movq %r13, %rdi
    movq $8, %rsi
    movq $5, %rdx
    call set_version*/

    #movq $0, 8(%r12)

    /*movq %r13, %rsi
    movq %r12, %rdi
    movb $42, 1000(%rsi)
    movb $42, %al
    movb %al, 1000(%rsi)
    #CMPSQ*/

    testb  $0x60, 0x314(%rax)
    #pcmpeqb  0x10(%rax), %xmm1

    /*vmovups v1,%ymm0
    vmovups v2,%ymm1
    vmovups v3,%ymm2
    #        addend + multiplicant1   * multiplicant2   = destination
    vfmaddps %ymm0,   %ymm1,            %ymm2,            %ymm3
    vmovups %ymm3, v4*/

    movabs $LC0, %rdi
    movq %r12, %rsi
    movq %r13, %rdx
    clr %rax       # needed for printf
    call printf

	#call main

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
    .ascii "heap pos: %p %p\n\0"
        #          2^-1  2^-2  2^-3   2^-4    2^-5     2^-6      2^-7       2^-8
        v1: .float 0.50, 0.25, 0.125, 0.0625, 0.03125, 0.015625, 0.0078125, 0.00390625
        v2: .float 2.0, 4.0, 8.0, 16.0, 32.0, 64.0, 128.0, 256.0
        v3: .float 512.0, 1024.0, 2048.0, 8192.0, 16384.0, 32768.0, 65536.0, 131072.0
        v4: .float 0,0,0,0,0,0,0,0
