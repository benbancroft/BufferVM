//
// Created by ben on 27/12/16.
//

#include "../../libc/stdlib.h"
#include "../h/stack.h"
#include "../h/utils.h"

#define PF_PROT         (1<<0)
#define PF_WRITE        (1<<1)
#define PF_USER         (1<<2)
#define PF_RSVD         (1<<3)
#define PF_INSTR        (1<<4)

int handle_page_fault(uint64_t addr, uint64_t error_code, uint64_t rip){

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
        //TODO - switch to VMA code when ready
        if (addr > user_stack_min && addr <= user_stack_start)
            return grow_stack(addr);
        else
            printf("here\n");
    }

    return 0;
}