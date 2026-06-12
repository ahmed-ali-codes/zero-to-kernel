/* ============================================================================
 * include/idt.h — Interrupt Descriptor Table Interface
 *
 * Purpose : Public API for IDT setup. The IDT maps CPU interrupt/exception
 *           vectors to handler functions. We register 32 ISR stubs (CPU
 *           exceptions) and 16 IRQ stubs (hardware interrupts).
 * ============================================================================ */

#ifndef IDT_H
#define IDT_H

#include <stdint.h>

/* --------------------------------------------------------------------------
 * IDT Gate (interrupt descriptor) — 8 bytes
 * -------------------------------------------------------------------------- */
typedef struct __attribute__((packed)) {
    uint16_t base_low;      /* lower 16 bits of handler address  */
    uint16_t sel;           /* code segment selector             */
    uint8_t  always0;       /* always 0                          */
    uint8_t  flags;         /* type + DPL + present bit          */
    uint16_t base_high;     /* upper 16 bits of handler address  */
} idt_entry_t;

/* --------------------------------------------------------------------------
 * IDT Pointer — loaded with LIDT instruction
 * -------------------------------------------------------------------------- */
typedef struct __attribute__((packed)) {
    uint16_t limit;
    uint32_t base;
} idt_ptr_t;

/* --------------------------------------------------------------------------
 * Register state pushed by our ISR/IRQ stubs
 * -------------------------------------------------------------------------- */
typedef struct __attribute__((packed)) {
    uint32_t ds;        /* Data segment selector */
    /* Pushed by pusha */
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    /* Our stub pushes these */
    uint32_t int_no;    /* interrupt / IRQ number */
    uint32_t err_code;  /* error code (0 if none) */
    /* CPU pushes these automatically on interrupt */
    uint32_t eip, cs, eflags, useresp, ss;
} registers_t;

/* --------------------------------------------------------------------------
 * Handler function type
 * -------------------------------------------------------------------------- */
typedef void (*isr_handler_t)(registers_t *regs);

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

/* Install the IDT */
void idt_install(void);

/* Register a C-level handler for a given interrupt vector */
void idt_set_handler(uint8_t num, isr_handler_t handler);

/* Assembly stub — loads IDTR */
extern void idt_flush(uint32_t idt_ptr_addr);

/* C-level dispatcher called by assembly stubs */
void isr_handler(registers_t *regs);
void irq_handler(registers_t *regs);

/* ISR stub declarations (defined in interrupts.asm) */
extern void isr0(void);  extern void isr1(void);  extern void isr2(void);
extern void isr3(void);  extern void isr4(void);  extern void isr5(void);
extern void isr6(void);  extern void isr7(void);  extern void isr8(void);
extern void isr9(void);  extern void isr10(void); extern void isr11(void);
extern void isr12(void); extern void isr13(void); extern void isr14(void);
extern void isr15(void); extern void isr16(void); extern void isr17(void);
extern void isr18(void); extern void isr19(void); extern void isr20(void);
extern void isr21(void); extern void isr22(void); extern void isr23(void);
extern void isr24(void); extern void isr25(void); extern void isr26(void);
extern void isr27(void); extern void isr28(void); extern void isr29(void);
extern void isr30(void); extern void isr31(void);

/* IRQ stub declarations (defined in interrupts.asm) */
extern void irq0(void);  extern void irq1(void);  extern void irq2(void);
extern void irq3(void);  extern void irq4(void);  extern void irq5(void);
extern void irq6(void);  extern void irq7(void);  extern void irq8(void);
extern void irq9(void);  extern void irq10(void); extern void irq11(void);
extern void irq12(void); extern void irq13(void); extern void irq14(void);
extern void irq15(void);

#endif /* IDT_H */
