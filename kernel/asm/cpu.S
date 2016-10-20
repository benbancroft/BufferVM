#include "../h/kernel_as.h"

    .section .text
    .global cpu_init

cpu_init:
    movw $(CPU_S), %ax
    movw	%ax, %gs
    ret