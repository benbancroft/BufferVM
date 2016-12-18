//
// Created by ben on 18/10/16.
//

#include "kernel_as.h"

#ifndef PROJECT_KERNEL_H
#define PROJECT_KERNEL_H

extern uint64_t user_heap_start;
extern uint64_t user_version_start;

void switch_usermode(void *entry);
void test();

#endif //PROJECT_KERNEL_H
