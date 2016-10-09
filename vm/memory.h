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

extern uint32_t pd_addr;
extern uint32_t pt_start_addr;
extern uint32_t page_counter;

typedef struct __attribute__ ((packed)){
    uint32_t pagetable;
    uint32_t page;
    uint32_t offset;
}pageinfo, *ppageinfo;

pageinfo mm_virtaddrtopageindex(unsigned long addr);

int32_t get_physaddr(uint32_t virtualaddr, struct vm_t *vm);

/**
start_addr should be page aligned
*/
int load_address_space(uint64_t start_addr, size_t mem_size, char *elf_seg_start, size_t elf_seg_size, int flags, struct vm_t *vm);

int map_address_space(uint64_t start_addr, uint64_t physical_addr, struct vm_t *vm);

#endif //BUFFERVM_MEMORY_H
