//
// Created by ben on 17/10/16.
//

#include "../h/tss.h"

void tss_init(uintptr_t kernel_stack){
    tss_load();
    memset((void *) tss, 0, sizeof(tss_entry_t));
    tss->rsp0 = kernel_stack;
}