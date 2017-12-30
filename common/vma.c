//
// Created by ben on 30/12/17.
//

#include <stdlib.h>
#include "vma.h"
#include "utils.h"

void vma_print_node(vm_area_t *vma, bool follow)
{
    if (vma == NULL) return;

    printf("VMA addr: %p end: %p\n", vma->start_addr, vma->end_addr);

    if (follow)
        vma_print_node(vma->next, true);
}

void vma_print(){
    printf("\nVMAs:\n");
    printf("-------------------------------------\n");

    vma_print_node(vma_list_start, true);
    printf("\n");
}

vm_area_t *vma_find(uint64_t addr)
{
    rb_node_t *rb_node;
    vm_area_t *vma = NULL;

    rb_node = vma_rb_root.rb_node;

    while (rb_node) {
        vm_area_t*tmp;

        tmp = container_of(rb_node, vm_area_t, rb_node);

        if (tmp->end_addr > addr) {
            vma = tmp;
            if (tmp->start_addr <= addr){
                break;
            }
            rb_node = rb_node->rb_left;
        } else
            rb_node = rb_node->rb_right;
    }

    return vma;
}
