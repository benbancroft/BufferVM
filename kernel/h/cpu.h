//
// Created by ben on 20/10/16.
//

#include "../../libc/stdlib.h"

#ifndef PROJECT_CPU_H
#define PROJECT_CPU_H

typedef struct cpu_t {
    uint64_t rtmp_r15;
    uint64_t rtmp_rsp;
    uint64_t rtmp_dsp;
    uint64_t rtmp_dcntr;
    uint64_t in_kernel;
} __attribute__
    ((packed))cpu_t;

extern cpu_t cpu;

extern void cpu_init(void);

#endif //PROJECT_CPU_H
