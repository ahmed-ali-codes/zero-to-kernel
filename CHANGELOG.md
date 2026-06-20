# Changelog

All notable changes to the **MiniOS** operating system will be documented in this file. The project adheres to Semantic Versioning (`Major.Minor.Patch`).

---

## [1.0.0] - 2026-06-12

### Added
- **MiniFS Directory Structure**: Added support for hierarchical directory nesting (`/`, `/root`, and user directories) using path-resolution routines.
- **Nested Filesystem Commands**: Added `mkd` (make directory), `deld` (delete directory), `wdir` (working directory), `goto` (change directory), and `dtree` (print filesystem tree).
- **MiniEdit Text Editor**: A built-in terminal text editor with fullscreen screen refreshing, character insertion, Backspace deletion, and saving changes upon pressing `ESC`.
- **PS/2 Mouse Scroll Support**: Added mouse initialization code using IntelliMouse command sequences; enabled console scrollback using the scroll wheel.
- **Programmable Interval Timer (PIT)**: Added PIT timer configuration (100 Hz frequency interrupts on IRQ0) for tracking uptime metrics.
- **Expanded Command Line Utilities**:
  - `calc` (addition, subtraction, multiplication, division math expression parsing).
  - `passgen` (pseudo-random password generation with character filters).
  - `todo` (in-memory todo list task tracker).
  - `notes` (convenience notes saver).
  - `sysinfo` (system uptime, memory availability, and command statistics dashboard).
  - `color` (runtime VGA console font coloring change).

---

## [0.5.0] - 2026-05-20

### Added
- **Dynamic Heap Memory Manager**: Implemented First-Fit memory block allocator (`kmalloc` and `kfree`) with memory coalescing to prevent block fragmentation.
- **Memory Diagnostic Utility**: Added the `memo` shell command displaying dynamic allocated/free heap memory metrics.
- **Standard Memory Utilities**: Written freestanding C utilities for `memset`, `memcpy`, `memcmp`, and string utilities (`strlen`, `strcmp`, `strcpy`, `atoi`, etc.).

---

## [0.4.0] - 2026-04-15

### Added
- **Command Line Shell Interface**: Added interactive prompt (`>`), input buffer processing, command dispatcher, and history tracking.
- **Shell Commands**: Built-in shell commands including `cmds` (help menu), `osinfo` (version/author details), `cls` (clear terminal display), and `wrt` (string echo console).

---

## [0.3.0] - 2026-03-25

### Added
- **Descriptor Tables**: Implemented 32-bit Global Descriptor Table (GDT) and Interrupt Descriptor Table (IDT) configuration.
- **PIC Remapping**: Programmed dual 8259A PIC chips, remapping hardware interrupts away from exception vectors to vectors `32-47`.
- **Keyboard Driver**: Keyboard interrupt handler routine hooked onto IRQ1. Maps hardware scan codes to US QWERTY characters.

---

## [0.2.0] - 2026-02-10

### Added
- **VGA Text Screen Driver**: Wraps direct write access to VGA memory `0xB8000`. Supports cursor positioning, scrolling, text colors, and `printf` formatted string writing.

---

## [0.1.0] - 2026-01-05

### Added
- **Kernel Boot Code**: Assembly file `boot.asm` with Multiboot signature validation, stack configuration, and switch to C entry point.
- **Build Pipeline**: Created initial compiler configurations and linker scripts compiling the kernel to x86 ELF formats. Bootable ISO generation using GRUB 2 and `grub-mkrescue`.
