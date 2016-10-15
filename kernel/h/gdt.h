//
// Created by ben on 15/10/16.
//

#ifndef KERNEL_GDT_H
#define KERNEL_GDT_H

#include "../../libc/stdlib.h"

// 64 bit long mode GDT segment descriptors
#define GDT_KERNEL_CODE     0x209800        //< kernel code (0x08)
#define GDT_KERNEL_DATA     0x209200        //< kernel data (0x10)
#define GDT_USER_CODE       0x20F800        //< user code (0x18)
#define GDT_USER_DATA       0x20F200        //< user data (0x20)

/**
 * The length of the GDT in bytes.
 */
#define GDT_LENGTH          0x1000

/**
 * Pointer to the GDT that can be loaded using the LGDT instruction.
 */
typedef struct gdt_pointer {
    uint16_t length;                //< length in bytes - 1
    uint64_t address;               //< virtual address
} __attribute__((packed)) gdt_pointer_t;

/**
 * The data of the 64 bit General Descriptor Table.
 */
extern uint64_t gdt_data[GDT_LENGTH / sizeof(uint64_t)];

/**
 * The pointer to the 64 bit General Descriptor Table.
 */
extern gdt_pointer_t gdt_pointer;

#endif //KERNEL_GDT_H
