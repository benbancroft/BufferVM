//
// Created by ben on 13/10/16.
//

#include "../h/cpu.h"
#include "../h/idt.h"
#include "../h/tss.h"
#include "../h/kernel.h"
#include "../h/syscall.h"
#include "../../common/paging.h"

tss_entry_t *tss = (tss_entry_t*) TSS_START;

uint64_t kernel_stack;
uint64_t user_stack;
uint64_t user_max = 0;

uint32_t step_counter = 0;
uint64_t idt_stack = 0;

void kernel_main(void *kernel_entry, void *user_entry, uint64_t _kernel_stack, uint64_t _user_stack, uint64_t _user_max) {

    kernel_stack = _kernel_stack;
    user_stack = _user_stack;
    user_max = _user_max;

    cpu_init();
    tss_init(kernel_stack);

    idt_init(true);
    printf("Unsigned sizes: %d %d %d %d\n", sizeof (uint8_t), sizeof (uint16_t), sizeof (uint32_t), sizeof (uint64_t));
    printf("Signed sizes: %d %d %d %d\n", sizeof (int8_t), sizeof (int16_t), sizeof (int32_t), sizeof (int64_t));

    syscall_init();

    switch_usermode(user_entry);

    //int a = 5;
    //printf("Div: %d\n", a / 0);
    //asm volatile ("int $14");

}
