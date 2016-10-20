//
// Created by ben on 09/10/16.
//

#ifndef BUFFERVM_VM_H
#define BUFFERVM_VM_H

#include <stdint.h>

#define TSS_START 0xfffbd000
#define CPU_START 0xFFFBC000

/* CR0 bits */
#define CR0_PE 1
#define CR0_MP (1 << 1)
#define CR0_EM (1 << 2)
#define CR0_TS (1 << 3)
#define CR0_ET (1 << 4)
#define CR0_NE (1 << 5)
#define CR0_WP (1 << 16)
#define CR0_AM (1 << 18)
#define CR0_NW (1 << 29)
#define CR0_CD (1 << 30)
#define CR0_PG (1 << 31)

/* CR4 bits */
#define CR4_VME 1
#define CR4_PVI (1 << 1)
#define CR4_TSD (1 << 2)
#define CR4_DE (1 << 3)
#define CR4_PSE (1 << 4)
#define CR4_PAE (1 << 5)
#define CR4_MCE (1 << 6)
#define CR4_PGE (1 << 7)
#define CR4_PCE (1 << 8)
#define CR4_OSFXSR (1 << 8)
#define CR4_OSXMMEXCPT (1 << 10)
#define CR4_UMIP (1 << 11)
#define CR4_VMXE (1 << 13)
#define CR4_SMXE (1 << 14)
#define CR4_FSGSBASE (1 << 16)
#define CR4_PCIDE (1 << 17)
#define CR4_OSXSAVE (1 << 18)
#define CR4_SMEP (1 << 20)
#define CR4_SMAP (1 << 21)

#define EFER_SCE 1
#define EFER_LME (1 << 8)
#define EFER_LMA (1 << 10)
#define EFER_NXE (1 << 11)

/* 64-bit page * entry bits */
#define PDE64_PRESENT 1
#define PDE64_RW (1 << 1)
#define PDE64_NO_EXE (1 << 63)
#define PDE64_USER (1 << 2)
#define PDE64_WRITE_MEM (1 << 3)
#define PDE64_DISABLE_CACHE (1 << 4)
#define PDE64_ACCESSED (1 << 5)
#define PDE64_DIRTY (1 << 6)
#define PDE64_PS (1 << 7)
#define PDE64_G (1 << 8)

#define PAGE_CREATE -1

#define ALIGN(x, y) (((x)+(y)-1) & ~((y)-1))

typedef struct tss_entry {
    uint32_t prev_tss;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserve2;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserve3;
    uint16_t reserve4;
    uint16_t iomap_base;
} __attribute__ ((packed))
tss_entry_t;

typedef struct cpu_t {
    uint64_t rtmp_r15;
    uint64_t rtmp_rsp;
} __attribute__
((packed))cpu_t;

struct vm_t {
    int sys_fd;
    int fd;
    char *mem;
};

struct vcpu_t {
    int fd;
    struct kvm_run *kvm_run;
};

void vm_init(struct vm_t *vm, size_t mem_size);

void vcpu_init(struct vm_t *vm, struct vcpu_t *vcpu);

static void print_seg(FILE *file, const char *name, struct kvm_segment *seg);

static void print_dt(FILE *file, const char *name, struct kvm_dtable *dt);

void kvm_show_regs(struct vm_t *vm);

static void setup_protected_mode(struct vm_t *vm, struct kvm_sregs *sregs);

/*void run_vm(struct vm_t *vm, uint64_t start_pointer);*/

int check(struct vm_t *vm, struct vcpu_t *vcpu, size_t sz);

static void setup_paged_32bit_mode(struct vm_t *vm, struct kvm_sregs *sregs);

void run(struct vm_t *vm, struct vcpu_t *vcpu, int kernel_binary_fd, int prog_binary_fd);

#endif //BUFFERVM_VM_H
