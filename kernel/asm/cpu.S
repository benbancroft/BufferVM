#include "../h/kernel_as.h"

    .section .text
    .global cpu_init

cpu_init:
    movw $0, %ax
    movw	%ax, %gs
    movl $(KERNEL_CS | ((USER_DS - 8) << 16)), %edx
    movabs $cpu, %rcx
    movl %ecx, %eax
    shrq $32, %rcx
    movl %ecx, %edx
    movl $(MSR_GS_BASE), %ecx
    wrmsr
    ret