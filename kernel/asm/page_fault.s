.global idt_page_fault_handler
.type idt_page_fault_handler, @function
idt_page_fault_handler:
    cli
    movq $LC0, %rdi
    movq %cr2, %rsi
    movq 8(%rsp), %rdx
    movq (%rsp), %rcx
    clr %rax       # needed for printf
    call printf

    call host_exit
.section .data
LC0:
        .ascii "Page fault - VA: %p PC: %p Error: %d\n\0"
