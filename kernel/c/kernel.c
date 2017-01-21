//
// Created by ben on 13/10/16.
//

#include "../h/cpu.h"
#include "../h/idt.h"
#include "../h/tss.h"
#include "../h/kernel.h"
#include "../h/syscall.h"
#include "../../common/paging.h"
#include "../h/host.h"

#include "../../intelxed/kit/include/xed-interface.h"
#include "../h/stack.h"
#include "../../common/elf.h"
#include "../h/utils.h"
#include "../h/gdt.h"

uint64_t kernel_min_address;

tss_entry_t *tss;

uint64_t kernel_stack;
uint64_t user_heap_start;
uint64_t user_version_start;

void xed_user_abort_function(const char *msg, const char *file, int line, void *other){
    printf("abort thing needs writing!\n");
    host_exit();
}
void *stack_alloc(uint64_t *sp, size_t  length){
    *sp -= length;
    //grow_stack(user_stack_vma, *sp);

    return (void*)*sp;
}

uint64_t stack_round(uint64_t *sp) {
    return (*sp = (((uint64_t) (*sp)) & ~15UL));
}

#define MAX_ENTRIES 10

void new_aux_entry(uint64_t *entries, uint64_t *num_entries, uint64_t id, uint64_t value){
    if (*num_entries+2 > MAX_ENTRIES*2){
        printf("AUX entry table is full.\n");
        return;
    }
    entries[(*num_entries)++] = id;
    entries[(*num_entries)++] = value;
}

void kernel_main(void *kernel_entry, uint64_t _kernel_stack_max, uint64_t _kernel_stack, uint64_t _tss_start, char *user_binary_location) {

    uint64_t esp;

    kernel_min_address = _tss_start;
    kernel_stack = _kernel_stack_max;
    tss = (tss_entry_t*)_tss_start;

    //TODO - add some assert mechanism to see if enough space (will this be needed?)

    uint64_t gdt_page = kernel_min_address - PAGE_SIZE;
    map_physical_pages(gdt_page, 0x1000, PDE64_WRITEABLE, 1, true, 0);
    kernel_min_address = gdt_page;

    uint64_t *test = (uint64_t*)gdt_page;

    printf("Here %p\n", *test);

    gdt_init(gdt_page);

    unmap_physical_page(0x0000, 0);
    unmap_physical_page(0x1000, 0);

    cpu_init();
    tss_init(kernel_stack);
    idt_init(true);
    syscall_init();

    vma_init(1000);
    //gdt_load(0xDEADB000);

    kernel_min_address = user_version_start = P2ALIGN(kernel_min_address, 2*PAGE_SIZE) / 2;

    user_stack_init(user_version_start, 16000);
    esp = user_stack_start;

    printf("\n-------------------\nKERNEL START\n-------------------\n\n");

    xed_register_abort_function(&xed_user_abort_function, 0);
    xed_tables_init();

    printf("Kernel stack: %p\n", kernel_stack);
    printf("Version store start: %p\n", user_version_start);
    printf("User stack start: %p\n", user_stack_start);
    printf("User stack min: %p\n", user_stack_min);
    printf("User binary location: %s\n", user_binary_location);

    elf_info_t user_elf_info;
    void *elf_entry;
    int user_bin_fd = read_binary(user_binary_location);
    load_elf_binary(user_bin_fd, &elf_entry, &user_elf_info, false, 0);
    host_close(user_bin_fd);
    user_heap_start = user_elf_info.max_page_addr;
    vma_print();

    //load sym tables and dwarf information (if enabled?) - use heap that only works at this boot stage
    //the file can be loaded into memory at zero?
    //discard the file
    //symbol/dwarf heap can then be closed - updating kernel min page addr below heap, and close heap
    //then load binary below this (will be version memory) - can also assert that user address space is big enough (will likely be)
    //load elf sections - this will be below version area so all should be good
    //load dynlibs etc

    //user

    //TODO - random bytes for userspace PRNG seeding.
    char *str_tst = stack_alloc(&esp, 5);

    uint64_t num_entries = 0;
    uint64_t entries[MAX_ENTRIES*2];

    new_aux_entry(entries, &num_entries, AT_ENTRY, (uint64_t)user_elf_info.entry_addr);
    new_aux_entry(entries, &num_entries, AT_FLAGS, 0);
    new_aux_entry(entries, &num_entries, AT_BASE, (uint64_t)elf_entry);
    new_aux_entry(entries, &num_entries, AT_PHNUM, user_elf_info.phdr_num);
    new_aux_entry(entries, &num_entries, AT_PHENT, sizeof(elf64_phdr_t));
    new_aux_entry(entries, &num_entries, AT_PHDR, (uint64_t)user_elf_info.load_addr + user_elf_info.phdr_off);
    new_aux_entry(entries, &num_entries, AT_PAGESZ, PAGE_SIZE);
    //new_aux_entry(entries, &num_entries, AT_HWCAP, 0);
    //new_aux_entry(entries, &num_entries, AT_HWCAP2, 0);

    printf("d %p %p %d\n", elf_entry, user_elf_info.entry_addr, user_elf_info.phdr_num);


    //then working down stack
    //envp 0..n
    //envc
    //argv 0..n
    //argc

    size_t argc = 0, envc = 0, items;
    uint64_t *usp;

    stack_alloc(&esp, (num_entries + 2) * sizeof (uint64_t));

    items = ((argc + 1) + (envc + 1) + 1) * sizeof (uint64_t);
    stack_alloc(&esp, items);
    stack_round(&esp);

    grow_stack(user_stack_vma, esp);

    usp = (uint64_t *)esp;

    //argc onto top of aligned stack
    *(usp++) = argc;

    //write argv
    while (argc-- > 0) {
        //pass pointer in here
        *(usp++) = (uint64_t)str_tst;
    }

    //null 64-bit before envp
    *(usp++) = 0;

    //write envp
    while (envc-- > 0) {
        //pass pointer in here
        *(usp++) = (uint64_t)str_tst;
    }

    //null 64-bit after envp
    *(usp++) = 0;

    for (size_t i = 0; i < num_entries; i++){
        *(usp++) = entries[i];
    }

    //null aux entry
    memset(usp, 0, 2 * sizeof (uint64_t));

    /*char *yo = "yo";
    printf("strtst %p %p %s\n", str_tst, esp, yo);
    memcpy(str_tst, yo, strlen(yo)+1);*/


    /*char *args = (char*)stack_alloc(&usp, 4);
    args[0] = 0;
    args[1] = 0;
    args[2] = 0;
    args[3] = 0;*/

    printf("\n-------------------\nINITIALISED USERLAND\n-------------------\n\n");

    printf("User entry %p, stack %p\n", elf_entry, esp);
    disassemble_address((uint64_t)elf_entry, 5);

    printf("\n-------------------\nENTERING USERLAND\n-------------------\n\n");

    switch_usermode(elf_entry, esp);
}
