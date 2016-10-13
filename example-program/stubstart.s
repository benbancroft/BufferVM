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
	call main

	# exit
	movq $60, %rax
	xorq %rdi, %rdi
	syscall

read:
	push    %rbp            # create stack frame
    movq    %rsp, %rbp

    # third argument: message length.
    # second argument: pointer to message to write.
    # first argument: file handle.
    movq    $0,%rax	        # system call number (read).
    syscall                 # call kernel.

	pop     %rbp            # restore the base pointer
	# return already in rax
	ret

write:
	push    %rbp            # create stack frame
    movq    %rsp, %rbp

    # third argument: message length.
    # second argument: pointer to message to write.
    # first argument: file handle.
    movq    $1,%rax	        # system call number (sys_write).
    syscall                 # call kernel.

	pop     %rbp            # restore the base pointer
	# return already in rax
	ret

_brk:
	push    %rbp            # create stack frame
    movq    %rsp, %rbp

    # first argument: file handle (stdout).
    movq    $12,%rax	    # system call number (sys_brk).
    syscall                 # call kernel.*/

	pop     %rbp            # restore the base pointer
	ret
