.data
hello:
    .string "Hello world!\n"

.text
.globl _start
.globl read
.globl write
.globl _brk
.type read, @function
.type write, @function
.type _brk, @function

_start:

	#movl %cr0, %eax
	#orl 0x80000000, %eax
    #movl %eax, %cr0

	#movl $42, %eax
    #movl %eax, 0xDEADB000
    #hlt

	call main

	# exit
	movl $1, %eax
	xorl %ebx, %ebx
	int $0x80

read:
	push    %ebp            # create stack frame
    movl    %esp, %ebp

    movl    16(%ebp),%edx    # third argument: message length.
    movl    12(%ebp),%ecx   # second argument: pointer to message to write.
    movl    8(%ebp),%ebx	# first argument: file handle.
    movl    $3,%eax	        # system call number (read).
    int     $0x80           # call kernel.

	pop     %ebp            # restore the base pointer
	movl	%edx, %eax
	ret

write:
	push    %ebp            # create stack frame
    movl    %esp, %ebp

    movl    16(%ebp),%edx    # third argument: message length.
    movl    12(%ebp),%ecx   # second argument: pointer to message to write.
    movl    8(%ebp),%ebx	# first argument: file handle.
    movl    $4,%eax	        # system call number (sys_write).
    int     $0x80           # call kernel.

	pop     %ebp            # restore the base pointer
	movl	%edx, %eax
	ret

_brk:
	push    %ebp            # create stack frame
    movl    %esp, %ebp

    movl    8(%ebp),%ebx	# first argument: file handle (stdout).
    movl    $45,%eax	# system call number (sys_brk).
    int     $0x80           # call kernel.

	pop     %ebp            # restore the base pointer
	ret
