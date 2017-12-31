//
// Created by ben on 17/10/16.
//

#include "../../libc/stdlib.h"
#include "../h/kernel.h"
#include "../h/syscall.h"
#include "../h/host.h"
#include "../../common/paging.h"
#include "../../common/version.h"
#include "../h/vma_heap.h"
#include "../../common/utils.h"

uint64_t curr_brk;

void syscall_read(uint32_t fd, const char* buf, size_t count){
    host_read(fd, buf, count);
}

void syscall_write(uint32_t fd, const char* buf, size_t count){
    host_write(fd, buf, count);
}

int syscall_open(const char *filename, int32_t flags, uint16_t mode){
    printf("Opening file: %s\n", filename);
    int fd = host_open(filename, flags, mode);
    printf("open fd: %d\n", fd);

    return fd;
}

int syscall_close(int32_t fd){
    printf("closing fd: %d\n", fd);
    return host_close(fd);
}

int syscall_ioctl(uint32_t fd, uint64_t request, void *argp){
    printf("syscall_ioctl fd: %d\n", fd);
    return 0;
    //return host_fstat(fd, stats);
}

int syscall_stat(const char *path, stat_t *stats){
    printf("syscall_stat path: %s\n", path);
    return host_stat(path, stats);
}

int syscall_fstat(uint32_t fd, stat_t *stats){
    printf("syscall_fstat fd: %d\n", fd);
    return host_fstat(fd, stats);
}

int syscall_access(const char *pathname, int mode){
    return host_access(pathname, mode);
}

ssize_t syscall_writev(uint64_t fd, const iovec_t *vec, uint64_t vlen, int flags){
    return host_writev(fd, vec, vlen, flags);
}

void syscall_exit_group(int status){
    printf("Exit group status: %d\n", status);
    host_exit();
}

void syscall_exit(){

    vma_print();

    host_exit();
}

uint64_t syscall_arch_prctl(int code, uint64_t addr) {
    uint64_t ret = 0;

    printf("Setup thread local storage at %p?\n", addr);

    //TODO - make this safe with interrupt handler etc
    switch (code){
        case ARCH_SET_GS:
            ret = write_msr(MSR_KERNEL_GS_BASE, addr);
            break;
        case ARCH_SET_FS:
            write_msr(MSR_FS_BASE, addr);
            break;
        case ARCH_GET_GS:
            ret = read_msr(MSR_KERNEL_GS_BASE);
            break;
        case ARCH_GET_FS:
            ret = read_msr(MSR_FS_BASE);
            break;
        default:
            ret = -EINVAL;
    }

    return ret;
}

vm_area_t *user_heap_vma;

uint64_t syscall_brk(uint64_t brk){
    uint64_t new_brk;
    size_t num_pages;
    uint64_t addr;

    if (brk == 0 && curr_brk == 0){
        curr_brk = user_heap_start;

        addr = mmap_region(NULL, curr_brk, PAGE_SIZE, VMA_GROWS, VMA_WRITE | VMA_READ | VMA_IS_VERSIONED, 0, &user_heap_vma);

        ASSERT(addr == curr_brk);

        printf("starting heap at: %p\n", user_heap_start);
    }
    else {
        if (brk < curr_brk){
            //TODO - shrinking brk
            return curr_brk;
        }

        new_brk = PAGE_ALIGN(brk);

        vm_area_t * vma = vma_find(curr_brk);

        if (vma != NULL && new_brk <= vma->start_addr)
            return curr_brk;

        vma->end_addr = new_brk;
        vma_gap_update(vma);

        printf("New brk at: %p\n", new_brk);

        curr_brk = new_brk;
    }

    return curr_brk;
}

/* Syscall table and parameter info */

void *syscall_table[SYSCALL_MAX] = {
        [0 ... SYSCALL_MAX-1] = &syscall_null_handler,
        [0] = &syscall_read,
        [1] = &syscall_write,
        [2] = &syscall_open,
        [3] = &syscall_close,
        [4] = &syscall_stat,
        [5] = &syscall_fstat,
        [9] = &syscall_mmap,
        [10] = &syscall_mprotect,
        [11] = &syscall_munmap,
        [12] = &syscall_brk,
        [16] = &syscall_ioctl,
        [20] = &syscall_writev,
        [21] = &syscall_access,
        [60] = &syscall_exit,
        [158] = &syscall_arch_prctl,
        [231] = &syscall_exit_group,
        [SYSCALL_MAX-1] = &get_version,
        [SYSCALL_MAX-2] = &set_version
};

void syscall_init()
{
    syscall_setup();

    curr_brk = 0;
}