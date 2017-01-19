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

void new_aux_entry(uint64_t *sp, uint64_t *entries, uint64_t id, uint64_t value){
    uint64_t *entry = (uint64_t*)stack_alloc(sp, 2*sizeof (uint64_t));
    entry[0] = id;
    entry[1] = value;

    *entries += 2;
}

void kernel_main(void *kernel_entry, uint64_t _kernel_stack_max, uint64_t _kernel_stack, uint64_t _tss_start, char *user_binary_location) {

    uint64_t usp;

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
    usp = user_stack_start;

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
    load_elf_binary(user_bin_fd, &elf_entry, &user_elf_info, true, 0);
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
    stack_alloc(&usp, 16);

    uint64_t num_entries = 0;

    new_aux_entry(&usp, &num_entries, AT_NULL, 0);
    new_aux_entry(&usp, &num_entries, AT_ENTRY, 0xDEADBEEF);
    new_aux_entry(&usp, &num_entries, AT_PHDR, 0xDEADBEEF);
    new_aux_entry(&usp, &num_entries, AT_PHENT, 0xDEADBEEF);
    new_aux_entry(&usp, &num_entries, AT_PHNUM, 0xDEADBEEF);
    new_aux_entry(&usp, &num_entries, AT_BASE, 0xDEADBEEF);
    new_aux_entry(&usp, &num_entries, AT_PAGESZ, PAGE_SIZE);

    //then working down stack
    //envp 0..n
    //envc
    //argv 0..n
    //argc

    char *args = (char*)stack_alloc(&usp, 2);
    args[0] = 0;
    args[1] = 0;

    printf("\n-------------------\nINITIALISED USERLAND\n-------------------\n\n");

    printf("User entry %p, stack %p\n", elf_entry, usp);
    disassemble_address((uint64_t)elf_entry, 5);

    printf("\n-------------------\nENTERING USERLAND\n-------------------\n\n");

    switch_usermode(elf_entry, usp);
}
