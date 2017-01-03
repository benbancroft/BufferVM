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

uint64_t kernel_min_address;

tss_entry_t *tss;

uint64_t kernel_stack;
uint64_t user_heap_start;
uint64_t user_version_start;

void xed_user_abort_function(const char *msg, const char *file, int line, void *other){
    printf("abort thing needs writing!\n");
    host_exit();
}

void kernel_main(void *kernel_entry, void *user_entry, uint64_t _kernel_stack, uint64_t _user_heap, uint64_t _tss_start) {

    kernel_min_address = _tss_start;

    kernel_stack = _kernel_stack;
    user_heap_start = _user_heap;

    tss = (tss_entry_t*)_tss_start;

    //TODO - add some assert mechanism to see if enough space (will this be needed?)


    cpu_init();
    tss_init(kernel_stack);
    idt_init(true);
    syscall_init();

    vma_init(1000);

    kernel_min_address = user_version_start = P2ALIGN(kernel_min_address, 2*PAGE_SIZE) / 2;

    user_stack_init(user_version_start, 16000);

    printf("\n-------------------\nKERNEL START\n-------------------\n\n");

    xed_register_abort_function(&xed_user_abort_function, 0);
    xed_tables_init();

    printf("Kernel stack: %p\n", kernel_stack);
    printf("Version store start: %p\n", user_version_start);
    printf("User stack start: %p\n", user_stack_start);
    printf("User stack min: %p\n", user_stack_min);

    printf("\n-------------------\nENTERING USERLAND\n-------------------\n\n");

    switch_usermode(user_entry);

    //int a = 5;
    //printf("Div: %d\n", a / 0);
    //asm volatile ("int $14");

}
