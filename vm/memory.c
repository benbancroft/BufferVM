//
// Created by ben on 09/10/16.
//

#include <stdint.h>
#include <stdio.h>
#include <memory.h>

#include "memory.h"

uint32_t pd_addr = 0xFFEFF000;
uint32_t pt_start_addr = 0xFFB00000;
//reserve first few pages for bootloader
uint64_t page_counter = 5;
uint64_t pml4_addr;

virt_addr_info_t get_virt_addr_info(uint64_t addr){
    virt_addr_info_t pginf;

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
uint64_t allocate_page(struct vm_t *vm, bool zero_page){
    uint64_t new_page = (page_counter++)*0x1000;

    if (zero_page)
        for (size_t i = 0; i < 0x1000; i++)
                vm->mem[new_page+i] = 0;


    return new_page;
}

void build_page_tables(struct vm_t *vm){
    pml4_addr = allocate_page(vm, true);
    uint64_t *pml4 = (void *)(vm->mem + pml4_addr);

    uint64_t pdpt_addr = allocate_page(vm, true);
    uint64_t *pdpt = (void *)(vm->mem + pdpt_addr);

    for (size_t i = 0; i < 512; i++)
        pml4[i] = pdpt_addr | PDE64_PRESENT | PDE64_RW | PDE64_USER;
}

uint64_t map_page_entry(uint64_t *table, size_t index, uint64_t flags, int64_t page, struct vm_t *vm){

    if (page == PAGE_CREATE) {
        if (!(table[index] & PDE64_PRESENT)) {
            table[index] = PDE64_PRESENT | PDE64_RW | PDE64_USER | flags | allocate_page(vm, true);
        }
    }else
        table[index] = PDE64_PRESENT | PDE64_RW | PDE64_USER | flags | (page & 0xFFFFFFFFFFF000);

    return table[index] & 0xFFFFFFFFFFF000;
}

void map_physical_page(uint64_t virtual_page_addr, uint64_t physical_page_addr, size_t num_pages, struct vm_t *vm) {

    //TODO - num_pages: painful algorithm needed. Could also look at option to use bigger pages for large allocation?

    virt_addr_info_t info = get_virt_addr_info(virtual_page_addr);

    uint64_t *pml4 = (void *)(vm->mem + pml4_addr);

    uint64_t pdpt_addr = map_page_entry(pml4, 0, 0, PAGE_CREATE, vm);
    uint64_t *pdpt = (void *)(vm->mem + pdpt_addr);

    uint64_t pd_addr = map_page_entry(pdpt, info.pg_dir_ptr_offset, 0, PAGE_CREATE, vm);
    uint64_t *pd = (void *)(vm->mem + pd_addr);

    uint64_t pd2_addr = map_page_entry(pd, info.pg_dir_offset, 0, PAGE_CREATE, vm);
    uint64_t *pd2 = (void *)(vm->mem + pd2_addr);

    map_page_entry(pd2, info.pg_tbl_offset, 0, physical_page_addr, vm);
}