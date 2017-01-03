//
// Created by ben on 18/10/16.
//

#include "../../common/syscall_as.h"

#ifndef KERNEL_SYSCALL_H
#define KERNEL_SYSCALL_H

uint64_t syscall_mmap(uint64_t addr, size_t length, uint64_t prot, uint64_t flags, int fd, uint64_t offset);
int syscall_munmap(uint64_t addr, size_t length);

void syscall_setup();
void syscall_init();

void syscall_null_handler();

#endif //KERNEL_SYSCALL_H
