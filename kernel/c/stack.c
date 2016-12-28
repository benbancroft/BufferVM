//
// Created by ben on 27/12/16.
//

#include "../h/stack.h"
#include "../../common/paging.h"

/**
 * Stack start address
 */
uint64_t user_stack_start;
/**
 * Current stack page address
 */
uint64_t user_stack_page;
/**
 * Stack min address. mmap VMAs sits below here.
 */
uint64_t user_stack_min;

void user_stack_init(uint64_t _user_stack_start, const uint64_t stack_page_limit){
    user_stack_page = user_stack_start = _user_stack_start;

    user_stack_min = user_stack_start - stack_page_limit * PAGE_SIZE;
}

int grow_stack(uint64_t addr){

    uint64_t new_start = PAGE_ALIGN(addr) - PAGE_SIZE;

    for (uint64_t i = new_start; i < user_stack_page; i += PAGE_SIZE){
        map_physical_page(i, -1, PDE64_NO_EXE | PDE64_WRITEABLE | PDE64_USER, 1, 0);
        //printf("Added stack page %p\n", i);
    }

    user_stack_page = new_start;

    //printf("grow stack\n");
    return 1;
}