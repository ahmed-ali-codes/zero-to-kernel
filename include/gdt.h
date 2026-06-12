/* ============================================================================
 * include/gdt.h — Global Descriptor Table Interface
 *
 * Purpose : Public API for GDT setup. The GDT defines memory segments
 *           (code + data) in protected mode. We set up three descriptors:
 *           null, kernel code (ring 0), and kernel data (ring 0).
 * ============================================================================ */

#ifndef GDT_H
#define GDT_H

#include <stdint.h>

/* --------------------------------------------------------------------------
 * GDT Entry (segment descriptor) — 8 bytes
 * -------------------------------------------------------------------------- */
typedef struct __attribute__((packed)) {
    uint16_t limit_low;     /* lower 16 bits of segment limit    */
    uint16_t base_low;      /* lower 16 bits of base address     */
    uint8_t  base_middle;   /* bits 16-23 of base address        */
    uint8_t  access;        /* access flags (type, DPL, present) */
    uint8_t  granularity;   /* granularity + upper 4 bits limit  */
    uint8_t  base_high;     /* upper 8 bits of base address      */
} gdt_entry_t;

/* --------------------------------------------------------------------------
 * GDT Pointer — loaded with LGDT instruction
 * -------------------------------------------------------------------------- */
typedef struct __attribute__((packed)) {
    uint16_t limit;         /* size of GDT in bytes minus 1      */
    uint32_t base;          /* linear address of GDT             */
} gdt_ptr_t;

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

/* Install the GDT and flush the segment registers */
void gdt_install(void);

/* Assembly helper — loads GDTR and reloads segment registers */
extern void gdt_flush(uint32_t gdt_ptr_addr);

/* Segment selector constants */
#define GDT_KERNEL_CODE_SEG  0x08
#define GDT_KERNEL_DATA_SEG  0x10

#endif /* GDT_H */
