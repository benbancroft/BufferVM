//
// Created by ben on 17/10/16.
//

#include "../../libc/stdlib.h"
#include "../h/kernel.h"
#include "../h/syscall.h"
#include "../h/host.h"

/* Syscall table and parameter info */
void *syscall_table[SYSCALL_MAX];

/* Register a syscall */
void syscall_register(int num, void *fn)
{
    if (num >= 0 && num < SYSCALL_MAX)
    {
        syscall_table[num] = fn;
    }
}

/* Unregister a syscall */
void syscall_unregister(int num)
{
    if (num >= 0 && num < SYSCALL_MAX)
    {
        syscall_table[num] = NULL;
    }
}

void syscall_write(uint32_t fd, const char* buf, size_t count){
    host_write(fd, buf, count);
}

void syscall_init()
{
    syscall_setup();

    size_t i;
    for (i = 0; i < SYSCALL_MAX; ++i) {
        syscall_register(i, (uintptr_t) &syscall_null_handler);
    }

    syscall_register(1, (uintptr_t) &syscall_write);
}