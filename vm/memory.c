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
page_table_info_t page_table_info;

pointer_info mm_virtaddrtopageindex(uint64_t addr){
    pointer_info pginf;

    pginf.phys_offset = addr & 0x0000000000000FFF;
    pginf.pg_tbl_offset = (addr & 0x00000000001FF000) >> 12;
    pginf.pg_dir_offset = (addr & 0x000000003fe00000) >> 21;
    pginf.pg_dir_ptr_offset = (addr & 0x0000007fc0000000) >> 30;
    pginf.version = (addr & 0x0000ff8000000000) >> 39;

    return pginf;
}

int32_t get_physaddr(uint32_t virtualaddr, struct vm_t *vm)
{
    /*pageinfo pginf = mm_virtaddrtopageindex(virtualaddr); // get the PDE and PTE indexes for the addr

    uint32_t *pd = (void *)(vm->mem + pd_addr);
    uint32_t *pt = (void *)(vm->mem + pt_start_addr);

    if(pd[pginf.pagetable] & 1){
        // page table exists.
        uint32_t *page_physical = (void *)(vm->mem + (pd[pginf.pagetable] & 0xFFFFF000));

        return (page_physical[pginf.page] & 0xFFFFF000) + pginf.offset;
    }*/

    return 0;
}

/**
start_addr should be page aligned
*/
/*int load_address_space(uint64_t start_addr, size_t mem_size, char *elf_seg_start, size_t elf_seg_size, int flags, struct vm_t *vm) {

    for (uint32_t i = start_addr; i < start_addr + mem_size; i += 0x1000) {

        uint32_t physical_addr = 0x10000 + page_counter++ * 0x1000;

        memset(vm->mem + physical_addr, 0x0, mem_size);
        memcpy(vm->mem + physical_addr, elf_seg_start, elf_seg_size);

        map_address_space(i, physical_addr, vm);

    }


    return 0;
}*/

//TODO - make this better
uint32_t allocate_page(){
    return page_counter++;
}

page_table_info_t build_page_tables(struct vm_t *vm){
    page_table_info.pml4e = (allocate_page()*0x1000);
    page_table_info.pdpe = (allocate_page()*0x1000);
    page_table_info.pde = (allocate_page()*0x1000);
    page_table_info.pte = (allocate_page()*0x1000);

    //TODO - map self
    for (size_t i = 0; i < 512; i++)
        map_page_entry((uint64_t *)vm->mem + page_table_info.pdpe, i, page_table_info.pde, 0);
        //map_page_entry((uint64_t *)vm->mem + page_table_info.pml4e, i, page_table_info.pdpe, 0);

    //map_page_entry((uint64_t *)vm->mem + page_table_info.pdpe, 0, page_table_info.pde, 0);
    //map_page_entry((uint64_t *)vm->mem + page_table_info.pde, 0, page_table_info.pte, 0);
    for (size_t i = 0; i < 512; i++)
        map_page_entry((uint64_t *)vm->mem + page_table_info.pde, i, page_table_info.pte, 0);

    for (size_t i = 0; i < 512; i++)
        map_page_entry((uint64_t *)vm->mem + page_table_info.pte, i, allocate_page()*0x1000, 0);
}

void *map_page_entry(uint64_t *table, size_t index, uint64_t address, uint64_t flags){
    table[index] = address | PDE64_PRESENT | PDE64_RW | PDE64_USER;
}

/*int map_address_space(uint64_t virtual_addr, uint64_t physical_addr, struct vm_t *vm) {

    uint32_t *pd = (void *)(vm->mem + pd_addr);
    uint32_t *pt = (void *)(vm->mem + pt_start_addr);

    pointer_info pginf = mm_virtaddrtopageindex(virtual_addr); // get the PDE and PTE indexes for the addr
    pginf.version = 0;

    map_page_entry((uint64_t *)vm->mem + (uint64_t)page_table_info.pdpe, pginf.version, 0);

    //printf("%d\n", pginf.offset);

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
        uint32_t *page_table = vm->mem + new_page_table;
        //uint32_t *page_table = pt + (pginf.pagetable * 0x1000); // virt addr of page tbl

        pd[pginf.pagetable] = (uint32_t) new_page_table | 3; // add the new page table into the pdir

        page_table[pginf.page] = (uint32_t)physical_addr | 3; // map the page!
    }


    return 0;
}*/