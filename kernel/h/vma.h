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
    /* The first cache line has the info for VMA tree walking. */

    unsigned long vm_start;         /* Our start address within vm_mm. */
    unsigned long vm_end;           /* The first byte after our end address
                                            within vm_mm. */

    /* linked list of VM areas per task, sorted by address */
    vm_area_t *vm_next, *vm_prev;

    rb_node_t vm_rb;

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

    /* Information about our backing store: */
    unsigned long vm_pgoff;         /* Offset (within vm_file) in PAGE_SIZE
                                            units */
    file_t vm_file_info;          /* File we map to (can be NULL). */
    void * vm_private_data;         /* was vm_pte (shared mem) */
};

#define VM_NONE         0x00000000

#define VM_READ         0x00000001      /* currently active flags */
#define VM_WRITE        0x00000002
#define VM_EXEC         0x00000004
#define VM_SHARED       0x00000008

/* mprotect() hardcodes VM_MAYREAD >> 4 == VM_READ, and so for r/w/x bits. */
#define VM_MAYREAD      0x00000010      /* limits for mprotect() etc */
#define VM_MAYWRITE     0x00000020
#define VM_MAYEXEC      0x00000040
#define VM_MAYSHARE     0x00000080

#define VM_GROWSDOWN    0x00000100      /* general info on the segment */

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

uint64_t mmap_region(file_t *file_info, uint64_t addr, uint64_t length, uint64_t vma_flags, uint64_t offset);

#endif //PROJECT_VMA_H
