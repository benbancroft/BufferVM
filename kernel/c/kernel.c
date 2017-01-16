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

void kernel_main(void *kernel_entry, uint64_t _kernel_stack_max, uint64_t _kernel_stack, uint64_t _tss_start, char *user_binary_location) {

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

    /*unmap_physical_page(0x0000, 0);
    unmap_physical_page(0x1000, 0);*/

    cpu_init();
    tss_init(kernel_stack);
    idt_init(true);
    syscall_init();

    vma_init(1000);
    //gdt_load(0xDEADB000);

    kernel_min_address = user_version_start = P2ALIGN(kernel_min_address, 2*PAGE_SIZE) / 2;

    user_stack_init(user_version_start, 16000);

    printf("\n-------------------\nKERNEL START\n-------------------\n\n");

    xed_register_abort_function(&xed_user_abort_function, 0);
    xed_tables_init();

    printf("Kernel stack: %p\n", kernel_stack);
    printf("Version store start: %p\n", user_version_start);
    printf("User stack start: %p\n", user_stack_start);
    printf("User stack min: %p\n", user_stack_min);
    printf("User binary location: %s\n", user_binary_location);

    elf_info_t user_elf_info;
    int user_bin_fd = read_binary(user_binary_location);
    load_elf_binary(user_bin_fd, &user_elf_info, true, 0);
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

    printf("\n-------------------\nENTERING USERLAND\n-------------------\n\n");

    //*((char*)0x400938) = 0xF4;
    printf("User entry %p\n", user_elf_info.entry_addr);
    disassemble_address((uint64_t)user_elf_info.entry_addr, 5);

    switch_usermode(user_elf_info.entry_addr);
}
