//
// Created by ben on 31/12/17.
//

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include <linux/kvm.h>
#include <unistd.h>
#include <fcntl.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <asm/errno.h>
#include <sys/uio.h>
#include <sys/utsname.h>
#include "../common/vma.h"
#include "../common/paging.h"
#include "vm.h"
#include "vma.h"

static bool exit_vm = false;

void host_exit() {
    printf("Exit hostcall\n");

    vma_print();

    exit_vm = true;

    return;
}

ssize_t host_write(uint32_t fd, const char *buf, size_t count) {
    char buffer[count];
    if (read_virtual_addr((uint64_t) buf, count, buffer)) {
        return (write(fd, buffer, count));
    } else {
        printf("Failed to write - Un-paged buffer?\n");

        return (-1);
    }
}

ssize_t host_read(uint32_t fd, const char *buf, size_t count) {
    char buffer[count];
    ssize_t ret = read(fd, buffer, count);
    if (!write_virtual_addr((uint64_t) buf, buffer, count))
        ret = -1;

    return (ret);
}

int host_open(const char *filename, int32_t flags, uint16_t mode) {
    char *buffer;
    int ret;

    if (read_virtual_cstr((uint64_t) filename, &buffer)) {
        ret = open(buffer, flags, mode);
        free(buffer);

        return (ret);
    } else {
        printf("Failed to write - Un-paged buffer?\n");

        return (-1);
    }
}

int host_openat(int dfd, const char *filename, int32_t flags, uint16_t mode) {
    char *buffer;
    int ret;

    if (read_virtual_cstr((uint64_t) filename, &buffer)) {
        ret = openat(dfd, buffer, flags, mode);
        free(buffer);

        return (ret);
    } else {
        printf("Failed to write - Un-paged buffer?\n");

        return (-1);
    }
}

int host_close(uint32_t fd) {
    return (close(fd));
}

int host_dup(uint32_t fd) {
    return (dup(fd));
}

int host_stat(const char *path, vm_stat_t *stats) {
    struct stat s_stats;
    int ret;
    char *file;
    if (read_virtual_cstr((uint64_t)path, &file)) {
        ret = stat(file, &s_stats);
        free(file);
        if (!write_virtual_addr((uint64_t)stats, (char *) &s_stats, sizeof(s_stats)))
            ret = -1;
    } else
        ret = -1;

    return (ret);
}

int host_fstat(uint32_t fd, vm_stat_t *stats) {
    int ret;
    struct stat s_stats;
    ret = fstat(fd, &s_stats);
    if (!write_virtual_addr((uint64_t) stats, (char *) &s_stats, sizeof(s_stats)))
        ret = -1;

    return (ret);
}

int host_access(const char *pathname, int mode) {
    return (access(pathname, mode));
}

void *host_mmap(void *addr, size_t length, uint64_t prot, uint64_t flags, int fd, uint64_t offset) {
    return (mmap(vm.mem + (uint64_t) addr, length, prot, flags, fd, offset) -
            (uint64_t) vm.mem);
}

int64_t host_lseek(int fd, int64_t offset, int whence) {
    return (lseek(fd, offset, whence));
}

ssize_t host_writev(int fd, const vm_iovec_t *vec, int vlen, int flags) {
    struct iovec vectors[vlen];
    struct iovec *s_vec;
    void *vecbuf;
    size_t ret = 0;

    if (vlen < 0){
        ret = -EINVAL;
    } else if (read_virtual_addr((uint64_t)vec, vlen * sizeof(struct iovec), vectors)) {
        for (int i = 0; i < vlen; i++) {
            s_vec = &vectors[i];

            vecbuf = malloc(s_vec->iov_len);
            if (!read_virtual_addr((uint64_t) s_vec->iov_base, s_vec->iov_len, vecbuf)) {
                ret = -EFAULT;
                break;
            }
            s_vec->iov_base = vecbuf;
        }
        if (!IS_ERR_VALUE(ret)) {
            ret = writev(fd, vectors, vlen);
            for (int i = 0; i < vlen; i++) {
                s_vec = &vectors[i];
                free(s_vec->iov_base);
            }
        }

    } else
        ret = -EFAULT;
    return (ret);
}

int host_set_vma_heap(uint64_t i_vma_heap_addr, vm_area_t **vma_list_start, rb_root_t *vma_rb_root) {
    uint64_t phys;

    vma_heap_addr = i_vma_heap_addr;

    if (!get_phys_addr((uint64_t)vma_list_start, &phys)) return (1);
    vma_list_start_ptr = (vm_area_t **)(vm.mem + phys);

    if (!get_phys_addr((uint64_t)vma_rb_root, &phys)) return (1);
    vma_rb_root_ptr = (rb_root_t *)(vm.mem + phys);

    return (0);
}

int host_uname(utsname_t *buf){
    int ret;
    struct utsname uname_buf;
    ret = uname(&uname_buf);
    if (!write_virtual_addr((uint64_t) buf, (char *) &uname_buf, sizeof(uname_buf)))
        ret = -1;

    return (ret);
}

typedef uint64_t (*hostcall_func_t)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

#define HOSTCALL_DEF(NAME)  \
    (hostcall_func_t)&host_ ## NAME,

#define HOSTCALL_DEF2(NAME)  \
    (hostcall_func_t)&NAME,

#define HOSTCALL_DEF_CUSTOM(NAME)  \
    (hostcall_func_t)&NAME,

hostcall_func_t hostcall_table[] = {
#include "../common/host.h"
};

int handle_host_call(struct kvm_regs *regs) {
    uint64_t call_no = regs->rax;

    if (call_no < sizeof(hostcall_table) / sizeof(hostcall_func_t)) {
        //printf("Supported hostcall %" PRIu64 "\n", call_no);
        regs->rax = (uint64_t) hostcall_table[call_no](regs->rdi, regs->rsi, regs->rdx, regs->rcx, regs->r8, regs->r9);
        if (exit_vm)
            return (1);
    } else {
        printf("Unsupported hostcall %" PRIu64 "\n", call_no);
        return (1);
    }

    return (0);
}

