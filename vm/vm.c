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
#include <fcntl.h>
#include <zconf.h>

#include "vm.h"
#include "memory.h"

extern const unsigned char code64[], code64_end[];

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

    if (ioctl(vm->fd, KVM_SET_TSS_ADDR, 0xfffbd000) < 0) {
        perror("KVM_SET_TSS_ADDR");
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

static void print_seg(FILE *file, const char *name, struct kvm_segment *seg) {
    fprintf(stderr,
            "%s %04x (%08llx/%08x p %d dpl %d db %d s %d type %x l %d"
                    " g %d avl %d)\n",
            name, seg->selector, seg->base, seg->limit, seg->present,
            seg->dpl, seg->db, seg->s, seg->type, seg->l, seg->g,
            seg->avl);
}

static void print_dt(FILE *file, const char *name, struct kvm_dtable *dt) {
    fprintf(stderr, "%s %llx/%x\n", name, dt->base, dt->limit);
}

void kvm_show_regs(struct vm_t *vm) {
    int fd = vm->sys_fd;
    struct kvm_regs regs;
    struct kvm_sregs sregs;
    int r;

    r = ioctl(fd, KVM_GET_REGS, &regs);
    if (r == -1) {
        perror("KVM_GET_REGS");
        return;
    }
    fprintf(stderr,
            "rax %016llx rbx %016llx rcx %016llx rdx %016llx\n"
                    "rsi %016llx rdi %016llx rsp %016llx rbp %016llx\n"
                    "r8  %016llx r9  %016llx r10 %016llx r11 %016llx\n"
                    "r12 %016llx r13 %016llx r14 %016llx r15 %016llx\n"
                    "rip %016llx rflags %08llx\n",
            regs.rax, regs.rbx, regs.rcx, regs.rdx,
            regs.rsi, regs.rdi, regs.rsp, regs.rbp,
            regs.r8, regs.r9, regs.r10, regs.r11,
            regs.r12, regs.r13, regs.r14, regs.r15,
            regs.rip, regs.rflags);
    r = ioctl(fd, KVM_GET_SREGS, &sregs);
    if (r == -1) {
        perror("KVM_GET_SREGS");
        return;
    }
    print_seg(stderr, "cs", &sregs.cs);
    print_seg(stderr, "ds", &sregs.ds);
    print_seg(stderr, "es", &sregs.es);
    print_seg(stderr, "ss", &sregs.ss);
    print_seg(stderr, "fs", &sregs.fs);
    print_seg(stderr, "gs", &sregs.gs);
    print_seg(stderr, "tr", &sregs.tr);
    print_seg(stderr, "ldt", &sregs.ldt);
    print_dt(stderr, "gdt", &sregs.gdt);
    print_dt(stderr, "idt", &sregs.idt);
    fprintf(stderr, "cr0 %llx cr2 %llx cr3 %llx cr4 %llx cr8 %llx"
                    " efer %llx\n",
            sregs.cr0, sregs.cr2, sregs.cr3, sregs.cr4, sregs.cr8,
            sregs.efer);
}

void fill_segment_descriptor(uint64_t *dt, struct kvm_segment *seg)
{
    uint16_t index = seg->selector >> 3;
    uint32_t limit = seg->g ? seg->limit >> 12 : seg->limit;
    dt[index] = (limit & 0xffff) /* Limit bits 0:15 */
                | (seg->base & 0xffffff) << 16 /* Base bits 0:23 */
                | (uint64_t)seg->type << 40
                | (uint64_t)seg->s << 44 /* system or code/data */
                | (uint64_t)seg->dpl << 45 /* Privilege level */
                | (uint64_t)seg->present << 47
                | (limit & 0xf0000ULL) << 48 /* Limit bits 16:19 */
                | (uint64_t)seg->avl << 52 /* Available for system software */
                | (uint64_t)seg->l << 53 /* 64-bit code segment */
                | (uint64_t)seg->db << 54 /* 16/32-bit segment */
                | (uint64_t)seg->g << 55 /* 4KB granularity */
                | (seg->base & 0xff000000ULL) << 56; /* Base bits 24:31 */
}

static void setup_protected_mode(struct vm_t *vm, struct kvm_sregs *sregs)
{
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

    sregs->cr0 |= CR0_PE; /* enter protected mode */
    sregs->gdt.base = 0x1000;
    sregs->gdt.limit = 3 * 8 - 1;

    gdt = (void *)(vm->mem + sregs->gdt.base);
    /* gdt[0] is the null segment */

    seg.type = 11; /* Code: execute, read, accessed */
    seg.selector = 1 << 3;
    fill_segment_descriptor(gdt, &seg);
    sregs->cs = seg;

    seg.type = 3; /* Data: read/write, accessed */
    seg.selector = 2 << 3;
    fill_segment_descriptor(gdt, &seg);
    sregs->ds = sregs->es = sregs->fs = sregs->gs
            = sregs->ss = seg;
}

static void setup_64bit_code_segment(struct vm_t *vm, struct kvm_sregs *sregs)
{
    struct kvm_segment seg = {
            .base = 0,
            .limit = 0xffffffff,
            .selector = 3 << 3,
            .present = 1,
            .type = 11, /* Code: execute, read, accessed */
            .dpl = 0,
            .db = 0,
            .s = 1, /* Code/data */
            .l = 1,
            .g = 1, /* 4KB granularity */
    };
    uint64_t *gdt = (void *)(vm->mem + sregs->gdt.base);

    sregs->gdt.limit = 4 * 8 - 1;

    fill_segment_descriptor(gdt, &seg);
}

static void setup_long_mode(struct vm_t *vm, struct kvm_sregs *sregs)
{
    uint64_t pml4_addr = 0x2000;
    uint64_t *pml4 = (void *)(vm->mem + pml4_addr);

    uint64_t pdpt_addr = 0x3000;
    uint64_t *pdpt = (void *)(vm->mem + pdpt_addr);

    uint64_t pd_addr = 0x4000;
    uint64_t *pd = (void *)(vm->mem + pd_addr);

    pml4[0] = PDE64_PRESENT | PDE64_RW | PDE64_USER | pdpt_addr;
    pdpt[0] = PDE64_PRESENT | PDE64_RW | PDE64_USER | pd_addr;
    pd[0] = PDE64_PRESENT | PDE64_RW | PDE64_USER | PDE64_PS;

    sregs->cr3 = pml4_addr;
    sregs->cr4 = CR4_PAE;
    sregs->cr0 = CR0_PE | CR0_MP | CR0_ET | CR0_NE | CR0_WP | CR0_AM;
    sregs->efer = EFER_LME;

    /* We don't set cr0.pg here, because that causes a vm entry
       failure. It's not clear why. Instead, we set it in the VM
       code. */
    setup_64bit_code_segment(vm, sregs);
}

int check(struct vm_t *vm, struct vcpu_t *vcpu, size_t sz)
{
    struct kvm_regs regs;
    uint64_t memval = 0;

    if (ioctl(vcpu->fd, KVM_GET_REGS, &regs) < 0) {
        perror("KVM_GET_REGS");
        exit(1);
    }

    if (regs.rax != 42) {
        printf("Wrong result: {E,R,}AX is %lld\n", regs.rax);
        return 0;
    }

    memcpy(&memval, &vm->mem[0x400], sz);
    if (memval != 42) {
        printf("Wrong result: memory at 0x400 is %lld\n",
               (unsigned long long)memval);
        return 0;
    }

    return 1;
}

void run(struct vm_t *vm, struct vcpu_t *vcpu, uint64_t entry_point)
{
    struct kvm_sregs sregs;
    struct kvm_regs regs;

    uint32_t physical_addr = 0x10000 + page_counter++ * 0x1000;

    //int magic = 21;

    //memcpy(vm->mem+physical_addr, magic, sizeof(magic));

    //map_address_space(0xDEADB000, physical_addr, vm);
    //map_address_space(0xC0DED000, physical_addr, vm);

    //allocate 50 stack pages
    /*for (uint32_t i = 0xc0000000; i > 0xc0000000 - 0x50000; i -= 0x1000) {

        uint32_t physical_addr = 0x10000 + page_counter++ * 0x1000;

        map_address_space(i, physical_addr, vm);

    }*/

    int ret;

    printf("Testing protected mode\n");

    if (ioctl(vcpu->fd, KVM_GET_SREGS, &sregs) < 0) {
        perror("KVM_GET_SREGS");
        exit(1);
    }

    setup_protected_mode(vm, &sregs);
    setup_long_mode(vm, &sregs);

    //sregs.cr0 = 0x80000000;

    if (ioctl(vcpu->fd, KVM_SET_SREGS, &sregs) < 0) {
        perror("KVM_SET_SREGS");
        exit(1);
    }

    memset(&regs, 0, sizeof(regs));
    /* Clear all FLAGS bits, except bit 1 which is always set. */
    regs.rflags = 2;
    regs.rip = 0;
    //regs.rsp = 0xc0000000;
    //regs.rip = entry_point;

    if (ioctl(vcpu->fd, KVM_SET_REGS, &regs) < 0) {
        perror("KVM_SET_REGS");
        exit(1);
    }

    memcpy(vm->mem, code64, code64_end-code64);

    // Repeatedly run code and handle VM exits.
    while (1) {
        ret = ioctl(vcpu->fd, KVM_RUN, 0);
        if (ret == -1)
            err(1, "KVM_RUN");
        switch (vcpu->kvm_run->exit_reason) {
            case KVM_EXIT_IO:
                if (vcpu->kvm_run->io.direction == KVM_EXIT_IO_OUT && vcpu->kvm_run->io.size == 1 && vcpu->kvm_run->io.port == 0x3f8 &&
                    vcpu->kvm_run->io.count == 1)
                    putchar(*(((char *) vcpu->kvm_run) + vcpu->kvm_run->io.data_offset));
                else
                    errx(1, "unhandled KVM_EXIT_IO");
                break;
            case KVM_EXIT_FAIL_ENTRY:
                errx(1, "KVM_EXIT_FAIL_ENTRY: hardware_entry_failure_reason = 0x%llx",
                     (unsigned long long) vcpu->kvm_run->fail_entry.hardware_entry_failure_reason);
                break;
            case KVM_EXIT_INTERNAL_ERROR: {
                kvm_show_regs(vm);

                //printf("PC: %p", err_regs.rip);

                errx(1, "KVM_EXIT_INTERNAL_ERROR: suberror = 0x%x", vcpu->kvm_run->internal.suberror);
            }
                break;
            case KVM_EXIT_SHUTDOWN:
                /*errx(1, "KVM_EXIT_FAIL_ENTRY: hardware_entry_failure_reason = 0x%llx",
                     (unsigned long long) vcpu->kvm_run->fail_entry.hardware_entry_failure_reason);*/

                if (ioctl(vcpu->fd, KVM_GET_REGS, &regs) < 0) {
                    perror("KVM_GET_REGS");
                    exit(1);
                }

                //add page table mapping in future
                //check for interupt
                if (vm->mem[regs.rip] == (char)0xCD && vm->mem[regs.rip+1] == (char)0x80){

                    printf("Syscall at Instr: %p\n", regs.rip);

                    //syscall(regs.rax, regs.rbx, vm->mem+regs.rcx, regs.rdx);
                    //

                    switch((int)regs.rax){
                        //Exit
                        case 1:
                            printf("Exit syscall\n");
                            check(vm, vcpu, 4);
                            return;
                        case 4:
                            //printf("Write - buffer: %p, phys: %d, sp: %p\n", regs.rcx, get_physaddr(regs.rcx, vm), regs.rsp);
                            write(regs.rbx, vm->mem + get_physaddr(regs.rcx, vm), regs.rdx);
                            break;

                    }
                }else{
                    printf("Instruction: %#04hhx\n", vm->mem[regs.rip]);
                    printf("MMIO Error: code = %d\n", vcpu->kvm_run->exit_reason);
                    check(vm, vcpu, 4);
                    return;
                }

                regs.rip += 2;

                if (ioctl(vcpu->fd, KVM_SET_REGS, &regs) < 0) {
                    perror("KVM_SET_REGS");
                    exit(1);
                }

                break;
            case KVM_EXIT_MMIO:
                printf("MMIO Error: code = %d\n", vcpu->kvm_run->exit_reason);
                break;
            case KVM_EXIT_HLT:
            default:
                printf("Other exit: code = %d\n", vcpu->kvm_run->exit_reason);
                check(vm, vcpu, 4);
                return;
        }
        continue;
    }
}