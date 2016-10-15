//
// Created by ben on 13/10/16.
//

#include "../h/kernel.h"

#include "../h/idt.h"
#include "../h/host.h"

void kernel_main() {
    //idt_initialize();
    printf("Unsigned sizes: %d %d %d %d\n", sizeof (uint8_t), sizeof (uint16_t), sizeof (uint32_t), sizeof (uint64_t));
    printf("Signed sizes: %d %d %d %d\n", sizeof (int8_t), sizeof (int16_t), sizeof (int32_t), sizeof (int64_t));

}
