//
// Created by ben on 14/10/16.
//

#ifndef KERNEL_HOST_H
#define KERNEL_HOST_H

#include "../../libc/stdlib.h"

void host_exit();

int host_write(uint32_t fd, const char* buf, size_t count);

#endif //KERNEL_HOST_H
