.text
.globl host_exit
.globl host_write
.globl host_read
.globl host_open
.globl host_close
.globl host_dup
.globl host_fstat
.globl host_mmap
.globl host_lseek
.globl host_writev
.globl host_regs
.globl allocate_pages
.globl map_physical_pages
.globl unmap_physical_page
.globl is_vpage_present
.globl host_print_var

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

host_open:
    movq $0x3, %rax
    hlt
    retq

host_close:
    movq $0x4, %rax
    hlt
    ret

host_dup:
    movq $0xA, %rax
    hlt
    ret

host_fstat:
    movq $0xB, %rax
    hlt
    ret

host_mmap:
    movq $0xC, %rax
    hlt
    ret

host_lseek:
    movq $0xD, %rax
    hlt
    ret

allocate_pages:
    movq $0x5, %rax
    hlt
    ret

map_physical_pages:
    movq $0x6, %rax
    hlt
    ret

unmap_physical_page:
    movq $0xE, %rax
    hlt
    ret

host_writev:
    movq $0xF, %rax
    hlt
    ret

is_vpage_present:
    movq $0x7, %rax
    hlt
    ret

host_regs:
    movq $0x8, %rax
    hlt
    ret

host_print_var:
    movq $0x9, %rax
    hlt
    ret
