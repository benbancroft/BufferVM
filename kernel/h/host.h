//
// Created by ben on 14/10/16.
//

#ifndef KERNEL_HOST_H
#define KERNEL_HOST_H

#include "../../libc/stdlib.h"

void host_exit();

int host_write(uint32_t fd, const char* buf, size_t count);

int host_read(uint32_t fd, const char* buf, size_t count);

int host_open(const char *filename, int32_t flags, uint16_t mode);

int host_close(uint32_t fd);

int host_regs();

int host_print_var(int32_t var);

#endif //KERNEL_HOST_H
