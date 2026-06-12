/* ============================================================================
 * include/pic.h — 8259 Programmable Interrupt Controller Interface
 *
 * Purpose : API for initialising and managing the 8259A PIC. We remap its
 *           IRQ vectors away from the CPU exception range (0–31) so that
 *           hardware IRQs are delivered as vectors 32–47.
 * ============================================================================ */

#ifndef PIC_H
#define PIC_H

#include <stdint.h>

/* PIC I/O port addresses */
#define PIC1_COMMAND  0x20   /* master PIC command port */
#define PIC1_DATA     0x21   /* master PIC data port    */
#define PIC2_COMMAND  0xA0   /* slave  PIC command port */
#define PIC2_DATA     0xA1   /* slave  PIC data port    */

/* PIC vector offsets after remapping */
#define PIC1_OFFSET   0x20   /* IRQ 0-7  → vectors 32-39  */
#define PIC2_OFFSET   0x28   /* IRQ 8-15 → vectors 40-47  */

/* End-of-interrupt command */
#define PIC_EOI       0x20

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

/* Remap the PIC and initialise it */
void pic_init(void);

/* Send End-Of-Interrupt signal for a given IRQ number (0-15) */
void pic_send_eoi(uint8_t irq);

/* Mask (disable) a specific IRQ line (0-15) */
void pic_mask_irq(uint8_t irq);

/* Unmask (enable) a specific IRQ line (0-15) */
void pic_unmask_irq(uint8_t irq);

/* Helper: write a byte to an I/O port */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

/* Helper: read a byte from an I/O port */
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Helper: small I/O delay (write to port 0x80) */
static inline void io_wait(void) {
    outb(0x80, 0);
}

#endif /* PIC_H */
