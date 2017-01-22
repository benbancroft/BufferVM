//
// Created by ben on 29/12/16.
//

#ifndef PROJECT_VMA_H
#define PROJECT_VMA_H

#include "../../common/stdlib_inc.h"
#include "../../libc/stdlib.h"
#include "rbtree.h"
#include "../../common/syscall.h"

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
    file_t file_info;
};

#define VMA_READ         0x00000001
#define VMA_WRITE        0x00000002
#define VMA_EXEC         0x00000004
#define VMA_SHARED       0x00000008
#define VMA_IS_VERSIONED    0x00000010

#define VMA_GROWS   0x00000010
#define VMA_IS_PREFAULTED   0x00000020

//do typedef earlier due to recursive reference
typedef struct vma_node vma_node_t;

struct vma_node {
    vma_node_t *next_freed;
    vm_area_t vma;
};

extern rb_root_t vma_rb_root;
extern uint64_t vma_highest_addr;
extern vm_area_t *vma_list_start;

void vma_init(size_t max_entries);

vm_area_t *vma_alloc();

vm_area_t *vma_zalloc();

void vma_free(vm_area_t *addr);

void vma_print();

void vma_gap_update(vm_area_t *vma);

vm_area_t *vma_find(uint64_t addr);

uint64_t vma_prot_to_pg(uint64_t prot);

int64_t vma_fault(vm_area_t *vma, bool continuous);

uint64_t
mmap_region(file_t *file_info, uint64_t addr, uint64_t length, uint64_t vma_flags, uint64_t vma_prot, uint64_t offset,
            vm_area_t **vma_out);

#endif //PROJECT_VMA_H
