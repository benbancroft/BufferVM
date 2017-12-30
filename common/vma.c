//
// Created by ben on 30/12/17.
//

#include <stdlib.h>
#include <stdio.h>
#include "vma.h"
#include "utils.h"

void vma_print_node(vm_area_t *vma, bool follow)
{
    if (vma == NULL) return;

    printf("VMA addr: %p end: %p file: %d\n", (void *)vma->start_addr, (void *)vma->end_addr, vma->file_info.fd);

    if (follow)
        vma_print_node(vma_ptr(vma->next), true);
}

void vma_print(){
    vm_area_t *start;
    printf("\nVMAs:\n");
    printf("-------------------------------------\n");

    start = vma_ptr(*vma_list_start_ptr);

    if (start != NULL){
        vma_print_node(start, true);
        printf("\n");
    }else {
        printf("No VMAs allocated\n");
    }
}

vm_area_t *vma_find(uint64_t addr)
{
    rb_node_t *rb_node;
    vm_area_t *vma = NULL;

    rb_node = vma_rb_ptr(vma_rb_root_ptr->rb_node);

    while (rb_node) {
        vm_area_t*tmp;

        tmp = container_of(rb_node, vm_area_t, rb_node);

        if (tmp->end_addr > addr) {
            vma = tmp;
            if (tmp->start_addr <= addr){
                break;
            }
            rb_node = vma_rb_ptr(rb_node->rb_left);
        } else
            rb_node = vma_rb_ptr(rb_node->rb_right);
    }

    return vma;
}
