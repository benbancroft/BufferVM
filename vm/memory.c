//
// Created by ben on 09/10/16.
//

#include <stdint.h>
#include <stdio.h>
#include <memory.h>
#include <assert.h>

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

int get_phys_addr(uint64_t virtual_addr, uint64_t *phys_addr, struct vm_t *vm)
{
    uint64_t page_addr;

    virt_addr_info_t info = get_virt_addr_info(virtual_addr);

    uint64_t *pml4 = (void *)(vm->mem + pml4_addr);

    if (!get_page_entry(pml4, 0, &page_addr, vm))
        return 0;

    uint64_t *pdpt = (void *)(vm->mem + page_addr);
    if (!get_page_entry(pdpt, info.pg_dir_ptr_offset, &page_addr, vm))
        return 0;

    uint64_t *pd = (void *)(vm->mem + page_addr);
    if (!get_page_entry(pd, info.pg_dir_offset, &page_addr, vm))
        return 0;

    uint64_t *pd2 = (void *)(vm->mem + page_addr);
    if (!get_page_entry(pd2, info.pg_tbl_offset, &page_addr, vm))
        return 0;

    *phys_addr = page_addr + info.phys_offset;

    return 1;
}

/**
start_addr should be page aligned
*/
void load_address_space(uint64_t start_addr, size_t mem_size, char *elf_seg_start, size_t elf_seg_size, int flags, struct vm_t *vm) {

    size_t size_left_mem = mem_size;
    size_t size_left_elf = elf_seg_size;

    for (uint64_t p = start_addr, i = 0; p < start_addr + mem_size; p += 0x1000, i += 0x1000) {

        uint64_t phy_addr = allocate_page(vm, false);

        memset(vm->mem + phy_addr, 0x0, size_left_mem < 0x1000 ? mem_size : 0x1000);
        memcpy(vm->mem + phy_addr, elf_seg_start+i, size_left_elf < 0x1000 ? size_left_elf : 0x1000);

        printf("Loaded page: %p %p, %#04hhx\n", p, phy_addr, vm->mem[phy_addr+0xc0a]);
        map_physical_page(p, phy_addr, flags, 1, vm);

        size_left_mem -= 0x1000;
        size_left_elf -= 0x1000;
    }
}

//TODO - make this better
uint64_t allocate_page(struct vm_t *vm, bool zero_page){
    uint64_t new_page = (page_counter++)*0x1000;

    if (zero_page)
        memset(vm->mem + new_page, 0x0, 0x1000);


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

int get_page_entry(uint64_t *table, size_t index, uint64_t *page, struct vm_t *vm) {

    if (table[index] & PDE64_PRESENT) {
        *page = table[index] & 0xFFFFFFFFFFF000;
        return 1;
    } else
        return 0;
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

void map_physical_page(uint64_t virtual_page_addr, uint64_t physical_page_addr, uint64_t flags, size_t num_pages, struct vm_t *vm) {

    //TODO - num_pages: painful algorithm needed. Could also look at option to use bigger pages for large allocation?

    virt_addr_info_t info = get_virt_addr_info(virtual_page_addr);

    uint64_t *pml4 = (void *)(vm->mem + pml4_addr);

    uint64_t pdpt_addr = map_page_entry(pml4, 0, 0, PAGE_CREATE, vm);
    uint64_t *pdpt = (void *)(vm->mem + pdpt_addr);

    uint64_t pd_addr = map_page_entry(pdpt, info.pg_dir_ptr_offset, 0, PAGE_CREATE, vm);
    uint64_t *pd = (void *)(vm->mem + pd_addr);

    uint64_t pd2_addr = map_page_entry(pd, info.pg_dir_offset, 0, PAGE_CREATE, vm);
    uint64_t *pd2 = (void *)(vm->mem + pd2_addr);

    map_page_entry(pd2, info.pg_tbl_offset, flags, physical_page_addr, vm);
}