/* ============================================================================
 * kernel/timer.c — Programmable Interval Timer (PIT) Driver
 *
 * Purpose : Initializes the PIT (IRQ0) to fire at a specified frequency.
 *           Maintains a global tick count used for uptime calculation and
 *           seeding the random number generator.
 * ============================================================================ */

#include "../include/timer.h"
#include "../include/idt.h"
#include "../include/pic.h"

/* The PIT has an internal clock running at 1193180 Hz */
#define PIT_INTERNAL_FREQ 1193180

/* I/O ports for the PIT */
#define PIT_CHANNEL0 0x40
#define PIT_COMMAND  0x43

static volatile uint32_t timer_ticks = 0;
static uint32_t timer_freq = 100;

/* --------------------------------------------------------------------------
 * IRQ0 Handler
 * -------------------------------------------------------------------------- */
static void timer_irq_handler(registers_t *regs) {
    (void)regs;
    timer_ticks++;
}

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

void timer_install(uint32_t frequency) {
    timer_freq = frequency;
    
    /* Register our handler for IRQ0 (Vector 32) */
    idt_set_handler(32, timer_irq_handler);

    /* Calculate the divisor (must fit in 16 bits) */
    uint32_t divisor = PIT_INTERNAL_FREQ / frequency;
    if (divisor > 65535) divisor = 65535;
    if (divisor == 0) divisor = 1;

    /* Send command word to PIT: Channel 0, lobyte/hibyte access, Mode 3 (square wave), binary */
    outb(PIT_COMMAND, 0x36);
    
    /* Send the divisor (low byte, then high byte) */
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));

    /* Unmask IRQ0 on the PIC so we start receiving interrupts */
    pic_unmask_irq(0);
}

uint32_t timer_get_ticks(void) {
    return timer_ticks;
}

uint32_t timer_get_uptime_seconds(void) {
    return timer_ticks / timer_freq;
}
