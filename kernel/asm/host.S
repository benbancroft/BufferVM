.text

#define HOSTCALL_DEF(NAME)  \
.globl host_## NAME;  \
host_ ## NAME:   \
    movq $(__COUNTER__), %rax;   \
    hlt; \
    ret;

#define HOSTCALL_DEF2(NAME)  \
.globl NAME;  \
NAME:   \
    movq $(__COUNTER__), %rax;   \
    hlt; \
    ret;

#define HOSTCALL_DEF_CUSTOM(NAME)

#include "../../common/host.h"

.globl unmap_physical_page
unmap_physical_page:
    pushq %rdi
    movq $(__COUNTER__), %rax
    hlt
    popq %rdi
    invlpg	(%rdi)
    ret
