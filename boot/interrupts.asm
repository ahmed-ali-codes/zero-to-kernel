; ============================================================================
; boot/interrupts.asm — ISR / IRQ Assembly Stubs
;
; Purpose : Provides the low-level entry points for all 32 CPU exception
;           handlers (ISR 0-31) and 16 hardware IRQ handlers (IRQ 0-15).
;           Each stub saves CPU state (via pusha), pushes the interrupt
;           number + error code, then calls the appropriate C dispatcher.
;           Also provides gdt_flush and idt_flush helpers.
;
; Called by: idt.c (installs stub addresses into IDT)
; Calls    : isr_handler() and irq_handler() in idt.c
; ============================================================================

[BITS 32]
[EXTERN isr_handler]
[EXTERN irq_handler]

; ---------------------------------------------------------------------------
; gdt_flush — load the new GDT and reload all segment registers
; Called from gdt.c: gdt_flush((uint32_t)&gdt_ptr);
; ---------------------------------------------------------------------------
[GLOBAL gdt_flush]
gdt_flush:
    mov eax, [esp+4]    ; get pointer to gdt_ptr struct
    lgdt [eax]          ; load GDT register

    ; Reload code segment via a far jump
    mov ax, 0x10        ; kernel data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:.flush     ; far jump to reload CS with kernel code selector
.flush:
    ret

; ---------------------------------------------------------------------------
; idt_flush — load the IDT register
; Called from idt.c: idt_flush((uint32_t)&idt_ptr);
; ---------------------------------------------------------------------------
[GLOBAL idt_flush]
idt_flush:
    mov eax, [esp+4]
    lidt [eax]
    ret

; ---------------------------------------------------------------------------
; Macro helpers
; ---------------------------------------------------------------------------

; ISR stub WITHOUT error code (CPU does not push one — we push a dummy 0)
%macro ISR_NO_ERR 1
[GLOBAL isr%1]
isr%1:
    cli
    push byte 0         ; dummy error code
    push byte %1        ; interrupt number
    jmp isr_common_stub
%endmacro

; ISR stub WITH error code (CPU already pushed one)
%macro ISR_ERR 1
[GLOBAL isr%1]
isr%1:
    cli
    push byte %1        ; interrupt number (error code already on stack)
    jmp isr_common_stub
%endmacro

; IRQ stub — remapped to IDT vectors 32-47
%macro IRQ 2
[GLOBAL irq%1]
irq%1:
    cli
    push byte 0         ; dummy error code
    push byte %2        ; IRQ number (as interrupt vector)
    jmp irq_common_stub
%endmacro

; ---------------------------------------------------------------------------
; CPU Exception ISRs (vectors 0-31)
; ---------------------------------------------------------------------------
ISR_NO_ERR  0   ; Divide By Zero
ISR_NO_ERR  1   ; Debug
ISR_NO_ERR  2   ; Non-Maskable Interrupt
ISR_NO_ERR  3   ; Breakpoint
ISR_NO_ERR  4   ; Overflow
ISR_NO_ERR  5   ; Bound Range Exceeded
ISR_NO_ERR  6   ; Invalid Opcode
ISR_NO_ERR  7   ; Device Not Available
ISR_ERR     8   ; Double Fault (has error code)
ISR_NO_ERR  9   ; Coprocessor Segment Overrun
ISR_ERR    10   ; Invalid TSS
ISR_ERR    11   ; Segment Not Present
ISR_ERR    12   ; Stack-Segment Fault
ISR_ERR    13   ; General Protection Fault
ISR_ERR    14   ; Page Fault
ISR_NO_ERR 15   ; Reserved
ISR_NO_ERR 16   ; x87 FPU Error
ISR_NO_ERR 17   ; Alignment Check
ISR_NO_ERR 18   ; Machine Check
ISR_NO_ERR 19   ; SIMD FP Exception
ISR_NO_ERR 20   ; Virtualisation Exception
ISR_NO_ERR 21   ; Reserved
ISR_NO_ERR 22   ; Reserved
ISR_NO_ERR 23   ; Reserved
ISR_NO_ERR 24   ; Reserved
ISR_NO_ERR 25   ; Reserved
ISR_NO_ERR 26   ; Reserved
ISR_NO_ERR 27   ; Reserved
ISR_NO_ERR 28   ; Reserved
ISR_NO_ERR 29   ; Reserved
ISR_NO_ERR 30   ; Security Exception
ISR_NO_ERR 31   ; Reserved

; ---------------------------------------------------------------------------
; Hardware IRQs (vectors 32-47, IRQs 0-15)
; ---------------------------------------------------------------------------
IRQ  0, 32   ; PIT Timer
IRQ  1, 33   ; PS/2 Keyboard
IRQ  2, 34   ; Cascade (slave PIC)
IRQ  3, 35   ; COM2
IRQ  4, 36   ; COM1
IRQ  5, 37   ; LPT2
IRQ  6, 38   ; Floppy Disk
IRQ  7, 39   ; LPT1 / Spurious
IRQ  8, 40   ; RTC
IRQ  9, 41   ; Free / ACPI
IRQ 10, 42   ; Free
IRQ 11, 43   ; Free
IRQ 12, 44   ; PS/2 Mouse
IRQ 13, 45   ; FPU
IRQ 14, 46   ; Primary ATA
IRQ 15, 47   ; Secondary ATA

; ---------------------------------------------------------------------------
; isr_common_stub — shared handler for CPU exceptions
; Stack at this point (bottom to top):
;   [ss, useresp, eflags, cs, eip]   <- pushed by CPU
;   [int_no, err_code]               <- pushed by our macro
; ---------------------------------------------------------------------------
isr_common_stub:
    pusha                   ; push edi, esi, ebp, esp, ebx, edx, ecx, eax
    mov ax, ds
    push eax                ; save data segment

    mov ax, 0x10            ; load kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp                ; pass pointer to registers_t to C handler
    call isr_handler
    add esp, 4              ; remove argument

    pop eax                 ; restore data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa
    add esp, 8              ; clean up int_no and err_code
    sti
    iret

; ---------------------------------------------------------------------------
; irq_common_stub — shared handler for hardware IRQs
; ---------------------------------------------------------------------------
irq_common_stub:
    pusha
    mov ax, ds
    push eax

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call irq_handler
    add esp, 4

    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa
    add esp, 8
    sti
    iret
