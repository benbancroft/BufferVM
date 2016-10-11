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
#include <memory.h>

#include "vm.h"


typedef struct __attribute__ ((packed)){
    uint32_t version;
    uint32_t pg_dir_ptr_offset;
    uint32_t pg_dir_offset;
    uint32_t pg_tbl_offset;
    uint32_t phys_offset;
}pointer_info, *p_pointer_info;

typedef struct {
    uint64_t pml4e;
    uint64_t pdpe;
    uint64_t pde;
    uint64_t pte;
} page_table_info_t;

extern uint32_t pd_addr;
extern uint32_t pt_start_addr;
extern uint32_t page_counter;
extern page_table_info_t page_table_info;

pointer_info mm_virtaddrtopageindex(unsigned long addr);

int32_t get_physaddr(uint32_t virtualaddr, struct vm_t *vm);

page_table_info_t build_page_tables(struct vm_t *vm);

void *map_page_entry(uint64_t *table, size_t index, uint64_t address, uint64_t flags);

/**
start_addr should be page aligned
*/
int load_address_space(uint64_t start_addr, size_t mem_size, char *elf_seg_start, size_t elf_seg_size, int flags, struct vm_t *vm);

//int map_address_space(uint64_t virtual_addr, uint64_t physical_addr, struct vm_t *vm);

#endif //BUFFERVM_MEMORY_H
