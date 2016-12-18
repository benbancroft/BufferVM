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

tss_entry_t *tss = (tss_entry_t*) TSS_START;

uint64_t kernel_stack;
uint64_t user_stack;
uint64_t user_heap_start;
uint64_t user_version_start;

void xed_user_abort_function(const char *msg, const char *file, int line, void *other){
    printf("abort thing needs writing!\n");
    host_exit();
}

void kernel_main(void *kernel_entry, void *user_entry, uint64_t _kernel_stack, uint64_t _user_stack, uint64_t _user_heap) {

    kernel_stack = _kernel_stack;
    user_stack = _user_stack;
    user_heap_start = _user_heap;

    user_version_start = kernel_stack + USER_VERSION_REDZONE;
    //TODO - add some assert mechanism to see if enough space (will this be needed?)
    printf("version store start %p\n", user_version_start);

    cpu_init();
    tss_init(kernel_stack);

    printf("kernel stack %p\n", _kernel_stack);

    idt_init(true);
    printf("Unsigned sizes: %d %d %d %d\n", sizeof (uint8_t), sizeof (uint16_t), sizeof (uint32_t), sizeof (uint64_t));
    printf("Signed sizes: %d %d %d %d\n", sizeof (int8_t), sizeof (int16_t), sizeof (int32_t), sizeof (int64_t));

    syscall_init();

    xed_register_abort_function(&xed_user_abort_function, 0);
    xed_tables_init();

    switch_usermode(user_entry);

    //int a = 5;
    //printf("Div: %d\n", a / 0);
    //asm volatile ("int $14");

}
