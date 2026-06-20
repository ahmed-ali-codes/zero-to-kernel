# ============================================================================
# MiniOS Makefile
#
# Purpose : Builds the MiniOS kernel from C and Assembly sources using the
#           i686-elf cross-compiler, then creates a bootable ISO with GRUB.
#
# Targets:
#   make         — build iso/minios.iso
#   make iso     — same as above
#   make run     — build and launch in QEMU
#   make clean   — remove all build output
#   make debug   — launch in QEMU with GDB stub on port 1234
#
# Requirements (install via bootstrap.sh):
#   i686-elf-gcc   cross-compiler
#   i686-elf-ld    cross-linker
#   nasm           assembler
#   qemu-system-i386 (or x86_64)
#   grub-mkrescue + xorriso
# ============================================================================

# --------------------------------------------------------------------------
# Toolchain
# --------------------------------------------------------------------------
CC       := i686-elf-gcc
LD       := i686-elf-ld
NASM     := nasm
OBJCOPY  := i686-elf-objcopy

# Homebrew installs GRUB tools as i686-elf-grub-mkrescue on macOS
GRUB_MKRESCUE := $(shell command -v i686-elf-grub-mkrescue 2>/dev/null || \
                          command -v grub-mkrescue 2>/dev/null || \
                          echo grub-mkrescue)

QEMU     := $(shell command -v qemu-system-i386 2>/dev/null || \
                     command -v qemu-system-x86_64 2>/dev/null || \
                     echo qemu-system-i386)

# --------------------------------------------------------------------------
# Directories
# --------------------------------------------------------------------------
BUILD_DIR := build
ISO_DIR   := iso

# --------------------------------------------------------------------------
# Compiler / Assembler flags
# --------------------------------------------------------------------------
CFLAGS := \
    -std=c99            \
    -ffreestanding      \
    -O2                 \
    -Wall               \
    -Wextra             \
    -Iinclude           \
    -fno-stack-protector\
    -fno-pie            \
    -m32

NASMFLAGS := -f elf32

LDFLAGS := \
    -T linker.ld        \
    -melf_i386          \
    --oformat=elf32-i386

# --------------------------------------------------------------------------
# Source files
# --------------------------------------------------------------------------
ASM_SRCS := \
    boot/boot.asm       \
    boot/interrupts.asm

C_SRCS := \
    kernel/kernel.c     \
    kernel/screen.c     \
    kernel/memory.c     \
    kernel/gdt.c        \
    kernel/idt.c        \
    kernel/pic.c        \
    kernel/timer.c      \
    kernel/keyboard.c   \
    kernel/mouse.c      \
    kernel/editor.c     \
    kernel/shell.c      \
    kernel/filesystem.c

# --------------------------------------------------------------------------
# Object files
# --------------------------------------------------------------------------
ASM_OBJS := $(patsubst %.asm,  $(BUILD_DIR)/%.o, $(ASM_SRCS))
C_OBJS   := $(patsubst %.c,    $(BUILD_DIR)/%.o, $(C_SRCS))
ALL_OBJS := $(ASM_OBJS) $(C_OBJS)

KERNEL_ELF := $(BUILD_DIR)/kernel.elf
KERNEL_BIN := $(BUILD_DIR)/kernel.bin

# --------------------------------------------------------------------------
# Default target
# --------------------------------------------------------------------------
.PHONY: all
all: iso

# --------------------------------------------------------------------------
# Build the ISO
# --------------------------------------------------------------------------
.PHONY: iso
iso: $(ISO_DIR)/minios.iso

$(ISO_DIR)/minios.iso: $(KERNEL_BIN)
	@echo "  [ISO]  Building bootable ISO..."
	@mkdir -p $(BUILD_DIR)/iso/boot/grub
	@cp $(KERNEL_BIN) $(BUILD_DIR)/iso/boot/kernel.bin
	@cp grub/grub.cfg $(BUILD_DIR)/iso/boot/grub/grub.cfg
	@mkdir -p $(ISO_DIR)
	@$(GRUB_MKRESCUE) -o $(ISO_DIR)/minios.iso $(BUILD_DIR)/iso 2>/dev/null
	@echo "  [ISO]  Created: $(ISO_DIR)/minios.iso"

# --------------------------------------------------------------------------
# Link the kernel ELF
# --------------------------------------------------------------------------
$(KERNEL_ELF): $(ALL_OBJS) linker.ld
	@echo "  [LD]   Linking kernel ELF..."
	@$(LD) $(LDFLAGS) -o $@ $(ALL_OBJS)

# --------------------------------------------------------------------------
# Strip to flat binary (GRUB Multiboot works with ELF directly, but
# providing the ELF is cleaner; keep .bin as an alias target)
# --------------------------------------------------------------------------
$(KERNEL_BIN): $(KERNEL_ELF)
	@cp $(KERNEL_ELF) $(KERNEL_BIN)
	@echo "  [BIN]  kernel.bin ready"

# --------------------------------------------------------------------------
# Compile C sources
# --------------------------------------------------------------------------
$(BUILD_DIR)/kernel/%.o: kernel/%.c
	@mkdir -p $(dir $@)
	@echo "  [CC]   $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# --------------------------------------------------------------------------
# Assemble ASM sources
# --------------------------------------------------------------------------
$(BUILD_DIR)/boot/%.o: boot/%.asm
	@mkdir -p $(dir $@)
	@echo "  [NASM] $<"
	@$(NASM) $(NASMFLAGS) $< -o $@

# --------------------------------------------------------------------------
# Run in QEMU
# --------------------------------------------------------------------------
.PHONY: run
run: iso
	@echo "  [QEMU] Booting MiniOS..."
	@$(QEMU) -cdrom $(ISO_DIR)/minios.iso -m 32M -serial stdio \
	         -vga std \
	         -display cocoa,zoom-to-fit=on

# --------------------------------------------------------------------------
# Debug: QEMU with GDB stub
# --------------------------------------------------------------------------
.PHONY: debug
debug: iso
	@echo "  [QEMU] Debug mode — GDB on port 1234"
	@echo "         Connect with: gdb build/kernel.elf"
	@echo "         Then:         target remote :1234"
	@$(QEMU) -cdrom $(ISO_DIR)/minios.iso -m 32M \
	         -vga std \
	         -display cocoa,zoom-to-fit=on \
	         -d int,cpu_reset \
	         -s -S

# --------------------------------------------------------------------------
# Clean
# --------------------------------------------------------------------------
.PHONY: clean
clean:
	@echo "  [CLN]  Removing build output..."
	@rm -rf $(BUILD_DIR) $(ISO_DIR)
	@echo "  [CLN]  Done."
