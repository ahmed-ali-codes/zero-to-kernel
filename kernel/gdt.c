/* ============================================================================
 * kernel/gdt.c — Global Descriptor Table
 *
 * Purpose : Sets up the x86 GDT with three descriptors:
 *             0: Null descriptor (required by CPU spec)
 *             1: Kernel Code segment (ring 0, execute/read)
 *             2: Kernel Data segment (ring 0, read/write)
 *
 *           Both code and data segments span the full 4 GiB address space
 *           (flat memory model). gdt_flush() is implemented in interrupts.asm.
 * ============================================================================ */

#include "../include/gdt.h"
#include "../include/memory.h"  /* memset */

/* --------------------------------------------------------------------------
 * GDT storage (3 entries)
 * -------------------------------------------------------------------------- */
#define GDT_NUM_ENTRIES  3

static gdt_entry_t gdt_entries[GDT_NUM_ENTRIES];
static gdt_ptr_t   gdt_ptr;

/* --------------------------------------------------------------------------
 * Internal helper — fill a GDT entry
 * -------------------------------------------------------------------------- */
static void gdt_set_gate(int idx, uint32_t base, uint32_t limit,
                         uint8_t access, uint8_t gran) {
    gdt_entries[idx].base_low    = (base  & 0xFFFF);
    gdt_entries[idx].base_middle = (base  >> 16) & 0xFF;
    gdt_entries[idx].base_high   = (base  >> 24) & 0xFF;
    gdt_entries[idx].limit_low   = (limit & 0xFFFF);
    gdt_entries[idx].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt_entries[idx].access      = access;
}

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */
void gdt_install(void) {
    gdt_ptr.limit = (sizeof(gdt_entry_t) * GDT_NUM_ENTRIES) - 1;
    gdt_ptr.base  = (uint32_t)&gdt_entries;

    /* 0: Null descriptor */
    gdt_set_gate(0, 0, 0, 0, 0);

    /* 1: Kernel Code segment
     *    Base=0, Limit=4GiB, Access=0x9A (present|ring0|code|executable|readable)
     *    Gran =0xCF (4-KiB granularity, 32-bit protected mode) */
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

    /* 2: Kernel Data segment
     *    Base=0, Limit=4GiB, Access=0x92 (present|ring0|data|writable)
     *    Gran =0xCF */
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    gdt_flush((uint32_t)&gdt_ptr);
}
