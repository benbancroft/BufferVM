//
// Created by ben on 27/12/16.
//

#include "../../libc/stdlib.h"
#include "../h/stack.h"
#include "../h/utils.h"
#include "../h/vma.h"
#include "../h/host.h"
#include "../../common/paging.h"

#define PF_PROT         (1<<0)
#define PF_WRITE        (1<<1)
#define PF_USER         (1<<2)
#define PF_RSVD         (1<<3)
#define PF_INSTR        (1<<4)

void handle_segfault(uint64_t addr){

    vma_print();

    printf("\nSEGFAULT ERROR at addr: %p\n", addr);
    host_exit();
}

int handle_page_fault(uint64_t addr, uint64_t error_code, uint64_t rip){

    vm_area_t *vma;

    /*bool p, w, u, r, i;

    p = (bool)(error_code & PF_PROT);
    w = (bool)(error_code & PF_WRITE);
    u = (bool)(error_code & PF_USER);
    r = (bool)(error_code & PF_RSVD);
    i = (bool)(error_code & PF_INSTR);

    printf("page fault at %p error: %d rip %p\n", addr, error_code, rip);
    disassemble_address(rip, 5);

    printf("P: %d W: %d U: %d R: %d I: %d\n", p, w, u, r ,i);*/

    if (!(error_code & PF_PROT)){

        vma = vma_find(addr);

        if (!vma){
            goto seg_fault;
        }

        if (vma->start_addr <= addr){
            goto handle_paging;
        }

        if (!(vma->flags & VMA_GROWS)){
            goto seg_fault;
        }

        if (grow_stack(vma, addr)){
            goto seg_fault;
        }

    handle_paging:
        ASSERT(!(vma->flags & VMA_IS_PREFAULTED));
        printf("loading page at addr: %p\n", addr);
        map_physical_pages(PAGE_ALIGN_DOWN(addr), -1, vma_prot_to_pg(vma->page_prot) | PDE64_USER, 1, false, 0);
        return 1;
    }

seg_fault:
    handle_segfault(addr);
    return 0;
}
