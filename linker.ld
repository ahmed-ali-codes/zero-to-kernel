/* ============================================================================
 * linker.ld — MiniOS Linker Script
 *
 * Purpose : Tells the linker how to lay out the kernel binary in memory.
 *           GRUB expects the kernel to start at 1 MiB (0x100000) when using
 *           the Multiboot specification.
 *
 * Layout:
 *   0x100000  .text    — executable code (kernel + boot.o)
 *             .rodata  — read-only data (strings, tables)
 *             .data    — initialised global/static data
 *             .bss     — uninitialised data (zeroed by boot.asm/startup)
 *
 * The Multiboot header in boot.asm must appear in the first 8 KiB of the
 * ELF file, so boot.o is listed first in the .text section.
 * ============================================================================ */

ENTRY(_start)

SECTIONS {
    /* Kernel loads at 1 MiB — standard for Multiboot kernels */
    . = 1M;

    /* --------------------------------------------------------------------- */
    /* .text — code                                                           */
    /* --------------------------------------------------------------------- */
    .text BLOCK(4K) : ALIGN(4K) {
        *(.multiboot)          /* Multiboot header — MUST be first */
        *(.text)
    }

    /* --------------------------------------------------------------------- */
    /* .rodata — read-only data                                               */
    /* --------------------------------------------------------------------- */
    .rodata BLOCK(4K) : ALIGN(4K) {
        *(.rodata)
    }

    /* --------------------------------------------------------------------- */
    /* .data — initialised data                                               */
    /* --------------------------------------------------------------------- */
    .data BLOCK(4K) : ALIGN(4K) {
        *(.data)
    }

    /* --------------------------------------------------------------------- */
    /* .bss — uninitialised data (zeroed on startup)                         */
    /* --------------------------------------------------------------------- */
    .bss BLOCK(4K) : ALIGN(4K) {
        *(COMMON)
        *(.bss)
    }
}
