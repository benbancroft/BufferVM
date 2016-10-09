#ifndef SBRK_D
#define SBRK_D

#include "stdlib.h"

int brk (void *addr);
void *sbrk (intptr_t increment);

#endif
