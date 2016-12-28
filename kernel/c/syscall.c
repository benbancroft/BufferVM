//
// Created by ben on 17/10/16.
//

#include "../../libc/stdlib.h"
#include "../h/kernel.h"
#include "../h/syscall.h"
#include "../h/host.h"
#include "../../common/paging.h"
#include "../../common/version.h"

uint64_t curr_brk;

/* Syscall table and parameter info */
void *syscall_table[SYSCALL_MAX];

/* Register a syscall */
void syscall_register(int num, void *fn)
{
    if (num >= 0 && num <= SYSCALL_MAX)
    {
        syscall_table[num] = fn;
    }
}

/* Unregister a syscall */
void syscall_unregister(int num)
{
    if (num >= 0 && num <= SYSCALL_MAX)
    {
        syscall_table[num] = NULL;
    }
}

void syscall_read(uint32_t fd, const char* buf, size_t count){
    host_read(fd, buf, count);
}

void syscall_write(uint32_t fd, const char* buf, size_t count){
    host_write(fd, buf, count);
}

int syscall_open(const char *filename, int32_t flags, uint16_t mode){
    return host_open(filename, flags, mode);
}

int syscall_close(int32_t fd){
    return host_close(fd);
}

void syscall_exit(){
    host_exit();
}

uint64_t syscall_brk(uint64_t brk){
    uint64_t new_brk;

    if (brk == 0 && curr_brk == 0){
        curr_brk = user_heap_start;
        printf("starting heap at: %p\n", user_heap_start);
    }
    else {
        if (brk < curr_brk){
            return -1;
        }

        new_brk = PAGE_ALIGN(brk);

        printf("new heap at: %p\n", new_brk);

        for (uint64_t p = curr_brk; p <= new_brk; p += PAGE_SIZE){
            //Actual memory page
            map_physical_page(p, -1, PDE64_NO_EXE | PDE64_WRITEABLE/* | PDE64_USER*/, 1, 0);
            //Version page
            map_physical_page(user_version_start + p, allocate_page(NULL, true), PDE64_NO_EXE | PDE64_WRITEABLE/* | PDE64_USER*/, 1, 0);
        }

        curr_brk = new_brk;
    }

    return curr_brk;
}

void syscall_init()
{
    syscall_setup();

    size_t i;
    for (i = 0; i < SYSCALL_MAX; ++i) {
        syscall_register(i, (uintptr_t) &syscall_null_handler);
    }

    syscall_register(0, (uintptr_t) &syscall_read);
    syscall_register(1, (uintptr_t) &syscall_write);
    syscall_register(2, (uintptr_t) &syscall_open);
    syscall_register(3, (uintptr_t) &syscall_close);
    syscall_register(12, (uintptr_t) &syscall_brk);
    syscall_register(60, (uintptr_t) &syscall_exit);

    syscall_register(SYSCALL_MAX, (uintptr_t) &get_version);
    syscall_register(SYSCALL_MAX-1, (uintptr_t) &set_version);

    curr_brk = 0;
}