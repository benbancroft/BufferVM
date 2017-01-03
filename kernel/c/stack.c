//
// Created by ben on 27/12/16.
//

#include "../h/stack.h"
#include "../../common/paging.h"
#include "../h/vma.h"
#include "../h/utils.h"
#include "../h/host.h"

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
/**
 * Stack page limit - max number of stack pages
 */
uint64_t user_stack_page_limit;

void map_stack_page(uint64_t addr){
    map_physical_page(addr, -1, PDE64_NO_EXE | PDE64_WRITEABLE | PDE64_USER, 1, false, 0);
    //printf("Added stack page %p\n", i);

}

void user_stack_init(uint64_t _user_stack_start, const uint64_t stack_page_limit){

    uint64_t addr;

    user_stack_start = _user_stack_start;

    //TODO - remove this and use VMA flags to do this in page fault from
    map_stack_page(user_stack_start - PAGE_SIZE);

    user_stack_page = _user_stack_start - PAGE_SIZE;

    user_stack_min = _user_stack_start - stack_page_limit * PAGE_SIZE;

    user_stack_page_limit = stack_page_limit;

    addr = mmap_region(NULL, user_stack_page, PAGE_SIZE, VM_GROWSDOWN, 0);

    ASSERT(addr == user_stack_page);
}

int grow_stack(vm_area_t *vma, uint64_t addr){

    uint64_t new_start = PAGE_ALIGN(addr) - PAGE_SIZE;

    if (PAGE_DIFFERENCE(user_stack_start, new_start) > user_stack_page_limit)
        return 1;

    for (uint64_t i = new_start; i < user_stack_page; i += PAGE_SIZE){
        map_stack_page(i);
    }

    vma->vm_start = new_start;
    user_stack_page = new_start;

    vma_gap_update(vma);

    //printf("grow stack\n");
    return 0;
}