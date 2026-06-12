; ============================================================================
; boot/boot.asm — MiniOS Entry Point
;
; Purpose : x86 Assembly entry point for MiniOS. Sets up the Multiboot
;           header so GRUB knows how to load us, establishes a 16 KiB
;           stack, and hands control to kernel_main() in C.
;
; Standard: Multiboot 1 (as specified by GRUB 2 legacy multiboot support)
; Assembled with NASM.
; ============================================================================

; --------------------------------------------------------------------------
; Multiboot Header Constants
; --------------------------------------------------------------------------
MBALIGN     equ 1 << 0              ; align loaded modules on page boundaries
MEMINFO     equ 1 << 1              ; provide memory map
FLAGS       equ MBALIGN | MEMINFO   ; multiboot flags
MAGIC       equ 0x1BADB002          ; multiboot magic number
CHECKSUM    equ -(MAGIC + FLAGS)    ; checksum: magic + flags + checksum = 0

; --------------------------------------------------------------------------
; Multiboot Header — GRUB looks for this in the first 8 KiB of the kernel
; --------------------------------------------------------------------------
section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

; --------------------------------------------------------------------------
; BSS — uninitialised data (stack lives here)
; --------------------------------------------------------------------------
section .bss
align 16
stack_bottom:
    resb 16384          ; 16 KiB kernel stack
stack_top:

; --------------------------------------------------------------------------
; Text — executable code
; --------------------------------------------------------------------------
section .text
global _start:function (_start.end - _start)

_start:
    ; Set up the stack. x86 stacks grow downward, so we point ESP at the
    ; top of our reserved stack region.
    mov esp, stack_top

    ; Optional: push multiboot info (eax = magic, ebx = info struct pointer)
    ; These will be useful if you later want to read the memory map.
    push ebx            ; multiboot_info_t *
    push eax            ; multiboot magic

    ; Call the C kernel entry point. It should never return.
    extern kernel_main
    call kernel_main

    ; If kernel_main somehow returns, disable interrupts and halt forever.
.hang:
    cli
    hlt
    jmp .hang
.end:
