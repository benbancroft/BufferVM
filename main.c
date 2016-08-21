#include <stdio.h>
#include <string.h>
#include <libelf.h>
#include <sys/mman.h>

#include <err.h>
#include <fcntl.h>
#include <linux/kvm.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#include <gelf.h>
#include <sysexits.h>
#include <unistd.h>

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

/* 32-bit page directory entry bits */
#define PDE32_PRESENT 1
#define PDE32_RW (1 << 1)
#define PDE32_USER (1 << 2)
#define PDE32_PS (1 << 7)

/* 64-bit page * entry bits */
#define PDE64_PRESENT 1
#define PDE64_RW (1 << 1)
#define PDE64_USER (1 << 2)
#define PDE64_ACCESSED (1 << 5)
#define PDE64_DIRTY (1 << 6)
#define PDE64_PS (1 << 7)
#define PDE64_G (1 << 8)

#define ALIGN(x, y) (((x)+(y)-1) & ~((y)-1))

uint32_t pd_addr = 0xFFEFF000;
uint32_t pt_start_addr = 0xFFB00000;
uint32_t page_counter = 0;

struct vm_t {
    int sys_fd;
    int fd;
    char *mem;
};

struct vcpu_t {
    int fd;
    struct kvm_run *kvm_run;
};

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

void printk(const char *msg) {
    fputs(msg, stderr);
}

/*void *resolve(const char* sym)
{
    static void *handle = NULL;
    if (handle == NULL) {
        handle = dlopen("libc.so", RTLD_NOW);
    }
    return dlsym(handle, sym);
}*/

void relocate(Elf32_Shdr *shdr, const Elf32_Sym *syms, const char *strings, const char *src, char *dst) {
    Elf32_Rel *rel = (Elf32_Rel *) (src + shdr->sh_offset);
    int j;
    for (j = 0; j < shdr->sh_size / sizeof(Elf32_Rel); j += 1) {
        const char *sym = strings + syms[ELF32_R_SYM(rel[j].r_info)].st_name;
        switch (ELF32_R_TYPE(rel[j].r_info)) {
            case R_386_JMP_SLOT:
            case R_386_GLOB_DAT:
                //TODO
                //*(Elf32_Word*)(dst + rel[j].r_offset) = (Elf32_Word)resolve(sym);
                break;
        }
    }
}

void *find_sym(const char *name, Elf32_Shdr *shdr, const char *strings, const char *src, char *dst) {
    Elf32_Sym *syms = (Elf32_Sym *) (src + shdr->sh_offset);
    int i;
    for (i = 0; i < shdr->sh_size / sizeof(Elf32_Sym); i += 1) {
        if (strcmp(name, strings + syms[i].st_name) == 0) {
            return dst + syms[i].st_value;
        }
    }
    return NULL;
}

void *image_load(int fd, struct vm_t *vm) {
    Elf *e;
    char *id, bytes[5];
    size_t n;
    GElf_Ehdr ehdr;

    void *entry = NULL;

    if (elf_version(EV_CURRENT) == EV_NONE)
        errx(EX_SOFTWARE, "ELF library initialization failed: %s", elf_errmsg(-1));

    if ((e = elf_begin(fd, ELF_C_READ, NULL)) == NULL)
        errx(EX_SOFTWARE, "elf_begin () failed: %s.", elf_errmsg(-1));

    if (elf_kind(e) != ELF_K_ELF)
        errx(EX_DATAERR, "Not an ELF object.");

    if (gelf_getehdr(e, &ehdr) == NULL)
        errx(EX_SOFTWARE, "getehdr () failed: %s.", elf_errmsg(-1));

    entry = ehdr.e_entry;

    if (elf_getphdrnum(e, &n) != 0)
        errx(EX_DATAERR, "elf_getphdrnum () failed: %s.", elf_errmsg(-1));

    GElf_Phdr phdr;
    uint64_t start;
    uint64_t taddr;

    for (int i = 0; i < n; i++) {

        if (gelf_getphdr(e, i, &phdr) != &phdr)
            errx(EX_SOFTWARE, "getphdr () failed: %s.", elf_errmsg(-1));

        if (phdr.p_type != PT_LOAD) {
            continue;
        }
        if (phdr.p_filesz > phdr.p_memsz) {
            printk("image_load:: p_filesz > p_memsz\n");
            //TODO
            //munmap(exec, size);
            return 0;
        }
        if (!phdr.p_filesz) {
            continue;
        }

        // p_filesz can be smaller than p_memsz,
        // the difference is zeroe'd out.
        start = elf_rawfile(e, NULL) + phdr.p_offset;
        taddr = phdr.p_vaddr;

        //char *segment = mmap(taddr, phdr[i].p_memsz, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);;

        int flags = PROT_READ | PROT_WRITE | PROT_EXEC;

        if (!(phdr.p_flags & PF_W))
            //Read only.
            flags = PROT_READ;

        if (phdr.p_flags & PF_X)
            // Executable.
            flags = PROT_EXEC;

        if (load_address_space(taddr, phdr.p_memsz, start, phdr.p_filesz, flags, vm)) {
            return 0;
        }

        printf("Loaded header at %p\n", taddr);
    }

    GElf_Shdr shdr;
    size_t shstrndx, sz;

    if (elf_getshdrstrndx(e, &shstrndx) != 0)
        errx(EX_SOFTWARE, "elf_getshdrstrndx () failed: %s.", elf_errmsg(-1));

    Elf_Scn *scn = NULL;
    Elf_Data *data = NULL;
    char *name;

    /*while ((scn = elf_nextscn(e, scn)) != NULL) {
        if (gelf_getshdr(scn , &shdr) != &shdr)
            errx(EX_SOFTWARE , "getshdr () failed: %s.", elf_errmsg ( -1));

        if (shdr.sh_type == SHT_DYNSYM) {
            //syms = (Elf32_Sym*)(elf_start + shdr[i].sh_offset);

            if ((data = elf_getdata(scn, data)) != NULL)
            {
                printf("Test: %i\n", data->d_off);
            }else{
                printf("fail\n");
            }


            //strings = elf_start + shdr[shdr[i].sh_link].sh_offset;
            //entry = find_sym("_start", shdr + i, strings, elf_start, exec);
            //printf("Found entry: %p", entry);
            //break;
        }else{
            printf("fail: %i\n", shdr.sh_type);
        }

        if ((name = elf_strptr(e, shstrndx , shdr.sh_name )) == NULL)
            errx(EX_SOFTWARE , "elf_strptr () failed: %s.", elf_errmsg ( -1));

        printf("Section %-4.4jd %s\n", (uintmax_t) elf_ndxscn(scn), name);
    }*/

    /*shdr = (Elf32_Shdr *)(elf_start + hdr->e_shoff);

    for(i=0; i < hdr->e_shnum; ++i) {
        if (shdr[i].sh_type == SHT_DYNSYM) {
            syms = (Elf32_Sym*)(elf_start + shdr[i].sh_offset);
            strings = elf_start + shdr[shdr[i].sh_link].sh_offset;
            entry = find_sym("_start", shdr + i, strings, elf_start, exec);
		printf("Found entry: %p", entry);
            break;
        }
    }*/

    //TODO - dynamic stoof
    /*for(i=0; i < hdr->e_shnum; ++i) {
        if (shdr[i].sh_type == SHT_REL) {
            relocate(shdr + i, syms, strings, elf_start, exec);
        }
    }*/

    elf_end(e);

    return entry;

}

/* image_load */

int read_binary(char *name, char **buffer) {
    FILE *file;

    unsigned long file_len;

    //Open file
    file = open(name, O_RDONLY, 0);
    if (file < 0) {
        fprintf(stderr, "Unable to open file %s\n", name);
        return 0;
    }

    return file;
}

typedef struct __attribute__ ((packed)){
    uint32_t pagetable;
    uint32_t page;
    uint32_t offset;
}pageinfo, *ppageinfo;
pageinfo mm_virtaddrtopageindex(unsigned long addr){
    pageinfo pginf;

    pginf.offset = addr & 0x00000FFF;
    //align address to 4k (highest 20-bits of address)
    addr &= ~0xFFF;
    pginf.pagetable = addr / 0x400000; // each page table covers 0x400000 bytes in memory
    pginf.page = (addr % 0x400000) / 0x1000; //0x1000 = page size
    return pginf;
}

int32_t get_physaddr(uint32_t virtualaddr, struct vm_t *vm)
{
    pageinfo pginf = mm_virtaddrtopageindex(virtualaddr); // get the PDE and PTE indexes for the addr

    uint32_t *pd = (void *)(vm->mem + pd_addr);
    uint32_t *pt = (void *)(vm->mem + pt_start_addr);

    if(pd[pginf.pagetable] & 1){
        // page table exists.
        uint32_t *page_physical = (void *)(vm->mem + (pd[pginf.pagetable] & 0xFFFFF000));

        return (page_physical[pginf.page] & 0xFFFFF000) + pginf.offset;
    }

    return 0;
}

/**
start_addr should be page aligned
*/
int load_address_space(uint64_t start_addr, size_t mem_size, char *elf_seg_start, size_t elf_seg_size, int flags, struct vm_t *vm) {

    for (uint32_t i = start_addr; i < start_addr + mem_size; i += 0x1000) {

        uint32_t physical_addr = 0x10000 + page_counter++ * 0x1000;

        memset(vm->mem + physical_addr, 0x0, mem_size);
        memcpy(vm->mem + physical_addr, elf_seg_start, elf_seg_size);

        map_address_space(i, physical_addr, vm);

    }


    return 0;
}

int map_address_space(uint64_t start_addr, uint64_t physical_addr, struct vm_t *vm) {

    uint32_t *pd = (void *)(vm->mem + pd_addr);
    uint32_t *pt = (void *)(vm->mem + pt_start_addr);

    pageinfo pginf = mm_virtaddrtopageindex(start_addr); // get the PDE and PTE indexes for the addr

    if(pd[pginf.pagetable] & 1){
        // page table exists.
        uint32_t *page_physical = (void *)(vm->mem + (pd[pginf.pagetable] & 0xFFFFF000));
        if(!page_physical[pginf.page] & 1){
            // page isn't mapped
            page_physical[pginf.page] = physical_addr & 0xFFFFF000 | 3;
        }else{
            // page is already mapped
            //return status_error;
            //TODO
        }
    }else{
        // doesn't exist, so alloc a page and add into pdir
        uint32_t new_page_table = 0x10000 + page_counter++ * 0x1000;
        uint32_t *page_physical = (void *)(vm->mem + new_page_table);
        //uint32_t *page_table = pt + (pginf.pagetable * 0x1000); // virt addr of page tbl

        pd[pginf.pagetable] = (uint32_t) new_page_table | 3; // add the new page table into the pdir

        page_physical[pginf.page] = (uint32_t)physical_addr & 0xFFFFF000 | 3; // map the page!
    }


    return 0;
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
    uint32_t limit = seg->g ? seg->limit >> 12 : seg->limit;
    dt[seg->selector] = (limit & 0xffff) /* Limit bits 0:15 */
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
            .selector = 1,
            .present = 1,
            .dpl = 0,
            .db = 1,
            .s = 1, /* Code/data */
            .l = 0,
            .g = 1, /* 4KB granularity */
    };
    uint64_t *gdt;

    //CR0_PE
    sregs->cr0 |= 1; /* enter protected mode */
    sregs->gdt.base = 4096;
    sregs->gdt.limit = 3 * 8 - 1;

    gdt = (void *)(vm->mem + sregs->gdt.base);
    /* gdt[0] is the null segment */

    seg.type = 11; /* Code: execute, read, accessed */
    seg.selector = 1;
    fill_segment_descriptor(gdt, &seg);
    sregs->cs = seg;

    seg.type = 3; /* Data: read/write, accessed */
    seg.selector = 2;
    fill_segment_descriptor(gdt, &seg);
    sregs->cs = sregs->ds = sregs->es = sregs->fs = sregs->gs
            = sregs->ss = seg;
}

/*void run_vm(struct vm_t *vm, uint64_t start_pointer) {

    size_t mmap_size;
    struct kvm_run *run;

    int ret;

    vm->vcpufd = ioctl(vm->vmfd, KVM_CREATE_VCPU, (unsigned long) 0);
    if (vm->vcpufd == -1)
        err(1, "KVM_CREATE_VCPU");

    // Map the shared kvm_run structure and following data.
    ret = ioctl(vm->kvm, KVM_GET_VCPU_MMAP_SIZE, NULL);
    if (ret == -1)
        err(1, "KVM_GET_VCPU_MMAP_SIZE");
    mmap_size = ret;
    if (mmap_size < sizeof(*run))
        errx(1, "KVM_GET_VCPU_MMAP_SIZE unexpectedly small");
    run = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, vm->vcpufd, 0);
    if (!run)
        err(1, "mmap vcpu");

    struct kvm_sregs sregs;
    struct kvm_regs regs;

    printf("Testing protected mode\n");

    if (ioctl(vm->vcpufd, KVM_GET_SREGS, &sregs) < 0) {
        perror("KVM_GET_SREGS");
        exit(1);
    }

    setup_protected_mode(vm, &sregs);

    if (ioctl(vm->vcpufd, KVM_SET_SREGS, &sregs) < 0) {
        perror("KVM_SET_SREGS");
        exit(1);
    }

    memset(&regs, 0, sizeof(regs));
    // Clear all FLAGS bits, except bit 1 which is always set.
    regs.rflags = 2;
    regs.rip = start_pointer-0x8048000;

    ret = ioctl(vm->vcpufd, KVM_SET_REGS, &regs);
    if (ret == -1)
        err(1, "KVM_SET_REGS");

    // Repeatedly run code and handle VM exits.
    while (1) {
        ret = ioctl(vm->vcpufd, KVM_RUN, NULL);
        if (ret == -1)
            err(1, "KVM_RUN");
        switch (run->exit_reason) {
            case KVM_EXIT_HLT:
                puts("KVM_EXIT_HLT");
                return 0;
            case KVM_EXIT_IO:
                if (run->io.direction == KVM_EXIT_IO_OUT && run->io.size == 1 && run->io.port == 0x3f8 &&
                    run->io.count == 1)
                    putchar(*(((char *) run) + run->io.data_offset));
                else
                    errx(1, "unhandled KVM_EXIT_IO");
                break;
            case KVM_EXIT_FAIL_ENTRY:
                errx(1, "KVM_EXIT_FAIL_ENTRY: hardware_entry_failure_reason = 0x%llx",
                     (unsigned long long) run->fail_entry.hardware_entry_failure_reason);
                break;
            case KVM_EXIT_INTERNAL_ERROR: {
                kvm_show_regs(vm);

                //printf("PC: %p", err_regs.rip);

                errx(1, "KVM_EXIT_INTERNAL_ERROR: suberror = 0x%x", run->internal.suberror);
            }
                break;
            default:
                errx(1, "exit_reason = 0x%x", run->exit_reason);
                break;
        }
    }
}*/

int check(struct vm_t *vm, struct vcpu_t *vcpu, size_t sz)
{
    struct kvm_regs regs;
    uint64_t memval = 0;

    memcpy(&memval, &vm->mem[get_physaddr(0xDEADBEEF, vm)], sz);

    if (memval != 42) {
        printf("Wrong result: memory at 0xDEADBEEF is %lld\n",
               (unsigned long long)memval);
        return 0;
    }

    return 1;
}

static void setup_paged_32bit_mode(struct vm_t *vm, struct kvm_sregs *sregs)
{

    sregs->cr3 = pd_addr;
    sregs->cr4 = CR4_PSE;
    sregs->cr0 = CR0_PE | CR0_MP | CR0_ET | CR0_NE | CR0_WP | CR0_AM;
    //sregs->cr0 |= 0x80000000;
    sregs->efer = 0;

    /* We don't set cr0.pg here, because that causes a vm entry
       failure. It's not clear why. Instead, we set it in the VM
       code. */
}

void run_protected_mode(struct vm_t *vm, struct vcpu_t *vcpu, uint64_t entry_point)
{
    struct kvm_sregs sregs;
    struct kvm_regs regs;

    uint32_t physical_addr = 0x10000 + page_counter++ * 0x1000;

    //int magic = 21;

    //memcpy(vm->mem+physical_addr, magic, sizeof(magic));

    map_address_space(0xDEADB000, physical_addr, vm);
    map_address_space(0xC0DED000, physical_addr, vm);

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
    setup_paged_32bit_mode(vm, &sregs);

    //sregs.cr0 = 0x80000000;

    if (ioctl(vcpu->fd, KVM_SET_SREGS, &sregs) < 0) {
        perror("KVM_SET_SREGS");
        exit(1);
    }

    memset(&regs, 0, sizeof(regs));
    /* Clear all FLAGS bits, except bit 1 which is always set. */
    regs.rflags = 2;
    regs.rip = entry_point;
    regs.rsp = 0xc0000000;
    //regs.rip = entry_point;

    if (ioctl(vcpu->fd, KVM_SET_REGS, &regs) < 0) {
        perror("KVM_SET_REGS");
        exit(1);
    }

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


int main(int argc, char **argv, char **envp) {
    if (argc < 2) {
        fprintf(stderr, "%s binary.bin\n", argv[0]);
        return 1;
    }


    //int (*ptr)(int, char **, char**);
    char *code;
    int binary_size;
    if (!(binary_size = read_binary(argv[1], &code))) {
        return 1;
    }

    struct vm_t vm;
    struct vcpu_t vcpu;

    vm_init(&vm, 0xFFF00000);
    vcpu_init(&vm, &vcpu);

    char *entry_point = image_load(binary_size, &vm);

    close(binary_size);

    printf("Entry point: %p, physical addr: %p\n", entry_point, get_physaddr(entry_point, &vm));

    printf("Main addr: %p\n", get_physaddr(0x804881e, &vm));

    run_protected_mode(&vm, &vcpu, get_physaddr(entry_point, &vm));

    //run_vm(&vm, ptr);

    return 0;
}