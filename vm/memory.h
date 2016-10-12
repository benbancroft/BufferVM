//
// Created by ben on 09/10/16.
//

#ifndef BUFFERVM_MEMORY_H
#define BUFFERVM_MEMORY_H

//
// Created by ben on 09/10/16.
//

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <memory.h>

#include "vm.h"


typedef struct {
    uint32_t version;
    uint32_t pg_dir_ptr_offset;
    uint32_t pg_dir_offset;
    uint32_t pg_tbl_offset;
    uint32_t phys_offset;
} virt_addr_info_t;

extern uint32_t pd_addr;
extern uint32_t pt_start_addr;
extern uint64_t page_counter;
extern uint64_t pml4_addr;

virt_addr_info_t get_virt_addr_info(uint64_t addr);

int32_t get_physaddr(uint32_t virtualaddr, struct vm_t *vm);

void build_page_tables(struct vm_t *vm);

uint64_t allocate_page(struct vm_t *vm, bool zero_page);

uint64_t map_page_entry(uint64_t *table, size_t index, uint64_t flags, int64_t page, struct vm_t *vm);

void load_address_space(uint64_t start_addr, size_t mem_size, char *elf_seg_start, size_t elf_seg_size, int flags, struct vm_t *vm);

void map_physical_page(uint64_t virtual_page_addr, uint64_t physical_page_addr, uint64_t flags, size_t num_pages, struct vm_t *vm);

#endif //BUFFERVM_MEMORY_H
