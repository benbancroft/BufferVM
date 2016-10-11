        .global bootloader, bootloader_end
        .code32
bootloader:

        // Set cr0.pg
        movl %cr0, %eax
        orl $0x80000000, %eax
        movl %eax, %cr0

        // We are now in ia32e compatibility mode. Switch to 64-bit
	    // code segment
        ljmp $(3 << 3), $1f
        .code64
1:
        movabsq $42, %rbx
        movq %rbx, 0x0000000000002000
        movq 0x0000100000002000, %rax
        hlt
bootloader_end:
