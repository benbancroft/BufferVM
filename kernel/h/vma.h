//
// Created by ben on 29/12/16.
//

#ifndef PROJECT_VMA_H
#define PROJECT_VMA_H

#include "../../common/stdlib_inc.h"
#include "../../libc/stdlib.h"

typedef struct vm_area vm_area_t;


struct vm_area {
    /* The first cache line has the info for VMA tree walking. */

    unsigned long vm_start;         /* Our start address within vm_mm. */
    unsigned long vm_end;           /* The first byte after our end address
                                            within vm_mm. */

    /* linked list of VM areas per task, sorted by address */
    vm_area_t *vm_next, *vm_prev;

    //struct rb_node vm_rb;

    /*
     * Largest free memory gap in bytes to the left of this VMA.
     * Either between this VMA and vma->vm_prev, or between one of the
     * VMAs below us in the VMA rbtree and its ->vm_prev. This helps
     * get_unmapped_area find a free area of the right size.
     */
    unsigned long rb_subtree_gap;

    /* Second cache line starts here. */

    struct mm_struct *vm_mm;        /* The address space we belong to. */
    uint64_t vm_page_prot;          /* Access permissions of this VMA. */
    uint64_t vm_flags;         /* Flags, see mm.h. */

    /*
     * A file's MAP_PRIVATE vma can be in both i_mmap tree and anon_vma
     * list, after a COW of one of the file pages.  A MAP_SHARED vma
     * can only be in the i_mmap tree.  An anonymous MAP_PRIVATE, stack
     * or brk vma (with NULL file) can only be in an anon_vma list.
     */
#if 0
    struct list_head anon_vma_chain; /* Serialized by mmap_sem &
                                           * page_table_lock */
    struct anon_vma *anon_vma;      /* Serialized by page_table_lock */

#endif

    /* Function pointers to deal with this struct. */
    const struct vm_operations_struct *vm_ops;

    /* Information about our backing store: */
    unsigned long vm_pgoff;         /* Offset (within vm_file) in PAGE_SIZE
                                            units */
    struct file * vm_file;          /* File we map to (can be NULL). */
    void * vm_private_data;         /* was vm_pte (shared mem) */
};

typedef struct vma_node vma_node_t;

struct vma_node {
    vma_node_t *next_freed;
    vm_area_t vma;
};

void vma_init(size_t max_entries);
vm_area_t *vma_alloc();
vm_area_t *vma_zalloc();
void vma_free(vm_area_t *addr);

#endif //PROJECT_VMA_H
