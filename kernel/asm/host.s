.text
.globl host_exit
.globl host_write
.globl host_read

host_exit:
    movq $0x0, %rax
    hlt

host_write:
    movq $0x1, %rax
    hlt
    ret

host_read:
    movq $0x2, %rax
    hlt
    ret
