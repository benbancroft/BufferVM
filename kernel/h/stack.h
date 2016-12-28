//
// Created by ben on 27/12/16.
//

#ifndef PROJECT_STACK_H
#define PROJECT_STACK_H

#include "../../libc/stdlib.h"

extern uint64_t user_stack_start;
extern uint64_t user_stack_min;

void user_stack_init(uint64_t _user_stack_start, const uint64_t stack_page_limit);
int grow_stack(uint64_t addr);

#endif //PROJECT_STACK_H
