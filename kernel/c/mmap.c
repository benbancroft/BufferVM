//
// Created by ben on 27/12/16.
//

#include "../../libc/stdlib.h"

void *syscall_mmap(void *addr, size_t length, int prot, int flags, int fd, uint64_t offset) {

    //should anonymous mappings be done like sbrk?

    //check for map_fixed
    //if not, largely ignore the hint
    //if hint is null, ignore completely
    //else, force it past the end of the heap region for user (MAX_HEAP anyone?)
    //should force any hint to be bigger than brk (unless MAP_FIXED)

    return NULL;
}

