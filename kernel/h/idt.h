//
// Created by ben on 13/10/16.
//

#ifndef KERNEL_IDT_H
#define KERNEL_IDT_H

#include "../../libc/stdlib.h"

extern void idt_flush(uint32_t);

struct idt_entry
{
    uint16_t base_lo;
    uint16_t sel;
    uint8_t always0;
    uint8_t flags;
    uint16_t base_hi;
} __attribute__((packed));

struct idt_ptr
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

struct idt_entry idt[256];
struct idt_ptr idtptr;

void idt_initialize();
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);

#endif //KERNEL_IDT_H
