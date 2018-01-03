//
// Created by ben on 30/12/17.
//

#define _GNU_SOURCE 1

#include <unistd.h>
#include <sys/mman.h>
#include <asm/errno.h>
#include <errno.h>

#include "vma.h"
#include "vm.h"
#include "../common/paging.h"

rb_root_t vma_rb_root;
rb_root_t *vma_rb_root_ptr;
uint64_t vma_highest_addr;
vm_area_t **vma_list_start_ptr;
uint64_t vma_heap_addr;
uint64_t vma_heap_phys_addr = 0;

inline vm_area_t *vma_get_list() {
    return (vma_ptr(*vma_list_start_ptr));
}

static inline void *vma_heap_ptr(void *vma) {

    if (vma == NULL ||
        (!vma_heap_phys_addr && !get_phys_addr((uint64_t) vma_heap_addr, &vma_heap_phys_addr)))
        return NULL;

    return ((vm_area_t *) (vm.mem + vma_heap_phys_addr + ((uint64_t) vma - vma_heap_addr)));
}

inline vm_area_t *vma_ptr(vm_area_t *vma) {
    return ((vm_area_t *) vma_heap_ptr(vma));
}

inline rb_node_t *vma_rb_ptr(rb_node_t *vma) {
    return ((rb_node_t *) vma_heap_ptr(vma));
}

static void remap_region(int64_t old_start, size_t old_size, int64_t new_start, size_t new_size) {
    void *ret = mremap(vm.mem + old_start, old_size, new_size,
                       MREMAP_MAYMOVE | MREMAP_FIXED, vm.mem + new_start);

    if (ret != vm.mem + new_start) {
        printf("Fail remapping - PA %p size %lx to PA %p size %lx errno: %d\n", vm.mem + old_start,
               old_size, vm.mem + new_start, new_size, errno);

        vma_print();

        while(1);
        exit(1);
    } else {
        printf("Remapped PA %lx size %lx to PA %ld size %lx\n", old_start,
               old_size, new_start, new_size);
    }
}

void host_map_vma(vm_area_t *vma) {
    int64_t new_phys;

    vma = vma_ptr(vma);

    size_t pages = PAGE_DIFFERENCE(vma->end_addr, vma->start_addr);
    printf("Mapped Pages in range: %p to %p, num: %ld\n", (void *) vma->start_addr, (void *) vma->end_addr, pages);
    printf("Used to be in range: %p to %p\n", (void *) vma->old_start_addr, (void *) vma->old_end_addr);

    size_t old_size = vma->phys_page_end - vma->phys_page_start;

    bool vma_changed = vma->old_start_addr != vma->start_addr || vma->old_end_addr != vma->end_addr;

    if (vma->phys_page_start == -1 || vma_changed) {
        new_phys = map_physical_pages(vma->start_addr,
                                      -1, vma_prot_to_pg(vma->page_prot),
                                      pages,
                                      MAP_CONTINUOUS);
    }

    if (vma->phys_page_start != -1 && vma_changed) {
            if (vma->old_start_addr >= vma->start_addr) {
                size_t move_size =
                        old_size - (vma->old_end_addr > vma->end_addr ? vma->old_end_addr - vma->end_addr : 0);
                printf("1 - file: %d old size %lx move size %lx offset %lx\n", vma->file_info.fd, old_size, move_size, vma->old_start_addr - vma->start_addr);
                remap_region(vma->phys_page_start, move_size, new_phys + (vma->old_start_addr - vma->start_addr),
                             move_size);
            } else {
                //wrong
                printf("2 - file: %d\n", vma->file_info.fd);
                size_t move_size = old_size - (vma->start_addr - vma->old_start_addr) -
                                   (vma->old_end_addr > vma->end_addr ? vma->old_end_addr - vma->end_addr : 0);
                remap_region(vma->phys_page_start + (vma->start_addr - vma->old_start_addr), move_size, new_phys,
                             move_size);
            }
            vma->updated = true;

            /*if (vma->file_info.fd != -1) {
                printf("TODO files\n");
                exit(1);
            }*/
    }

    vma->phys_page_start = new_phys;
    vma->phys_page_end = new_phys + pages * PAGE_SIZE;
    vma->old_start_addr = vma->start_addr;
    vma->old_end_addr = vma->end_addr;

    vma->faulted = true;
    vma->flags |= VMA_IS_PREFAULTED;

}

void host_unmap_vma(vm_area_t *vma) {
    vma = vma_ptr(vma);

    if (vma->phys_page_start != -1) {
        size_t pages = PAGE_DIFFERENCE(vma->old_end_addr, vma->old_start_addr);
        printf("Unmapped Pages in range: %p to %p, num: %ld\n", (void *) vma->old_start_addr, (void *) vma->old_end_addr, pages);
        for (size_t i = 0; i < pages; i++) {
            unmap_physical_page(vma->old_start_addr + i * PAGE_SIZE);
        }
    }
}
