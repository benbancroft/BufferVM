//
// Created by ben on 13/10/16.
//

#include "../h/kernel.h"

#include "../h/idt.h"
#include "../h/host.h"

void kernel_main() {
    idt_init(true);
    printf("Unsigned sizes: %d %d %d %d\n", sizeof (uint8_t), sizeof (uint16_t), sizeof (uint32_t), sizeof (uint64_t));
    printf("Signed sizes: %d %d %d %d\n", sizeof (int8_t), sizeof (int16_t), sizeof (int32_t), sizeof (int64_t));

    //int a = 5;
    //printf("Div: %d\n", a / 0);
    //asm volatile ("int $14");

}
