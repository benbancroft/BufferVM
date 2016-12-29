//
// Created by ben on 29/12/16.
//

#include "../../libc/stdlib.h"
#include "../h/vma.h"
#include "../h/utils.h"
#include "../h/kernel.h"

vma_node_t *free_list;

void *vma_heap_head;

static bool *vma_freelist_find(vma_node_t *next, vm_area_t *addr){
    if (next == NULL || &next->vma == addr) return false;
    else return vma_freelist_find(next->next_freed, addr);
}

static void *vma_next_freed(vma_node_t *next){
    if (next == NULL) return NULL;
    else if (next->next_freed == NULL) return next;
    else return vma_next_freed(next->next_freed);
}

void vma_init(size_t max_entries){
    vma_heap_head = (void *)(kernel_min_address - max_entries * sizeof (vma_node_t));
    kernel_min_address = (uint64_t)vma_heap_head;
}

vm_area_t *vma_alloc(){
    vma_node_t* node;
    if ((node = vma_next_freed(free_list)) == NULL){
        node = (vma_node_t *)vma_heap_head;
        vma_heap_head += sizeof (vma_node_t);
    }

    return &node->vma;
}

void vma_free(vm_area_t *addr){
    if (vma_freelist_find(free_list, addr)){
        printf("Double free of VMA!\n");
    } else{
        vma_node_t *free_node;

        free_node = container_of(addr, vma_node_t, vma);

        //if list not empty, slip in at front
        if (free_list != NULL)
            free_node->next_freed = free_list;

        free_list = free_node;

    }
}