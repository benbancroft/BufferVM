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

#define PAGE_SIZE 0x1000

#define P2ALIGN(x, align)    ((x) & -(align))
#define P2ROUNDUP(x, align) (-(-(x) & -(align)))

typedef struct {
    uint64_t version;
    uint64_t pg_dir_ptr_offset;
    uint64_t pg_dir_offset;
    uint64_t pg_tbl_offset;
    uint64_t phys_offset;
} virt_addr_info_t;

extern uint32_t pd_addr;
extern uint32_t pt_start_addr;
extern uint64_t page_counter;
extern uint64_t pml4_addr;

int read_virtual_addr(uint64_t virtual_addr, size_t size, void *buffer, struct vm_t *vm);
virt_addr_info_t get_virt_addr_info(uint64_t addr);

int get_phys_addr(uint64_t virtual_addr, uint64_t *phys_addr, struct vm_t *vm);
int get_page_entry(uint64_t *table, size_t index, uint64_t *page, struct vm_t *vm);

void build_page_tables(struct vm_t *vm);

uint64_t allocate_page(struct vm_t *vm, bool zero_page);

uint64_t map_page_entry(uint64_t *table, size_t index, uint64_t flags, int64_t page, struct vm_t *vm);

void load_address_space(uint64_t start_addr, size_t mem_size, char *elf_seg_start, size_t elf_seg_size, int flags, struct vm_t *vm);

void map_physical_page(uint64_t virtual_page_addr, uint64_t physical_page_addr, uint64_t flags, size_t num_pages, struct vm_t *vm);

#endif //BUFFERVM_MEMORY_H
