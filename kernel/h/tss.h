//
// Created by ben on 17/10/16.
//

#ifndef PROJECT_TSS_H
#define PROJECT_TSS_H

#include "../../libc/stdlib.h"

typedef struct tss_entry {
    uint32_t prev_tss;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserve2;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserve3;
    uint16_t reserve4;
    uint16_t iomap_base;
} __attribute__ ((packed))
        tss_entry_t;

tss_entry_t* tss;

extern void tss_load(void);

void tss_init(uintptr_t kernel_stack);

#endif //PROJECT_TSS_H
