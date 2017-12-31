//
// Created by ben on 30/12/17.
//

#ifndef COMMON_VMA_H
#define COMMON_VMA_H

#include <stdint.h>
#include <stdbool.h>
#include "syscall.h"
#include "rbtree.h"

typedef struct vm_area vm_area_t;

struct vm_area {
    uint64_t start_addr;
    uint64_t end_addr;

    uint64_t page_prot;
    uint64_t flags;

    //reference to VMA above and below
    vm_area_t *prev, *next;

    rb_node_t rb_node;

    //memory gap pre-calculated between VMA and previous, or VMA below in rbtree and its previous
    uint64_t rb_subtree_gap;

    //file page offset
    uint64_t page_offset;
    vm_file_t file_info;
};

#define VMA_READ         0x00000001
#define VMA_WRITE        0x00000002
#define VMA_EXEC         0x00000004
#define VMA_SHARED       0x00000008
#define VMA_IS_VERSIONED    0x00000010

#define VMA_GROWS   0x00000010
#define VMA_IS_PREFAULTED   0x00000020

extern rb_root_t vma_rb_root;
extern rb_root_t *vma_rb_root_ptr;
extern uint64_t vma_highest_addr;
extern vm_area_t *vma_list_start;
extern vm_area_t **vma_list_start_ptr;
extern uint64_t vma_heap_addr;

//stubs for none common code

vm_area_t *vma_ptr(vm_area_t *);
rb_node_t *vma_rb_ptr(rb_node_t *vma);

//common functions

void vma_print_node(vm_area_t *vma, bool follow);

void vma_print();

vm_area_t *vma_find(uint64_t addr);

#endif //COMMON_VMA_H
