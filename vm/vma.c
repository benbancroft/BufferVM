//
// Created by ben on 30/12/17.
//

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

void host_unmap_vma(vm_area_t *vma) {

    vma = vma_ptr(vma);

    size_t pages = PAGE_DIFFERENCE(vma->end_addr, vma->start_addr);
    printf("Unmapped Pages in range: %p to %p n %ld\n", (void *) vma->start_addr, (void *) vma->end_addr, pages);
    for (size_t i = 0; i < pages; i++) {
        unmap_physical_page(vma->start_addr + i * PAGE_SIZE);
    }
}
