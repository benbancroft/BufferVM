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

tss_entry_t *tss = (tss_entry_t*) TSS_START;

uint64_t kernel_stack;
uint64_t user_stack;
uint64_t user_heap_start;

void kernel_main(void *kernel_entry, void *user_entry, uint64_t _kernel_stack, uint64_t _user_stack, uint64_t _user_heap) {

    kernel_stack = _kernel_stack;
    user_stack = _user_stack;
    user_heap_start = _user_heap;

    cpu_init();
    tss_init(kernel_stack);

    printf("kernel stack %p\n", _kernel_stack);
    host_print_var(_kernel_stack);

    idt_init(true);
    printf("Unsigned sizes: %d %d %d %d\n", sizeof (uint8_t), sizeof (uint16_t), sizeof (uint32_t), sizeof (uint64_t));
    printf("Signed sizes: %d %d %d %d\n", sizeof (int8_t), sizeof (int16_t), sizeof (int32_t), sizeof (int64_t));

    syscall_init();

    switch_usermode(user_entry);

    //int a = 5;
    //printf("Div: %d\n", a / 0);
    //asm volatile ("int $14");

}
