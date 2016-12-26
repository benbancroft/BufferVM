//
// Created by ben on 09/10/16.
//

#ifndef BUFFERVM_PAGING_H
#define BUFFERVM_PAGING_H

//
// Created by ben on 09/10/16.
//

#include "stdlib_inc.h"

/* 64-bit page * entry bits */
#define PDE64_PRESENT 1
#define PDE64_WRITEABLE (1 << 1)
#define PDE64_NO_EXE (UINT64_C(1) << 63)
#define PDE64_USER (1 << 2)
#define PDE64_WRITE_MEM (1 << 3)
#define PDE64_DISABLE_CACHE (1 << 4)
#define PDE64_ACCESSED (1 << 5)
#define PDE64_DIRTY (1 << 6)
#define PDE64_PS (1 << 7)
#define PDE64_G (1 << 8)

#define PAGE_CREATE -1

#define PAGE_SIZE 0x1000UL

#define P2ALIGN(x, align)    ((x) & -(align))
#define P2ROUNDUP(x, align) (-(-(x) & -(align)))

#define PAGE_ALIGN(addr) P2ROUNDUP(addr, PAGE_SIZE)

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

int read_virtual_cstr(uint64_t virtual_addr, char **buffer, char *mem_offset);
int read_virtual_addr(uint64_t virtual_addr, size_t size, void *buffer, char *mem_offset);
uint64_t un_sign_extend(uint64_t addr);
virt_addr_info_t get_virt_addr_info(uint64_t addr);

int is_vpage_present(uint64_t virtual_addr, char *mem_offset);
int get_phys_addr(uint64_t virtual_addr, uint64_t *phys_addr, char *mem_offset);
int get_page_entry(uint64_t *table, size_t index, uint64_t *page);

void build_page_tables(char *mem_offset);

uint64_t allocate_page(char *mem_offset, bool zero_page);

uint64_t map_page_entry(uint64_t *table, size_t index, uint64_t flags, int64_t page, char *mem_offset);

void load_address_space(uint64_t start_addr, size_t mem_size, char *elf_seg_start, size_t elf_seg_size, uint64_t flags, char *mem_offset);

void map_physical_page(uint64_t virtual_page_addr, uint64_t physical_page_addr, uint64_t flags, size_t num_pages, char *mem_offset);

#endif //BUFFERVM_PAGING_H
