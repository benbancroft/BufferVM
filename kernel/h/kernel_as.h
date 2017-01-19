//
// Created by ben on 14/10/16.
//

#ifndef KERNEL_KERNEL_AS_H
#define KERNEL_KERNEL_AS_H

#define KERNEL_CS	0x10
#define KERNEL_DS	0x18
#define USER_CS	    0x30
#define USER_DS	    0x28

#define TSS_S	    0x40
#define CPU_S       0x50

#define CPU_TMP_R15       0x0
#define CPU_TMP_RSP       0x8
#define CPU_TMP_DSP       0x16
#define CPU_TMP_DCNTR       0x24
#define CPU_IN_KERN       0x32

/* AMD64 MSR values */
#define AMD64_MSR_EFER			0xC0000080
#define AMD64_MSR_STAR			0xC0000081
#define AMD64_MSR_LSTAR			0xC0000082
#define AMD64_MSR_CSTAR			0xC0000083
#define AMD64_MSR_SFMASK	0xC0000084

#define SINGLE_STEP_F 0x0100

#define MSR_FS_BASE		0xc0000100 /* 64bit FS base */
#define MSR_GS_BASE		0xc0000101 /* 64bit GS base */

#define pushRegs() \
    pushq %rax; \
    pushq %rdi; \
    pushq %rsi; \
    pushq %rdx; \
    pushq %r8; \
    pushq %r9; \
    pushq %r10; \
    pushq %r11;

#define popRegs() \
    popq %r11; \
    popq %r10; \
    popq %r9; \
    popq %r8; \
    popq %rdx; \
    popq %rsi; \
    popq %rdi; \
    popq %rax;

#endif //KERNEL_KERNEL_AS_H
