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
        movabsq $42, %rax
        movq %rax, 0x400
        hlt
bootloader_end:
