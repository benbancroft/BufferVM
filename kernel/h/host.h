//
// Created by ben on 14/10/16.
//

#ifndef KERNEL_HOST_H
#define KERNEL_HOST_H

#include "../../libc/stdlib.h"
#include "../../common/syscall.h"
#include "../../common/vma.h"

void host_exit();

int host_write(uint32_t fd, const char* buf, size_t count);

int host_read(uint32_t fd, const char* buf, size_t count);

int host_open(const char *filename, int32_t flags, uint16_t mode);

int host_close(uint32_t fd);

int host_dup(uint32_t fd);

int host_stat(const char *path, stat_t *stats);

int host_fstat(uint32_t fd, stat_t *stats);

int host_access(const char *pathname, int mode);

void *host_mmap(void *addr, size_t length, uint64_t prot, uint64_t flags, int fd, uint64_t offset);

int64_t host_lseek(int fd, int64_t offset, int whence);

ssize_t host_writev(uint64_t fd, const iovec_t *vec, uint64_t vlen, int flags);

int host_regs();

int host_print_var(int64_t var);

int host_set_vma_heap(uint64_t vma_heap_addr, vm_area_t **vma_list_start, rb_root_t *vma_rb_root);

void host_unmap_vma(vm_area_t *vma);

#endif //KERNEL_HOST_H
