//
// Created by ben on 09/10/16.
//

#include <stdlib.h>
#include "../common/paging.h"

uint32_t pd_addr = 0xFFEFF000;
uint32_t pt_start_addr = 0xFFB00000;
//reserve first few pages for bootstrap
uint64_t page_counter = 10;
uint64_t pml4_addr;

uint64_t kbit(uint64_t addr, size_t  k){
    return (addr >> k) & 1;
}

uint64_t un_sign_extend(uint64_t addr){
    if (kbit(addr, 63) == 0) return addr;

    size_t extension_bits = 0;
    for (int32_t i = 62; i >= 0; i--){
        if (kbit(addr, i) == 1) extension_bits++;
        else break;
    }
    //handle exception for how 32/16-bit pointers are zero extended
    if (extension_bits > 32)
        extension_bits = 32;

    return (addr & (0xFFFFFFFFFFFFFFFF >> extension_bits));
}

virt_addr_info_t get_virt_addr_info(uint64_t addr){
    virt_addr_info_t pginf;

    pginf.phys_offset = addr & 0x0000000000000FFF;
    pginf.pg_tbl_offset = (addr & 0x00000000001FF000) >> 12;
    pginf.pg_dir_offset = (addr & 0x000000003fe00000) >> 21;
    pginf.pg_dir_ptr_offset = (addr & 0x0000007fc0000000) >> 30;
    pginf.version = (addr & 0x0000ff8000000000) >> 39;

    return pginf;
}

int read_virtual_cstr(uint64_t virtual_addr, char **buffer, char *mem_offset){

    uint64_t phys_addr;
    size_t page_bytes;
    size_t bytes_read = 0;

    while (true){
        if (!get_phys_addr(virtual_addr + bytes_read, &phys_addr, mem_offset)) return 0;

        page_bytes = PAGE_SIZE - ((virtual_addr + bytes_read) % PAGE_SIZE);

        for (size_t i = 0; i < page_bytes; i++){
            if ((mem_offset+phys_addr)[bytes_read++] == 0){
                bytes_read++;
                *buffer = malloc(bytes_read);
                return read_virtual_addr(virtual_addr, bytes_read, *buffer, mem_offset);
            }
        }
    }
}

int read_virtual_addr(uint64_t virtual_addr, size_t size, void *buffer, char *mem_offset){

    uint64_t phys_addr;
    size_t page_bytes;
    size_t bytes_read = 0;
    size_t bytes_left = size;

    while (bytes_left > 0){
        if (!get_phys_addr(virtual_addr + bytes_read, &phys_addr, mem_offset)) return 0;

        page_bytes = PAGE_SIZE - ((virtual_addr + bytes_read) % PAGE_SIZE);
        if (page_bytes > bytes_left) page_bytes = bytes_left;

        memcpy(buffer + bytes_read, mem_offset + phys_addr, page_bytes);

        bytes_read += page_bytes;
        bytes_left -= page_bytes;
    }

    return 1;
}

int is_vpage_present(uint64_t virtual_addr, char *mem_offset){
    return get_phys_addr(virtual_addr, NULL, mem_offset);
}

int get_phys_addr(uint64_t virtual_addr, uint64_t *phys_addr, char *mem_offset)
{
    uint64_t page_addr;

    virt_addr_info_t info = get_virt_addr_info(virtual_addr);

    uint64_t *pml4 = (void *)(mem_offset + pml4_addr);

    if (!get_page_entry(pml4, 0, &page_addr))
        return 0;

    uint64_t *pdpt = (void *)(mem_offset + page_addr);
    if (!get_page_entry(pdpt, info.pg_dir_ptr_offset, &page_addr))
        return 0;

    uint64_t *pd = (void *)(mem_offset + page_addr);
    if (!get_page_entry(pd, info.pg_dir_offset, &page_addr))
        return 0;

    uint64_t *pd2 = (void *)(mem_offset + page_addr);
    if (!get_page_entry(pd2, info.pg_tbl_offset, &page_addr))
        return 0;

    if (phys_addr != NULL)
        *phys_addr = page_addr + info.phys_offset;

    return 1;
}

/**
start_addr should be page aligned
*/
void load_address_space(uint64_t start_addr, size_t mem_size, char *elf_seg_start, size_t elf_seg_size, uint64_t flags, char *mem_offset) {

    size_t size_left_mem = mem_size;
    size_t size_left_elf = elf_seg_size;

    for (uint64_t p = start_addr, i = 0; p < start_addr + mem_size; p += 0x1000, i += 0x1000) {

        uint64_t phy_addr = allocate_page(mem_offset, false);

        memset(mem_offset + phy_addr, 0x0, 0x1000);
        memcpy(mem_offset + phy_addr, elf_seg_start+i, size_left_elf < 0x1000 ? size_left_elf : 0x1000);

        //printf("Loaded page: %p %p, %#04hhx\n", (void*) p, (void*) phy_addr, mem_offset[phy_addr+0xc0a]);
        map_physical_page(p, phy_addr, flags, 1, mem_offset);

        if (start_addr == 0xc0000000)
            map_physical_page(0xffffffffc0000000, phy_addr, flags, 1, mem_offset);

        size_left_mem -= 0x1000;
        size_left_elf -= 0x1000;
    }
}

//TODO - make this better
uint64_t allocate_page(char *mem_offset, bool zero_page){
    uint64_t new_page = (page_counter++)*0x1000;

    if (zero_page)
        memset(mem_offset + new_page, 0x0, 0x1000);


    return new_page;
}

void build_page_tables(char *mem_offset){
    pml4_addr = allocate_page(mem_offset, true);
    uint64_t *pml4 = (void *)(mem_offset + pml4_addr);

    uint64_t pdpt_addr = allocate_page(mem_offset, true);

    for (size_t i = 0; i < 512; i++)
        pml4[i] = pdpt_addr | PDE64_PRESENT | PDE64_USER | PDE64_WRITEABLE;
}

int get_page_entry(uint64_t *table, size_t index, uint64_t *page) {

    if (table[index] & PDE64_PRESENT) {
        *page = table[index] & 0xFFFFFFFFFFF000;
        return 1;
    } else
        return 0;
}

uint64_t map_page_entry(uint64_t *table, size_t index, uint64_t flags, int64_t page, char *mem_offset){

    if (page == PAGE_CREATE) {
        if (!(table[index] & PDE64_PRESENT)) {
            table[index] = PDE64_PRESENT | flags | allocate_page(mem_offset, true);
        }else {
            table[index] |= flags;
        }
    }else
        table[index] = PDE64_PRESENT | flags | (page & 0xFFFFFFFFFFF000);

    return table[index] & 0xFFFFFFFFFFF000;
}

void map_physical_page(uint64_t virtual_page_addr, uint64_t physical_page_addr, uint64_t flags, size_t num_pages, char *mem_offset) {

    //TODO - num_pages: painful algorithm needed. Could also look at option to use bigger pages for large allocation?

    virt_addr_info_t info = get_virt_addr_info(virtual_page_addr);

    uint64_t *pml4 = (void *)(mem_offset + pml4_addr);

    uint64_t pdpt_addr = map_page_entry(pml4, 0, PDE64_USER | PDE64_WRITEABLE, PAGE_CREATE, mem_offset);
    uint64_t *pdpt = (void *)(mem_offset + pdpt_addr);

    uint64_t pd_addr = map_page_entry(pdpt, info.pg_dir_ptr_offset, PDE64_USER | PDE64_WRITEABLE, PAGE_CREATE, mem_offset);
    uint64_t *pd = (void *)(mem_offset + pd_addr);

    uint64_t pd2_addr = map_page_entry(pd, info.pg_dir_offset, PDE64_USER | PDE64_WRITEABLE, PAGE_CREATE, mem_offset);
    uint64_t *pd2 = (void *)(mem_offset + pd2_addr);

    map_page_entry(pd2, info.pg_tbl_offset, flags | PDE64_WRITEABLE, physical_page_addr, mem_offset);
}