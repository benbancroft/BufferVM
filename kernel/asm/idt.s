.section .text
.global idt_load
.global idt_null_handler
.global idt_gpf_handler
idt_load:
    lidt (%rdi)
    ret
idt_null_handler:
    cli
    movq $LC0, %rdi
    movq 8(%rsp), %rsi
    movl (%rsp), %edx
    clr %rax       # needed for printf
    call printf
    call host_exit
idt_gpf_handler:
    cli
    movq $LC1, %rdi
    movq 8(%rsp), %rsi
    movl (%rsp), %edx
    clr %rax       # needed for printf
    call printf
    call host_exit

    .section .data
    LC0:
        .ascii "Unhandled interrupt at PC: %p %x\n\0"
    LC1:
        .ascii "GPF interrupt at PC: %p %x\n\0"


