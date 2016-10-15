.section .text
	.global idt_load
	.global idt_null_handler
	.type idt_load, @function
	.type idt_null_handler, @function
	idt_load:
		lidt (%rdi)
        ret
    idt_null_handler:
        iretq
