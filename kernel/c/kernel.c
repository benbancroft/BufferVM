//
// Created by ben on 13/10/16.
//

#include "../h/idt.h"
#include "../h/tss.h"
#include "../h/kernel.h"
#include "../h/syscall.h"

tss_entry_t *tss = (tss_entry_t*) TSS_START;

uint64_t kernel_stack;
uint64_t user_stack;

void kernel_main(uintptr_t kernel_entry, uintptr_t user_entry, uint64_t _kernel_stack, uint64_t _user_stack) {

    kernel_stack = _kernel_stack;
    user_stack = _user_stack;

    tss_init(kernel_stack);

    idt_init(true);
    printf("Unsigned sizes: %d %d %d %d\n", sizeof (uint8_t), sizeof (uint16_t), sizeof (uint32_t), sizeof (uint64_t));
    printf("Signed sizes: %d %d %d %d\n", sizeof (int8_t), sizeof (int16_t), sizeof (int32_t), sizeof (int64_t));

    syscall_init();

    //int a = 5;
    //printf("Div: %d\n", a / 0);
    //asm volatile ("int $14");

}
