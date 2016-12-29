//
// Created by ben on 28/12/16.
//

#ifndef PROJECT_UTILS_H
#define PROJECT_UTILS_H

#include "../../libc/stdlib.h"

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type, member) );})

void disassemble_address(uint64_t addr, size_t num_inst);

#endif //PROJECT_UTILS_H
