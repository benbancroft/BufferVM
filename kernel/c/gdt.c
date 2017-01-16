//
// Created by ben on 17/10/16.
//

#include "../h/gdt.h"
#include "../h/tss.h"
#include "../../common/paging.h"

void gdt_init(uint64_t gdt_page){
    gdt.limit = PAGE_SIZE - 1;
    gdt.base = (void *)gdt_page;
    gdt_load((uint64_t)&gdt);
}