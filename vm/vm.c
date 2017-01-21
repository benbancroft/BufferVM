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
#include <signal.h>
#include <errno.h>
#include <sys/uio.h>

#include "vm.h"
#include "../common/paging.h"
#include "elf.h"
#include "gdt.h"
#include "../common/elf.h"
#include "../common/syscall.h"

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
    map_physical_pages(0x0000, 0x0000, PDE64_WRITEABLE | PDE64_USER, 1, false, vm->mem);
    //Page for gdt
    int64_t gdt_page = map_physical_pages(0x1000, 0x1000, PDE64_WRITEABLE | PDE64_USER, 1, true, vm->mem);
    printf("GDT page %" PRIx64 "\n", gdt_page);

    //map_physical_pages(0xDEADB000, allocate_pages(1, vm->mem, false), PDE64_WRITEABLE, 1, false, vm->mem);

    sregs->cr0 |= CR0_PE; /* enter protected mode */
    sregs->gdt.base = gdt_page;
    sregs->cr3 = pml4_addr;
    sregs->cr4 = CR4_PAE | CR4_OSFXSR | CR4_OSXMMEXCPT;
    sregs->cr0 = CR0_PE | CR0_MP | CR0_ET | CR0_NE | CR0_WP | CR0_AM;
    sregs->efer = EFER_LME | EFER_NXE;

    gdt = (void *) (vm->mem + gdt_page);
    /* gdt[0] is the null segment */

    gdt[0] = 0xDEADBEEF;

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
    /*seg.selector = 0x50;
    seg.type = 19;
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
    fill_segment_descriptor(gdt, &seg);*/

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
        map_physical_pages(p, -1, PDE64_WRITEABLE, 1, false, vm->mem);
    }

    return (tss_start);
}

void intHandler(int signal) {
    //Here to be interrupted
}

void run(struct vm_t *vm, struct vcpu_t *vcpu, int kernel_binary_fd, char *user_binary_location)
{
    signal(SIGINT, intHandler);

    struct kvm_sregs sregs;
    struct kvm_regs regs;

    elf_info_t kernel_elf_info = { NULL, 0, 0, 0, 0 };

    uint64_t ksp_max;
    uint64_t ksp;
    uint64_t tss_start;
    uint64_t user_binary_location_addr;

    size_t i;
    uint64_t p;

    int ret;

    printf("Testing protected mode\n");

    if (ioctl(vcpu->fd, KVM_GET_SREGS, &sregs) < 0) {
        perror("KVM_GET_SREGS");
        exit(1);
    }

    setup_long_mode(vm, &sregs);

    load_elf_binary(kernel_binary_fd, NULL, &kernel_elf_info, true, vm->mem);
    printf("Entry point kernel: %p\n", kernel_elf_info.entry_addr);

    /*load_elf_binary(prog_binary_fd, &user_elf_info, vm->mem);
    printf("Entry point user: %p\n", user_elf_info.entry_addr);*/

    //allocate 20 stack pages for kernel stack
    ksp = kernel_elf_info.min_page_addr;
    for (p = ksp - PAGE_SIZE; i < 20; p -= PAGE_SIZE, i++) {
        map_physical_pages(p, -1, PDE64_NO_EXE | PDE64_WRITEABLE | PDE64_USER, 1, false, vm->mem);
    }

    //copy user binary location to top of stack
    ksp_max = ksp;
    size_t user_loc_size = strlen(user_binary_location) + 1;
    if (!write_virtual_addr(ksp - user_loc_size, user_binary_location, user_loc_size, vm->mem))
    {
        perror("Error writing user binary location.");
        exit(1);
    }
    user_binary_location_addr = ksp - user_loc_size;
    ksp = P2ALIGN(user_binary_location_addr, MIN_ALIGN);

    tss_start = setup_kernel_tss(vm, &sregs, p);

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
    //kernel stack max
    regs.rsi = ksp_max;
    //kernel stack actual
    regs.rdx = ksp;
    //tss start
    regs.rcx = tss_start;
    //user_binary_location
    regs.r8 = user_binary_location_addr;

    if (ioctl(vcpu->fd, KVM_SET_REGS, &regs) < 0) {
        perror("KVM_SET_REGS");
        exit(1);
    }

    memcpy(vm->mem, bootstrap, bootstrap_end-bootstrap);
    printf("Loaded bootstrap: %" PRIu64 "\n", (uint64_t) (bootstrap_end-bootstrap));

    // Repeatedly run code and handle VM exits.
    while (1) {
        ret = ioctl(vcpu->fd, KVM_RUN, 0);
        if (ret == -1){
            if (ioctl(vcpu->fd, KVM_GET_REGS, &regs) < 0) {
                perror("KVM_GET_REGS");
                exit(1);
            }
            printf("KVM_RUN exited with error: %d, PC: %p\n", ret, (void *)regs.rip);

        }
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

                switch (regs.rax) {
                    //Exit
                    case 0: {
                        printf("Exit hostcall\n");

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
                        {
                            char buffer[regs.rdx];
                            regs.rax = read(regs.rdi, buffer, regs.rdx);
                            if (!write_virtual_addr(regs.rsi, buffer, regs.rdx, vm->mem))
                                regs.rax = -1;
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
                    case 10:
                        regs.rax = dup((int)regs.rdi);
                        break;
                    case 11:
                        {
                            struct stat stats;
                            regs.rax = fstat((int)regs.rdi, &stats);
                            if (!write_virtual_addr(regs.rsi, (char *)&stats, sizeof (stats), vm->mem))
                                regs.rax = -1;
                        }
                        break;
                    case 12:
                    {
                        regs.rax =
                                ((uint64_t)mmap(vm->mem + regs.rdi, regs.rsi, regs.rdx, regs.rcx, regs.r8, regs.r9)) -
                                        (uint64_t)vm->mem;
                    }
                        break;
                    case 13:
                        regs.rax = lseek(regs.rdi, regs.rsi, regs.rdx);
                        break;
                    case 5:
                        regs.rax = allocate_pages(regs.rdi, vm->mem, regs.rdx == 1);
                        break;
                    case 6:
                        regs.rax = map_physical_pages(regs.rdi, regs.rsi, regs.rdx, regs.rcx, regs.r8, vm->mem);
                        break;
                    case 14:
                        unmap_physical_page(regs.rdi, vm->mem);
                        break;
                    case 15:
                    {
                        struct iovec vectors[regs.rdx];
                        struct iovec *vec;
                        void *vecbuf;
                        size_t ret = 0;

                        if (read_virtual_addr(regs.rsi, regs.rdx * sizeof (struct iovec), vectors, vm->mem)){
                            for (size_t i = 0; i < regs.rdx; i++){
                                vec = &vectors[i];

                                vecbuf = malloc(vec->iov_len);
                                if (!read_virtual_addr((uint64_t)vec->iov_base, vec->iov_len, vecbuf, vm->mem)){
                                    ret = -EFAULT;
                                    break;
                                }
                                vec->iov_base = vecbuf;
                            }
                            if (!IS_ERR_VALUE(ret)){
                                ret = writev(regs.rdi, vectors, regs.rdx);
                                for (size_t i = 0; i < regs.rdx; i++){
                                    vec = &vectors[i];
                                    free(vec->iov_base);
                                }
                            }

                        } else
                            ret = -EFAULT;
                        regs.rax = ret;
                    }
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
                        printf("Unsupported hostcall %" PRIu64 "\n", (uint64_t) regs.rax);
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
