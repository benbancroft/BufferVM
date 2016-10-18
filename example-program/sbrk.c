#include "sbrk.h"

void *__curbrk = NULL;

int brk(void *addr) {
    void *newbrk = _brk(addr);

    __curbrk = newbrk;

    if (newbrk < addr) {
        return -1;
        printf("F:\n");
    }

    return 0;
}

void *sbrk(intptr_t increment) {
    void *oldbrk;

    //printf("SBRK: %d\n", increment);

    if (__curbrk == NULL)
        if (brk(0) < 0)        /* Initialize the break.  */
            return (void *) -1;

    if (increment == 0)
        return __curbrk;

    oldbrk = __curbrk;
    if ((increment > 0 ? ((size_t) oldbrk + (size_t) increment < (size_t) oldbrk) : ((size_t) oldbrk <
                                                                                              (size_t) -increment)) ||
        brk(oldbrk + increment) < 0)
        return (void *) -1;

    return oldbrk;
}
