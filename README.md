# MiniOS

A minimal, text-mode x86 operating system built from scratch in C99 and x86 Assembly.
Boots via GRUB 2 and runs inside the QEMU emulator.

**Author**: Ahmed Ali  
**Stack**: C99 + x86 Assembly (NASM) · GRUB 2 · QEMU  
**Target**: x86 (i686), 32-bit protected mode

---

## Features (v1.0)

| Subsystem | Details |
|---|---|
| Bootloader | GRUB 2 (Multiboot spec) |
| Screen driver | VGA text buffer (0xB8000), 80×25, 16 colours, auto-scroll, printf |
| GDT | Flat memory model — null, kernel code, kernel data |
| IDT | 32 CPU exception handlers + 16 hardware IRQ gates |
| PIC | 8259A remapped (IRQ 0-15 → vectors 32-47) |
| Keyboard | IRQ1, PS/2 scancode set 1, US QWERTY, ring buffer, Shift/CapsLock |
| Shell | 9 built-in commands with colour output |
| Memory | 256 KiB heap, block-list allocator, kmalloc/kfree |
| Filesystem | Flat in-memory file table — 16 files × 256 bytes |

---

## Project Structure

```
MiniOS/
├── boot/
│   ├── boot.asm          ; Multiboot header + stack + _start entry
│   └── interrupts.asm    ; ISR/IRQ assembly stubs + gdt_flush/idt_flush
├── kernel/
│   ├── kernel.c          ; kernel_main() — initialises all subsystems
│   ├── screen.c          ; VGA text mode driver
│   ├── memory.c          ; Heap allocator + string/memory utilities
│   ├── gdt.c             ; Global Descriptor Table
│   ├── idt.c             ; Interrupt Descriptor Table + dispatchers
│   ├── pic.c             ; 8259A PIC driver
│   ├── keyboard.c        ; PS/2 keyboard IRQ1 driver
│   ├── shell.c           ; Interactive command shell
│   └── filesystem.c      ; In-memory flat filesystem
├── include/              ; Header files for all modules
├── grub/
│   └── grub.cfg          ; GRUB menu entry
├── linker.ld             ; Linker script (kernel at 1 MiB)
├── Makefile              ; Build, clean, run, debug targets
├── bootstrap.sh          ; One-shot toolchain installer (macOS)
└── README.md
```

---

## Quick Start

### 1. Install the toolchain (macOS)

```bash
chmod +x bootstrap.sh
./bootstrap.sh
```

This installs: `i686-elf-gcc`, `nasm`, `qemu`, `xorriso`, `grub-mkrescue` via Homebrew.

### 2. Build and run

```bash
make run
```

MiniOS will boot in a QEMU window and present a shell prompt.

### 3. Manual tool verification

```bash
i686-elf-gcc --version    # should print GCC cross-compiler info
nasm --version            # NASM 2.x or higher
qemu-system-i386 --version
grub-mkrescue --version
```

---

## Makefile Targets

| Target | Description |
|---|---|
| `make` / `make iso` | Build `iso/minios.iso` |
| `make run` | Build ISO and launch in QEMU |
| `make debug` | QEMU with GDB stub on port 1234 |
| `make clean` | Remove all build output |

---

## Shell Commands

Once MiniOS boots, you'll see a `>` prompt. Available commands:

```
cmds          — show this list
osinfo        — show OS info and author
cls           — clear the screen
wrt [text]    — print text back to screen
memo          — show heap usage stats
new [file]    — create a new file
write  [file] — prompt for text and write it to the file
see [file]    — display file contents
laf           — list all files
del [file]    — permanently deletes a file
cpy [src] [dst] — copy a file
mov [src] [dst] — move / rename a file
hunt [file] [word] — search in file
cnt [file]    — count words/lines
flip          — flip a coin
dice          — roll a dice
```

### Example session

```
MiniOS v1.0 — Type 'cmds' for available commands.

> wrt hello from MiniOS
hello from MiniOS

> new notes
Created: notes

> write notes
Enter text: This is my first OS file
Written to notes (23 bytes)

> laf
  notes                     (23 bytes)
  1 file(s)

> memo
Heap total : 262144 bytes
Heap used  : 24 bytes
Heap free  : 262120 bytes

> osinfo
  MiniOS v1.0
  Built by Ahmed Ali
  Stack: C99 + x86 Assembly, GRUB 2, QEMU

> xyz
Unknown command: 'xyz'
Type 'cmds' for a list of commands.
```

---

## Debugging

### Triple-fault / silent crash

```bash
make debug
# In another terminal:
gdb build/kernel.elf
(gdb) target remote :1234
(gdb) continue
```

Or add QEMU debug flags directly:

```bash
qemu-system-i386 -cdrom iso/minios.iso -d int,cpu_reset -no-reboot
```

### GDB tips

```gdb
(gdb) info registers          # all CPU registers
(gdb) x/10i $eip              # disassemble 10 instructions at EIP
(gdb) x/4xw 0x100000          # inspect memory at kernel load address
(gdb) break kernel_main        # set breakpoint
```

---

## Learning Milestones

| Version | Milestone |
|---|---|
| v0.1 | Bootable kernel — "proof of life" VGA write |
| v0.2 | Screen driver with colour, scroll, printf |
| v0.3 | GDT + IDT + PIC + PS/2 keyboard IRQ |
| v0.4 | Interactive shell with 4 commands |
| v0.5 | Heap allocator (kmalloc/kfree) + `mem` |
| v1.0 | In-memory filesystem + `create/write/read/ls` |

---

## References

- [OSDev Wiki](https://wiki.osdev.org) — The definitive beginner reference
- [James Molloy's Kernel Dev Tutorial](http://www.jamesmolloy.co.uk/tutorial_html/) — GDT/IDT/keyboard
- [Bran's Kernel Dev Tutorial](http://www.osdever.net/bkerndev/) — VGA + shell
- [Nick Blundell — Writing a Simple OS from Scratch (PDF)](https://www.cs.bham.ac.uk/~exr/lectures/opsys/10_11/lectures/os-dev.pdf)
- [QEMU Documentation](https://www.qemu.org/docs/master/) — debug flags

---

## License

MIT — build it, break it, learn from it.
