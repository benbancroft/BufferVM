//
// Created by ben on 17/10/16.
//

#include "../h/tss.h"
#include "../h/host.h"

void tss_init(uint64_t kernel_stack){
    tss_load();
    memset((void *) tss, 0, sizeof(tss_entry_t));
    host_print_var((uint64_t)tss);
    tss->rsp0 = kernel_stack;
}