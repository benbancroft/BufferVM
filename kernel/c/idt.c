#include <linux/types.h>
#include "../h/idt.h"
#include "../h/host.h"

static void idt_unhandled(void)
{
    asm volatile ("cli");
    void *rip;
    asm("movq $., %0" : "=r"(rip));
    printf("Unhandled interrupt at: %p\n", rip);
    host_exit();
}

static void idt_fault_pf(void)
{
    asm volatile ("cli");
    printf("Page fault.\n");
    host_exit();
}

void idt_set_gate(uint8_t num, uint64_t handler, uint8_t dpl)
{
    if (num >= 0 && num < 256)
    {
        /* Set the ISR's base address */
        idt[num].handler_low = (handler & 0xFFFF);
        idt[num].handler_medium = (handler >> 16) & 0xFFFF;
        idt[num].handler_high = (handler >> 32) & 0xFFFFFFFF;

        /* Set the ISR's code segment selector */
        idt[num].selector = 3 << 3;
        idt[num].ist = 0;
        idt[num].zero = 0;

        /* Build the entry's access flags */
        idt[num].flags = 0xE;                 // interrupt gate
        idt[num].flags |= (dpl & 0b11) << 5;  // dpl
        idt[num].flags |= (1 << 7); // present
    }
}

void idt_init(bool bsp)
{

    size_t i;
    for (i = 0; i < IDT_MAX; ++i) {
        idt_set_gate(i, (uintptr_t) &idt_unhandled, 0x0);
    }

    idt_set_gate(14, (uintptr_t) &idt_page_fault_handler, 0x0);
    //idt_set_gate(&idt[13], (uintptr_t) &idt_fault_gp, 0x8, 0x0);

    /* Running on the bootstrap processor */
    if (bsp)
    {
        /* Set up the IDT register structure */
        idtr.limit = (sizeof(struct idt_entry) * IDT_MAX) - 1;
        idtr.base = (uintptr_t) &idt;
    }

    //printf("L: %d B %p\n", idtr.limit, idtr.base);

    /* Load our new IDT */
    idt_load((uint64_t)&idtr);
}