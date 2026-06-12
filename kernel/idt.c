/* ============================================================================
 * kernel/idt.c — Interrupt Descriptor Table
 *
 * Purpose : Populates the IDT with gates for all 32 CPU exceptions (ISR 0-31)
 *           and 16 hardware IRQ stubs (IRQ 0-15, mapped to vectors 32-47).
 *           Provides a C-level dispatcher (isr_handler / irq_handler) that
 *           the assembly stubs call after saving CPU state.
 *
 *           isr_handler: handles CPU exceptions (prints and halts)
 *           irq_handler:  dispatches to registered C handler functions
 * ============================================================================ */

#include "../include/idt.h"
#include "../include/screen.h"
#include "../include/pic.h"
#include "../include/memory.h"

/* --------------------------------------------------------------------------
 * IDT storage (256 entries)
 * -------------------------------------------------------------------------- */
#define IDT_NUM_ENTRIES  256

static idt_entry_t idt_entries[IDT_NUM_ENTRIES];
static idt_ptr_t   idt_ptr;

/* Registered C handlers for IRQs (IRQ 0-15) */
static isr_handler_t irq_handlers[16];

/* Registered C handlers for ISRs/exceptions (0-31) */
static isr_handler_t isr_handlers[32];

/* --------------------------------------------------------------------------
 * Internal helper — set one IDT gate
 * -------------------------------------------------------------------------- */
static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt_entries[num].base_low  = (base & 0xFFFF);
    idt_entries[num].base_high = (base >> 16) & 0xFFFF;
    idt_entries[num].sel       = sel;
    idt_entries[num].always0   = 0;
    /* OR with 0x60 to set the DPL (ring 0 = 0, but let's keep ring 0 only) */
    idt_entries[num].flags     = flags;
}

/* --------------------------------------------------------------------------
 * Install the IDT
 * -------------------------------------------------------------------------- */
void idt_install(void) {
    idt_ptr.limit = (sizeof(idt_entry_t) * IDT_NUM_ENTRIES) - 1;
    idt_ptr.base  = (uint32_t)&idt_entries;

    memset(&idt_entries, 0, sizeof(idt_entries));
    memset(irq_handlers,  0, sizeof(irq_handlers));
    memset(isr_handlers,  0, sizeof(isr_handlers));

    /* Register all CPU exception ISR stubs */
    idt_set_gate( 0, (uint32_t)isr0,  0x08, 0x8E);
    idt_set_gate( 1, (uint32_t)isr1,  0x08, 0x8E);
    idt_set_gate( 2, (uint32_t)isr2,  0x08, 0x8E);
    idt_set_gate( 3, (uint32_t)isr3,  0x08, 0x8E);
    idt_set_gate( 4, (uint32_t)isr4,  0x08, 0x8E);
    idt_set_gate( 5, (uint32_t)isr5,  0x08, 0x8E);
    idt_set_gate( 6, (uint32_t)isr6,  0x08, 0x8E);
    idt_set_gate( 7, (uint32_t)isr7,  0x08, 0x8E);
    idt_set_gate( 8, (uint32_t)isr8,  0x08, 0x8E);
    idt_set_gate( 9, (uint32_t)isr9,  0x08, 0x8E);
    idt_set_gate(10, (uint32_t)isr10, 0x08, 0x8E);
    idt_set_gate(11, (uint32_t)isr11, 0x08, 0x8E);
    idt_set_gate(12, (uint32_t)isr12, 0x08, 0x8E);
    idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E);
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);
    idt_set_gate(15, (uint32_t)isr15, 0x08, 0x8E);
    idt_set_gate(16, (uint32_t)isr16, 0x08, 0x8E);
    idt_set_gate(17, (uint32_t)isr17, 0x08, 0x8E);
    idt_set_gate(18, (uint32_t)isr18, 0x08, 0x8E);
    idt_set_gate(19, (uint32_t)isr19, 0x08, 0x8E);
    idt_set_gate(20, (uint32_t)isr20, 0x08, 0x8E);
    idt_set_gate(21, (uint32_t)isr21, 0x08, 0x8E);
    idt_set_gate(22, (uint32_t)isr22, 0x08, 0x8E);
    idt_set_gate(23, (uint32_t)isr23, 0x08, 0x8E);
    idt_set_gate(24, (uint32_t)isr24, 0x08, 0x8E);
    idt_set_gate(25, (uint32_t)isr25, 0x08, 0x8E);
    idt_set_gate(26, (uint32_t)isr26, 0x08, 0x8E);
    idt_set_gate(27, (uint32_t)isr27, 0x08, 0x8E);
    idt_set_gate(28, (uint32_t)isr28, 0x08, 0x8E);
    idt_set_gate(29, (uint32_t)isr29, 0x08, 0x8E);
    idt_set_gate(30, (uint32_t)isr30, 0x08, 0x8E);
    idt_set_gate(31, (uint32_t)isr31, 0x08, 0x8E);

    /* Register IRQ stubs (vectors 32-47) */
    idt_set_gate(32, (uint32_t)irq0,  0x08, 0x8E);
    idt_set_gate(33, (uint32_t)irq1,  0x08, 0x8E);
    idt_set_gate(34, (uint32_t)irq2,  0x08, 0x8E);
    idt_set_gate(35, (uint32_t)irq3,  0x08, 0x8E);
    idt_set_gate(36, (uint32_t)irq4,  0x08, 0x8E);
    idt_set_gate(37, (uint32_t)irq5,  0x08, 0x8E);
    idt_set_gate(38, (uint32_t)irq6,  0x08, 0x8E);
    idt_set_gate(39, (uint32_t)irq7,  0x08, 0x8E);
    idt_set_gate(40, (uint32_t)irq8,  0x08, 0x8E);
    idt_set_gate(41, (uint32_t)irq9,  0x08, 0x8E);
    idt_set_gate(42, (uint32_t)irq10, 0x08, 0x8E);
    idt_set_gate(43, (uint32_t)irq11, 0x08, 0x8E);
    idt_set_gate(44, (uint32_t)irq12, 0x08, 0x8E);
    idt_set_gate(45, (uint32_t)irq13, 0x08, 0x8E);
    idt_set_gate(46, (uint32_t)irq14, 0x08, 0x8E);
    idt_set_gate(47, (uint32_t)irq15, 0x08, 0x8E);

    idt_flush((uint32_t)&idt_ptr);
}

/* Register a C handler for a given vector */
void idt_set_handler(uint8_t num, isr_handler_t handler) {
    if (num < 32) {
        isr_handlers[num] = handler;
    } else if (num >= 32 && num < 48) {
        irq_handlers[num - 32] = handler;
    }
}

/* --------------------------------------------------------------------------
 * CPU exception names (for diagnostics)
 * -------------------------------------------------------------------------- */
static const char *exception_names[] = {
    "Division By Zero",         "Debug",
    "Non-Maskable Interrupt",   "Breakpoint",
    "Overflow",                 "Bound Range Exceeded",
    "Invalid Opcode",           "Device Not Available",
    "Double Fault",             "Coprocessor Segment Overrun",
    "Invalid TSS",              "Segment Not Present",
    "Stack-Segment Fault",      "General Protection Fault",
    "Page Fault",               "Reserved",
    "x87 FPU Error",            "Alignment Check",
    "Machine Check",            "SIMD Floating-Point Exception",
    "Virtualisation Exception", "Reserved",
    "Reserved",                 "Reserved",
    "Reserved",                 "Reserved",
    "Reserved",                 "Reserved",
    "Reserved",                 "Reserved",
    "Security Exception",       "Reserved"
};

/* --------------------------------------------------------------------------
 * C dispatcher for CPU exceptions
 * -------------------------------------------------------------------------- */
void isr_handler(registers_t *regs) {
    uint32_t num = regs->int_no;

    /* Call registered handler if any */
    if (num < 32 && isr_handlers[num]) {
        isr_handlers[num](regs);
        return;
    }

    /* Default: print and halt */
    terminal_setcolor(COLOR_WHITE, COLOR_RED);
    terminal_writestring("\n*** KERNEL EXCEPTION ***\n");
    if (num < 32) {
        terminal_printf("  Exception #%u: %s\n", (unsigned)num, exception_names[num]);
    } else {
        terminal_printf("  Exception #%u\n", (unsigned)num);
    }
    terminal_printf("  EIP=0x%x  CS=0x%x  EFlags=0x%x\n",
                    regs->eip, regs->cs, regs->eflags);
    terminal_printf("  EAX=0x%x  EBX=0x%x  ECX=0x%x  EDX=0x%x\n",
                    regs->eax, regs->ebx, regs->ecx, regs->edx);
    terminal_printf("  Error code: 0x%x\n", regs->err_code);
    terminal_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
    terminal_writestring("\nSystem halted. Restart QEMU.\n");

    /* Halt */
    __asm__ volatile ("cli; hlt");
    for (;;) { __asm__ volatile ("hlt"); }
}

/* --------------------------------------------------------------------------
 * C dispatcher for hardware IRQs
 * -------------------------------------------------------------------------- */
void irq_handler(registers_t *regs) {
    uint32_t irq_num = regs->int_no - 32;

    /* Call registered handler */
    if (irq_num < 16 && irq_handlers[irq_num]) {
        irq_handlers[irq_num](regs);
    }

    /* Send EOI to PIC */
    pic_send_eoi((uint8_t)irq_num);
}
