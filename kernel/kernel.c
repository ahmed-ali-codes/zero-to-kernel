/* ============================================================================
 * kernel/kernel.c — MiniOS Kernel Entry Point
 *
 * Purpose : kernel_main() is the C entry point called by boot.asm after the
 *           Multiboot bootloader (GRUB) loads the kernel. It initialises all
 *           subsystems in the correct order and then launches the shell.
 *
 * Boot sequence:
 *   1. Screen driver     — so we can print anything
 *   2. Print boot banner
 *   3. Memory manager    — heap must exist before GDT/IDT (uses memset)
 *   4. GDT               — protected mode segments
 *   5. IDT               — interrupt gates
 *   6. PIC               — remap hardware IRQs away from exceptions
 *   7. Keyboard driver   — register IRQ1 handler
 *   8. Filesystem        — clear the in-memory file table
 *   9. Shell             — enter the command loop (never returns)
 * ============================================================================ */

#include "../include/screen.h"
#include "../include/memory.h"
#include "../include/gdt.h"
#include "../include/idt.h"
#include "../include/pic.h"
#include "../include/keyboard.h"
#include "../include/mouse.h"
#include "../include/filesystem.h"
#include "../include/shell.h"
#include "../include/timer.h"

/* --------------------------------------------------------------------------
 * Banner (printed before shell starts)
 * -------------------------------------------------------------------------- */
static void print_banner(void) {
    terminal_setcolor(COLOR_CYAN, COLOR_BLACK);
    terminal_writestring("  __  __ _       _  ___  ____\n");
    terminal_writestring(" |  \\/  (_)_ __ (_)/ _ \\/ ___|\n");
    terminal_writestring(" | |\\/| | | '_ \\| | | | \\___ \\\n");
    terminal_writestring(" | |  | | | | | | | |_| |___) |\n");
    terminal_writestring(" |_|  |_|_|_| |_|_|\\___/|____/\n");
    terminal_writestring("\n");

    terminal_setcolor(COLOR_WHITE, COLOR_BLACK);
    terminal_writestring(" MiniOS v1.0 - A minimal x86 OS\n");
    terminal_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
    terminal_writestring(" Built by Ahmed Ali | C99 + x86 ASM | GRUB + QEMU\n");
    terminal_writestring(" ------------------------------------------------\n");
    terminal_setcolor(COLOR_WHITE, COLOR_BLACK);
}

/* --------------------------------------------------------------------------
 * Subsystem initialisation status printer
 * -------------------------------------------------------------------------- */
static void ok(const char *name) {
    terminal_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
    terminal_writestring(" [");
    terminal_setcolor(COLOR_LIGHT_GREEN, COLOR_BLACK);
    terminal_writestring(" OK ");
    terminal_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
    terminal_writestring("] ");
    terminal_setcolor(COLOR_WHITE, COLOR_BLACK);
    terminal_writestring(name);
    terminal_putchar('\n');
}

/* --------------------------------------------------------------------------
 * kernel_main — called from boot.asm
 *
 * Parameters passed by GRUB (from boot.asm):
 *   magic — should be 0x2BADB002 (Multiboot magic)
 *   mbi   — pointer to multiboot_info_t struct (unused in v1.0)
 * -------------------------------------------------------------------------- */
void kernel_main(unsigned int magic, unsigned int mbi) {
    (void)magic;
    (void)mbi;

    /* 1. Screen — must be first so all subsequent prints work */
    terminal_init();
    terminal_clear();

    /* 2. Boot banner */
    print_banner();
    terminal_writestring("\n Initialising subsystems...\n\n");

    /* 3. Memory manager */
    memory_init();
    ok("Memory manager (256 KiB heap)");

    /* 4. GDT */
    gdt_install();
    ok("Global Descriptor Table (GDT)");

    /* 5. IDT */
    idt_install();
    ok("Interrupt Descriptor Table (IDT)");

    /* 6. PIC — remap IRQs 0-15 to vectors 32-47 */
    pic_init();
    ok("8259 PIC (IRQs remapped to 0x20-0x2F)");

    /* 7. Timer */
    timer_install(100);
    ok("Programmable Interval Timer (100 Hz)");

    /* 8. Keyboard */
    keyboard_install();
    ok("PS/2 Keyboard driver (IRQ1)");

    /* 9. Mouse */
    mouse_install();
    ok("PS/2 Mouse driver with scroll wheel (IRQ12)");

    /* 10. Filesystem */
    fs_init();
    ok("In-memory filesystem (16 files x 256 B)");

    /* Boot complete */
    terminal_writestring("\n");
    terminal_setcolor(COLOR_YELLOW, COLOR_BLACK);
    terminal_writestring(" Boot complete. Type 'help' for available commands.\n");
    terminal_setcolor(COLOR_WHITE, COLOR_BLACK);

    /* 11. Shell — enters infinite command loop, never returns */
    shell_run();

    /* Should never reach here */
    for (;;) {
        __asm__ volatile ("hlt");
    }
}
