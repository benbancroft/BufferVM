//
// Created by ben on 18/10/16.
//

#include "../../common/syscall_as.h"

#ifndef PROJECT_SYSCALL_H
#define PROJECT_SYSCALL_H

void *syscall_mmap(void *addr, size_t length, int prot, int flags, int fd, uint64_t offset);

void syscall_setup();
void syscall_init();

void syscall_null_handler();

#endif //PROJECT_SYSCALL_H
