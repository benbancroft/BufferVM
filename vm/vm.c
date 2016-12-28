//
// Created by ben on 09/10/16.
//

#include <stdio.h>
#include <linux/kvm.h>
#include <sys/ioctl.h>
#include <err.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <inttypes.h>

#include "vm.h"
#include "../common/paging.h"
#include "elf.h"
#include "gdt.h"

extern const unsigned char bootstrap[], bootstrap_end[];

void vm_init(struct vm_t *vm, size_t mem_size)
{
    int api_ver;
    struct kvm_userspace_memory_region memreg;

    vm->sys_fd = open("/dev/kvm", O_RDWR);
    if (vm->sys_fd < 0) {
        perror("open /dev/kvm");
        exit(1);
    }

    api_ver = ioctl(vm->sys_fd, KVM_GET_API_VERSION, 0);
    if (api_ver < 0) {
        perror("KVM_GET_API_VERSION");
        exit(1);
    }

    if (api_ver != KVM_API_VERSION) {
        fprintf(stderr, "Got KVM api version %d, expected %d\n",
                api_ver, KVM_API_VERSION);
        exit(1);
    }

    vm->fd = ioctl(vm->sys_fd, KVM_CREATE_VM, 0);
    if (vm->fd < 0) {
        perror("KVM_CREATE_VM");
        exit(1);
    }

    vm->mem = mmap(NULL, mem_size, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (vm->mem == MAP_FAILED) {
        perror("mmap mem");
        exit(1);
    }

    madvise(vm->mem, mem_size, MADV_MERGEABLE);

    memreg.slot = 0;
    memreg.flags = 0;
    memreg.guest_phys_addr = 0;
    memreg.memory_size = mem_size;
    memreg.userspace_addr = (unsigned long)vm->mem;
    if (ioctl(vm->fd, KVM_SET_USER_MEMORY_REGION, &memreg) < 0) {
        perror("KVM_SET_USER_MEMORY_REGION");
        exit(1);
    }
}

void vcpu_init(struct vm_t *vm, struct vcpu_t *vcpu)
{
    int vcpu_mmap_size;

    vcpu->fd = ioctl(vm->fd, KVM_CREATE_VCPU, 0);
    if (vcpu->fd < 0) {
        perror("KVM_CREATE_VCPU");
        exit(1);
    }

    if (ioctl(vm->fd, KVM_SET_TSS_ADDR, TSS_KVM) < 0) {
        perror("KVM_SET_TSS_ADDR");
        exit(1);
    }

    vcpu_mmap_size = ioctl(vm->sys_fd, KVM_GET_VCPU_MMAP_SIZE, 0);
    if (vcpu_mmap_size <= 0) {
        perror("KVM_GET_VCPU_MMAP_SIZE");
        exit(1);
    }

    vcpu->kvm_run = mmap(NULL, vcpu_mmap_size, PROT_READ | PROT_WRITE,
                         MAP_SHARED, vcpu->fd, 0);
    if (vcpu->kvm_run == MAP_FAILED) {
        perror("mmap kvm_run");
        exit(1);
    }
}

static void setup_long_mode(struct vm_t *vm, struct kvm_sregs *sregs) {
    struct kvm_segment seg = {
            .base = 0,
            .limit = 0xffffffff,
            .present = 1,
            .dpl = 0,
            .db = 1,
            .s = 1, /* Code/data */
            .l = 0,
            .g = 1, /* 4KB granularity */
    };
    uint64_t *gdt;

    build_page_tables(vm->mem);

    //Page for bootstrap
    map_physical_page(0x0000, 0x0000, PDE64_WRITEABLE, 1, vm->mem);
    //Page for gdt
    map_physical_page(0x1000, 0x1000, PDE64_WRITEABLE, 1, vm->mem);

    map_physical_page(0xDEADB000, allocate_page(vm->mem, false), PDE64_WRITEABLE, 1, vm->mem);

    sregs->cr0 |= CR0_PE; /* enter protected mode */
    sregs->gdt.base = 0x1000;
    sregs->cr3 = pml4_addr;
    sregs->cr4 = CR4_PAE;
    sregs->cr0 = CR0_PE | CR0_MP | CR0_ET | CR0_NE | CR0_WP | CR0_AM;
    sregs->efer = EFER_LME | EFER_NXE;

    gdt = (void *) (vm->mem + sregs->gdt.base);
    /* gdt[0] is the null segment */

    //32-bit code segment - needed for bootstrap
    sregs->gdt.limit = 3 * 8 - 1;

    seg.type = 11; /* Code: execute, read, accessed */
    seg.selector = 0x08;
    fill_segment_descriptor(gdt, &seg);
    sregs->cs = seg;

    //64-bit stuff
    sregs->gdt.limit = 18 * 8 - 1;
    seg.l = 1;

    //64-bit kernel code segment
    seg.type = 11; /* Code: execute, read, accessed */
    seg.selector = 0x10;
    seg.db = 0;
    fill_segment_descriptor(gdt, &seg);

    //64-bit kernel data segment
    seg.type = 3; /* Data: read/write, accessed */
    seg.selector = 0x18;
    fill_segment_descriptor(gdt, &seg);

    sregs->ds = sregs->es = sregs->fs = sregs->ss = seg;


    seg.dpl = 3;
    seg.db = 0;

    //64-bit user data segment
    seg.type = 3; /* Data: read/write, accessed */
    seg.selector = 0x28;
    fill_segment_descriptor(gdt, &seg);

    //64-bit user code segment
    seg.type = 11; /* Code: execute, read, accessed */
    seg.selector = 0x30;
    fill_segment_descriptor(gdt, &seg);

    //64-bit cpu kernel data segment
    seg.selector = 0x50;
    seg.type = 19; /* Data: read/write, accessed */
    //one for each cpu in future!
    seg.limit = sizeof(cpu_t) - 1;
    seg.base = CPU_START-0x1000;
    seg.s = 0;
    seg.dpl = 0;
    seg.present = 1;
    seg.avl = 0;
    seg.l = 1;
    seg.db = 0;
    seg.g = 0;
    fill_segment_descriptor(gdt, &seg);

}

static uint64_t setup_kernel_tss(struct vm_t *vm, struct kvm_sregs *sregs, uint64_t kernel_min_addr){

    struct kvm_segment seg;
    uint64_t *gdt;
    uint64_t tss_start;

    gdt = (void *) (vm->mem + sregs->gdt.base);
    tss_start = kernel_min_addr - 3*PAGE_SIZE;

    //TSS segment

    seg.selector = 0x40;
    seg.limit = TSS_SIZE-1;
    seg.base = tss_start;
    seg.type = 9;
    seg.s = 0;
    seg.dpl = 3;
    seg.present = 1;
    seg.avl = 0;
    seg.l = 1;
    seg.db = 0;
    seg.g = 0;
    fill_segment_descriptor(gdt, &seg);

    //map 3 pages required for TSS

    size_t i = 0;
    for (uint64_t p = tss_start; i < 3; p += PAGE_SIZE, i++) {
        map_physical_page(p, allocate_page(vm->mem, false), PDE64_WRITEABLE, 1, vm->mem);
    }

    return (tss_start);
}

void run(struct vm_t *vm, struct vcpu_t *vcpu, int kernel_binary_fd, int prog_binary_fd)
{
    struct kvm_sregs sregs;
    struct kvm_regs regs;

    elf_info_t kernel_elf_info;
    elf_info_t user_elf_info;

    uint64_t ksp;
    uint64_t tss_start;
    uint64_t user_split_addr;

    size_t i;
    uint64_t p;

    int ret;

    printf("Testing protected mode\n");

    if (ioctl(vcpu->fd, KVM_GET_SREGS, &sregs) < 0) {
        perror("KVM_GET_SREGS");
        exit(1);
    }

    setup_long_mode(vm, &sregs);

    kernel_elf_info = image_load(kernel_binary_fd, false, vm);
    printf("Entry point kernel: %p\n", kernel_elf_info.entry_addr);

    user_elf_info = image_load(prog_binary_fd, true, vm);
    printf("Entry point user: %p\n", user_elf_info.entry_addr);

    //allocate 20 stack pages for kernel stack
    ksp = kernel_elf_info.min_page_addr;
    for (p = ksp - PAGE_SIZE; i < 20; p -= PAGE_SIZE, i++) {

        uint64_t phy_addr = allocate_page(vm->mem, false);

        map_physical_page(p, phy_addr, PDE64_NO_EXE | PDE64_WRITEABLE | PDE64_USER, 1, vm->mem);
    }

    tss_start = setup_kernel_tss(vm, &sregs, p);

    user_split_addr = P2ALIGN(tss_start, 2*PAGE_SIZE) / 2;

    //allocate 1 stack pages for user stack
    //we will have a hard limit down to RLIMIT_STACK, but not all of these pages will be paged in
    //This will be handled by a page fault
    i = 0;
    /*for (p = user_split_addr - PAGE_SIZE; i < 50; p -= PAGE_SIZE, i++) {

        uint64_t phy_addr = allocate_page(vm->mem, false);

        map_physical_page(p, phy_addr, PDE64_NO_EXE | PDE64_WRITEABLE | PDE64_USER, 1, vm->mem);
    }*/
    printf("userstack %p\n", (void*)user_split_addr);

    if (ioctl(vcpu->fd, KVM_SET_SREGS, &sregs) < 0) {
        perror("KVM_SET_SREGS");
        exit(1);
    }

    memset(&regs, 0, sizeof(regs));
    /* Clear all FLAGS bits, except bit 1 which is always set. */
    regs.rflags = 2;
    //bootstrap entry
    regs.rip = 0;
    //kernel entry
    regs.rdi = (uint64_t) kernel_elf_info.entry_addr;
    //user entry
    regs.rsi = (uint64_t) user_elf_info.entry_addr;
    //kernel stack
    regs.rdx = ksp;
    //user split - for stack (down) and version storage (up) start point
    regs.rcx = user_split_addr;
    //user heap
    regs.r8 = user_elf_info.max_page_addr;
    //tss start
    regs.r9 = tss_start;

    if (ioctl(vcpu->fd, KVM_SET_REGS, &regs) < 0) {
        perror("KVM_SET_REGS");
        exit(1);
    }

    memcpy(vm->mem, bootstrap, bootstrap_end-bootstrap);
    printf("Loaded bootstrap: %" PRIu64 "\n", (uint64_t) (bootstrap_end-bootstrap));

    // Repeatedly run code and handle VM exits.
    while (1) {
        ret = ioctl(vcpu->fd, KVM_RUN, 0);
        if (ret == -1)
            err(1, "KVM_RUN");
        switch (vcpu->kvm_run->exit_reason) {
            case KVM_EXIT_IO:
                if (vcpu->kvm_run->io.direction == KVM_EXIT_IO_OUT && vcpu->kvm_run->io.size == 1 &&
                    vcpu->kvm_run->io.port == 0x3f8 &&
                    vcpu->kvm_run->io.port == 0x3f8 &&
                    vcpu->kvm_run->io.port == 0x3f8 &&
                    vcpu->kvm_run->io.port == 0x3f8 &&
                    vcpu->kvm_run->io.count == 1)
                    putchar(*(((char *) vcpu->kvm_run) + vcpu->kvm_run->io.data_offset));
                else
                    errx(1, "unhandled KVM_EXIT_IO");
                break;
            case KVM_EXIT_FAIL_ENTRY:
                errx(1, "KVM_EXIT_FAIL_ENTRY: hardware_entry_failure_reason = 0x%llx",
                     (unsigned long long) vcpu->kvm_run->fail_entry.hardware_entry_failure_reason);
                break;
            case KVM_EXIT_INTERNAL_ERROR:
                errx(1, "KVM_EXIT_INTERNAL_ERROR: suberror = 0x%x", vcpu->kvm_run->internal.suberror);
            case KVM_EXIT_HLT:

                if (ioctl(vcpu->fd, KVM_GET_REGS, &regs) < 0) {
                    perror("KVM_GET_REGS");
                    exit(1);
                }

                //add page table mapping in future
                //check for interupt

                uint64_t syscall_arg_phys;

                switch (regs.rax) {
                    //Exit
                    case 0: {
                        printf("Exit syscall\n");

                        char buffer[0x1000];
                        uint64_t number;
                        //printf("Size: %s %d\n", buffer, regs.rdx);
                        if (read_virtual_addr(0xDEADB000, 0x1000, buffer, vm->mem)) {
                            memcpy(&number, buffer+0xEEF, sizeof(uint64_t));
                            printf("0xDEADBEEF val: %" PRIu64 "\n", number);
                        }

                        return;
                    }
                    case 1:
                    {
                        char buffer[regs.rdx];
                        //printf("Size: %s %d\n", buffer, regs.rdx);
                        if (read_virtual_addr(regs.rsi, regs.rdx, buffer, vm->mem)){
                            regs.rax = write(regs.rdi, buffer, regs.rdx);
                        }else{
                            printf("Failed to write - Un-paged buffer?\n");
                        }
                    }
                        break;
                    case 2:
                        if (get_phys_addr(regs.rsi, &syscall_arg_phys, vm->mem)){
                            //printf("Read - buffer: %p, phys: %d, sp: %p\n", regs.rsi, syscall_arg_phys, vm, regs.rsp);
                            //Handle page boundary!
                            regs.rax = read(regs.rdi, vm->mem + syscall_arg_phys, regs.rdx);
                        }
                        break;
                    case 3:
                        {
                            char *buffer;
                            if (read_virtual_cstr(regs.rdi, &buffer, vm->mem)){
                                regs.rax = open(buffer, (int32_t)regs.rsi, (uint16_t)regs.rdx);
                                free(buffer);
                            }else{
                                printf("Failed to write - Un-paged buffer?\n");
                            }
                        }
                        break;
                    case 4:
                        regs.rax = close((int)regs.rdi);
                        break;
                    case 5:
                        regs.rax = allocate_page(vm->mem, regs.rsi == 1);
                        break;
                    case 6:
                        map_physical_page(regs.rdi, regs.rsi, regs.rdx, regs.rcx, vm->mem);
                        break;
                    case 7:
                        regs.rax = (uint64_t)is_vpage_present(regs.rdi, vm->mem);
                        break;
                    case 8:
                        printf("RAX: %llu, RDI %llu, RSI %llu\n", regs.rax, regs.rdi, regs.rsi);
                        break;
                    case 9:
                        printf("Host var: %" PRIx64 "\n", (int64_t)regs.rdi);
                        break;
                    default:
                        printf("Unsupported syscall %" PRIu64 "\n", (uint64_t) regs.rax);
                        return;

                }

                if (ioctl(vcpu->fd, KVM_SET_REGS, &regs) < 0) {
                    perror("KVM_SET_REGS");
                    exit(1);
                }

                break;
            case KVM_EXIT_MMIO:
            case KVM_EXIT_SHUTDOWN:

                if (ioctl(vcpu->fd, KVM_GET_REGS, &regs) < 0) {
                    perror("KVM_GET_REGS");
                    exit(1);
                }

                printf("MMIO Error: code = %d, PC: %p\n", vcpu->kvm_run->exit_reason, (void *) regs.rip);
                return;
            default:
                printf("Other exit: code = %d\n", vcpu->kvm_run->exit_reason);
                return;
        }
        continue;
    }
}
