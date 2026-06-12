#!/usr/bin/env bash
# ============================================================================
# bootstrap.sh — MiniOS Toolchain Installer (macOS)
#
# Purpose : Installs the complete toolchain needed to build and run MiniOS
#           on a macOS system. Run this once before your first `make run`.
#
# What it installs:
#   - Homebrew (if not present)
#   - NASM          — assembler for boot.asm and interrupts.asm
#   - QEMU          — x86 emulator to run the OS
#   - i686-elf-gcc  — cross-compiler via the OSDev Homebrew tap
#   - xorriso       — ISO creation library (needed by grub-mkrescue)
#   - GRUB tools    — grub-mkrescue for building the bootable ISO
#
# Usage:
#   chmod +x bootstrap.sh
#   ./bootstrap.sh
#
# After install:
#   make run
# ============================================================================

set -e  # Exit immediately on any error

# Colours
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
CYAN='\033[0;36m'; NC='\033[0m'

ok()   { echo -e "${GREEN}  [OK]${NC}  $1"; }
info() { echo -e "${CYAN}  [..]${NC}  $1"; }
warn() { echo -e "${YELLOW}  [!!]${NC}  $1"; }
fail() { echo -e "${RED}  [ERR]${NC} $1"; exit 1; }

echo ""
echo -e "${CYAN}============================================${NC}"
echo -e "${CYAN}  MiniOS Bootstrap — Toolchain Installer    ${NC}"
echo -e "${CYAN}============================================${NC}"
echo ""

# --------------------------------------------------------------------------
# 1. Homebrew
# --------------------------------------------------------------------------
if ! command -v brew &>/dev/null; then
    info "Homebrew not found — installing..."
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    ok "Homebrew installed"
else
    ok "Homebrew already installed: $(brew --version | head -1)"
fi

# --------------------------------------------------------------------------
# 2. NASM
# --------------------------------------------------------------------------
if ! command -v nasm &>/dev/null; then
    info "Installing NASM..."
    brew install nasm
    ok "NASM installed: $(nasm --version | head -1)"
else
    ok "NASM already installed: $(nasm --version | head -1)"
fi

# --------------------------------------------------------------------------
# 3. QEMU
# --------------------------------------------------------------------------
if ! command -v qemu-system-i386 &>/dev/null && ! command -v qemu-system-x86_64 &>/dev/null; then
    info "Installing QEMU..."
    brew install qemu
    ok "QEMU installed"
else
    ok "QEMU already installed"
fi

# --------------------------------------------------------------------------
# 4. xorriso (required by grub-mkrescue)
# --------------------------------------------------------------------------
if ! command -v xorriso &>/dev/null; then
    info "Installing xorriso..."
    brew install xorriso
    ok "xorriso installed"
else
    ok "xorriso already installed"
fi

# --------------------------------------------------------------------------
# 5. i686-elf cross-compiler (via OSDev Homebrew tap)
# --------------------------------------------------------------------------
if ! command -v i686-elf-gcc &>/dev/null; then
    info "Adding OSDev Homebrew tap (i686-elf toolchain)..."
    brew tap nativeos/i686-elf-toolchain 2>/dev/null || true
    info "Installing i686-elf-binutils..."
    brew install i686-elf-binutils 2>/dev/null || true
    info "Installing i686-elf-gcc..."
    brew install i686-elf-gcc 2>/dev/null || true

    if ! command -v i686-elf-gcc &>/dev/null; then
        warn "Homebrew tap may not have worked. Trying alternative..."
        # Fallback: try the philippzu tap
        brew tap philippzu/osdev-tools 2>/dev/null || true
        brew install philippzu/osdev-tools/i686-elf-toolchain 2>/dev/null || true
    fi

    if command -v i686-elf-gcc &>/dev/null; then
        ok "i686-elf-gcc installed: $(i686-elf-gcc --version | head -1)"
    else
        warn "Auto-install failed. Manual steps:"
        echo ""
        echo "  Option A — Build from source (OSDev Wiki):"
        echo "    https://wiki.osdev.org/GCC_Cross-Compiler"
        echo ""
        echo "  Option B — Pre-built binary (recommended for macOS):"
        echo "    brew install --cask gcc-arm-embedded  (ARM, as reference)"
        echo "    or download from: https://github.com/lordmilko/i686-elf-tools/releases"
        echo "    then add to PATH."
        echo ""
        echo "  After manual install, run: make iso && make run"
        exit 1
    fi
else
    ok "i686-elf-gcc already installed: $(i686-elf-gcc --version | head -1)"
fi

# --------------------------------------------------------------------------
# 6. GRUB tools (grub-mkrescue)
# --------------------------------------------------------------------------
if ! command -v grub-mkrescue &>/dev/null; then
    info "Installing GRUB tools..."
    brew install grub 2>/dev/null || true

    if ! command -v grub-mkrescue &>/dev/null; then
        # Try i386-elf-grub via OSDev tap
        brew install i386-elf-grub 2>/dev/null || true
    fi

    if command -v grub-mkrescue &>/dev/null; then
        ok "grub-mkrescue installed"
    else
        warn "grub-mkrescue not found after install."
        warn "Try: brew install --cask grub"
        warn "Or:  brew install i386-elf-grub"
    fi
else
    ok "grub-mkrescue already installed"
fi

# --------------------------------------------------------------------------
# Summary
# --------------------------------------------------------------------------
echo ""
echo -e "${CYAN}============================================${NC}"
echo -e "${GREEN}  Toolchain setup complete!                 ${NC}"
echo -e "${CYAN}============================================${NC}"
echo ""
echo "  Build and run MiniOS:"
echo -e "    ${YELLOW}cd MiniOS${NC}"
echo -e "    ${YELLOW}make run${NC}"
echo ""
echo "  Other targets:"
echo "    make clean   — remove build artefacts"
echo "    make debug   — QEMU + GDB stub on port 1234"
echo ""
