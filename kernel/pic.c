/* ============================================================================
 * kernel/pic.c — 8259A Programmable Interrupt Controller Driver
 *
 * Purpose : Remaps the PIC so that hardware IRQ 0-15 arrive at IDT vectors
 *           32-47 instead of overlapping with CPU exceptions (0-31).
 *           Provides pic_send_eoi() so interrupt handlers can acknowledge
 *           completion and re-enable future interrupts on that line.
 * ============================================================================ */

#include "../include/pic.h"

/* PIC initialisation command words */
#define ICW1_ICW4       0x01   /* ICW4 will be sent              */
#define ICW1_SINGLE     0x02   /* single (cascade) mode          */
#define ICW1_INIT       0x10   /* initialisation command         */
#define ICW4_8086       0x01   /* 8086/88 mode                   */

void pic_init(void) {
    /* Start initialisation sequence (ICW1) */
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();

    /* ICW2: vector offsets */
    outb(PIC1_DATA, PIC1_OFFSET);   /* master IRQ 0  → vector 32 */
    io_wait();
    outb(PIC2_DATA, PIC2_OFFSET);   /* slave  IRQ 8  → vector 40 */
    io_wait();

    /* ICW3: master has slave on IRQ2, slave is on cascade line 2 */
    outb(PIC1_DATA, 0x04);          /* master: slave at IRQ2 (bit 2) */
    io_wait();
    outb(PIC2_DATA, 0x02);          /* slave:  cascade identity = 2  */
    io_wait();

    /* ICW4: 8086 mode for both */
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    /* Mask all interrupts by default. We will unmask them individually. */
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        /* Slave PIC also needs EOI for IRQ8-15 */
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

void pic_mask_irq(uint8_t irq) {
    uint16_t port;
    uint8_t  value;
    if (irq < 8) {
        port  = PIC1_DATA;
    } else {
        port  = PIC2_DATA;
        irq  -= 8;
    }
    value = inb(port) | (uint8_t)(1 << irq);
    outb(port, value);
}

void pic_unmask_irq(uint8_t irq) {
    uint16_t port;
    uint8_t  value;
    if (irq < 8) {
        port  = PIC1_DATA;
    } else {
        port  = PIC2_DATA;
        irq  -= 8;
    }
    value = inb(port) & (uint8_t)~(1 << irq);
    outb(port, value);
}
