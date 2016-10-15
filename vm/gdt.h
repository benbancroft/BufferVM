//
// Created by ben on 15/10/16.
//

#ifndef PROJECT_GDT_H
#define PROJECT_GDT_H

#include <stdint.h>
#include <linux/kvm.h>

typedef struct gdt_entry
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed))
gdt_entry_t;

void gdt_set_gate(gdt_entry_t *gdt, int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);

void fill_segment_descriptor(uint64_t *dt, struct kvm_segment *seg);

#endif //PROJECT_GDT_H
