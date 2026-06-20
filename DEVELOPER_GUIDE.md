# MiniOS Developer Guide

Welcome to the **MiniOS Developer Guide**. This document outlines how to set up the development environment, build the kernel, run tests, and debug low-level kernel routines.

---

## 1. Toolchain Setup

To compile MiniOS, you need an x86 cross-compiler to prevent your host machine's compiler from linking host-system standard libraries (like glibc) or emitting host-specific structures.

### macOS Configuration

We provide a script, `bootstrap.sh`, which automatically downloads and configures the required GNU tools via Homebrew:

```bash
# Make the bootstrap script executable
chmod +x bootstrap.sh

# Run the installation
./bootstrap.sh
```

This installs:
- **`i686-elf-gcc`**: GCC targeting the 32-bit ELF format.
- **`nasm`**: Netwide Assembler for boot code and interrupt handlers.
- **`qemu-system-i386`**: Emulator for hardware execution.
- **`xorriso`** and **`grub-mkrescue`**: Utilities to build bootable ISOs.

---

## 2. Build Pipeline

The GNU Make script coordinates compilation. Below are the key build recipes:

```bash
# 1. Build the bootable ISO
make

# 2. Build the ISO and launch in QEMU immediately
make run

# 3. Clean all build caches and targets
make clean
```

### Compiler Configurations (Makefile Flags)
- `-std=c99`: Targets the ISO C99 standard.
- `-ffreestanding`: Indicates a standalone system where standard library headers might not exist.
- `-fno-stack-protector`: Disables stack-smashing defenses (which rely on host OS support).
- `-fno-pie` / `-m32`: Generates a flat 32-bit execution model (not Position-Independent).

---

## 3. Low-level Debugging with QEMU & GDB

Debugging kernels requires intercepting CPU behavior directly. MiniOS sets up a debugging environment using a GDB remote connection.

### 3.1. Launching QEMU with GDB server

Run:
```bash
make debug
```

This commands launches QEMU with:
- `-s`: Opens a gdbserver port on TCP `1234`.
- `-S`: Freezes the virtual CPU at startup, waiting for a debugger connection.
- `-d int,cpu_reset`: Logs interrupts and CPU resets to screen/console.

### 3.2. Connecting GDB

In a separate terminal window, launch the debugger pointing to the symbols inside `kernel.elf`:

```bash
gdb build/kernel.elf
```

Inside the GDB command line, hook onto the emulator:

```gdb
# Connect to the QEMU debug server
(gdb) target remote :1234

# Set a breakpoint inside C entry function
(gdb) break kernel_main

# Resume execution
(gdb) continue
```

### 3.3. Key Debugging Commands

| GDB Command | Description |
|---|---|
| `info registers` | Show contents of EAX, EBX, ECX, EDX, CR0, CR3, etc. |
| `x/10i $eip` | Disassemble next 10 assembly instructions at the instruction pointer. |
| `x/4xw 0xB8000` | Inspect memory-mapped VGA screen buffers. |
| `step` / `next` | Single-step execution (step into vs step over). |
| `layout asm` / `layout src` | Enable visual split views for assembly and source code. |

---

## 4. Coding & Documentation Guidelines

When modifying the kernel or adding drivers, adhere to these guidelines:

1. **Header Protection**: Always use header guards `#ifndef HEADER_NAME_H` for all files in the `include/` directory.
2. **Comment Structure**: Document public functions using descriptive headers in C implementation files:
   ```c
   /**
    * @brief Allocates dynamic heap space.
    * 
    * @param size Number of bytes to allocate.
    * @return void* Pointer to the allocated buffer, or NULL if out of memory.
    */
   void *kmalloc(size_t size);
   ```
3. **Hardware Boundaries**: Never perform direct I/O port writes outside of wrapper interfaces (`outb`, `inb`, `io_wait` defined in [pic.h](file:///Users/apple/Documents/Ecotrustia-data/Own-projects-planning/mini-os/MiniOS/include/pic.h)).
