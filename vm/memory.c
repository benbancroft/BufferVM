//
// Created by ben on 09/10/16.
//

#include <stdint.h>
#include <stdio.h>
#include <memory.h>

#include "memory.h"

uint32_t pd_addr = 0xFFEFF000;
uint32_t pt_start_addr = 0xFFB00000;
uint32_t page_counter = 0;

pageinfo mm_virtaddrtopageindex(unsigned long addr){
    pageinfo pginf;

    pginf.offset = addr & 0x00000FFF;
    //align address to 4k (highest 20-bits of address)
    addr &= ~0xFFF;
    pginf.pagetable = addr / 0x400000; // each page table covers 0x400000 bytes in memory
    pginf.page = (addr % 0x400000) / 0x1000; //0x1000 = page size
    return pginf;
}

int32_t get_physaddr(uint32_t virtualaddr, struct vm_t *vm)
{
    pageinfo pginf = mm_virtaddrtopageindex(virtualaddr); // get the PDE and PTE indexes for the addr

    uint32_t *pd = (void *)(vm->mem + pd_addr);
    uint32_t *pt = (void *)(vm->mem + pt_start_addr);

    if(pd[pginf.pagetable] & 1){
        // page table exists.
        uint32_t *page_physical = (void *)(vm->mem + (pd[pginf.pagetable] & 0xFFFFF000));

        return (page_physical[pginf.page] & 0xFFFFF000) + pginf.offset;
    }

    return 0;
}

/**
start_addr should be page aligned
*/
int load_address_space(uint64_t start_addr, size_t mem_size, char *elf_seg_start, size_t elf_seg_size, int flags, struct vm_t *vm) {

    for (uint32_t i = start_addr; i < start_addr + mem_size; i += 0x1000) {

        uint32_t physical_addr = 0x10000 + page_counter++ * 0x1000;

        memset(vm->mem + physical_addr, 0x0, mem_size);
        memcpy(vm->mem + physical_addr, elf_seg_start, elf_seg_size);

        map_address_space(i, physical_addr, vm);

    }


    return 0;
}

int map_address_space(uint64_t start_addr, uint64_t physical_addr, struct vm_t *vm) {

    uint32_t *pd = (void *)(vm->mem + pd_addr);
    uint32_t *pt = (void *)(vm->mem + pt_start_addr);

    pageinfo pginf = mm_virtaddrtopageindex(start_addr); // get the PDE and PTE indexes for the addr

    printf("%d\n", pginf.offset);

    if(pd[pginf.pagetable] & 1){
        // page table exists.
        uint32_t *page_table = pt + (pginf.pagetable * 0x1000);
        if(!page_table[pginf.page] & 1){
            // page isn't mapped
            page_table[pginf.page] = physical_addr & 0xFFFFF000 | 3;
        }else{
            // page is already mapped
            //return status_error;
            //TODO
        }
    }else{
        // doesn't exist, so alloc a page and add into pdir
        uint32_t new_page_table = 0x10000 + page_counter++ * 0x1000;
        uint32_t *page_table = pt + (pginf.pagetable * 0x1000);
        //uint32_t *page_table = pt + (pginf.pagetable * 0x1000); // virt addr of page tbl

        pd[pginf.pagetable] = (uint32_t) new_page_table | 3; // add the new page table into the pdir

        page_table[pginf.page] = (uint32_t)physical_addr | 3; // map the page!
    }


    return 0;
}