# MiniOS Testing Manual

This document details the environment setup, manual testing checklists, and subsystem verification steps used to ensure the reliability and correctness of MiniOS.

---

## 1. Test Environment

MiniOS runs in a bare-metal emulator. Tests should be run using the following host configurations:

```text
CPU Architecture : x86 (i386/i686 target)
Physical Memory  : 32 MiB (allocated via QEMU flag `-m 32M`)
Emulation Host   : QEMU emulator (v8.0 or later recommended)
Video Output     : Standard VGA Display (VGA text buffer at 0xB8000)
Input Hardware   : PS/2 Keyboard and PS/2 IntelliMouse
```

---

## 2. Kernel Boot Verification Checklist

Ensure the boot sequence runs correctly by verifying the boot sequence output:

1. **Clean compilation**: Verify that running `make clean && make` finishes with no compiler warnings or linker errors.
2. **Launch Time**: Run `make run`. The QEMU window must load the GRUB kernel and display the console shell prompt within **3 seconds**.
3. **Subsystem Logs**: Ensure each initialization block prints `[ OK ]` on the splash screen:
   - `[ OK ] Memory manager (256 KiB heap)`
   - `[ OK ] Global Descriptor Table (GDT)`
   - `[ OK ] Interrupt Descriptor Table (IDT)`
   - `[ OK ] 8259 PIC (IRQs remapped to 0x20-0x2F)`
   - `[ OK ] Programmable Interval Timer (100 Hz)`
   - `[ OK ] PS/2 Keyboard driver (IRQ1)`
   - `[ OK ] PS/2 Mouse driver with scroll wheel (IRQ12)`
   - `[ OK ] In-memory filesystem (16 files x 256 B)`

---

## 3. Shell Command Verification Suite

Test the terminal shell commands to ensure appropriate behavior:

| Command | Action | Expected Output | Status |
|---|---|---|---|
| `cls` | Clear Screen | The screen is wiped clean, and the cursor is placed back at `(0,0)`. | [ ] Pass |
| `wrt hello` | Echo input string | Output: `hello` | [ ] Pass |
| `osinfo` | Show OS Metadata | Output shows Author name (`Ahmed Ali`) and compiler tools stack. | [ ] Pass |
| `calc 10 * 5` | Mathematical Calculator | Output: `Result: 50` | [ ] Pass |
| `passgen 10` | Random Password Generator | Displays a 10-character alphanumeric string. | [ ] Pass |
| `flip` / `dice` | RNG tests | Random coin flip or dice number generated. | [ ] Pass |
| `uptime` | PIT Counter Check | Shows system uptime formatting as `hh:mm:ss`. Increases dynamically. | [ ] Pass |

---

## 4. Filesystem & Text Editor (MiniEdit) Testing

Because the filesystem operates entirely in volatile memory, testing verifies read/write logic:

### 4.1. File Creation & Directory Nesting Test
1. Create a directory: `mkd docs`
2. Enter directory: `goto docs`
3. Verify directory tree path: `wdir` (should display `/root/docs`)
4. Create a new file: `new notes.txt`
5. Print files list: `laf` (should show `notes.txt (0 bytes)`)

### 4.2. File Write & Read Verification
1. Run `write notes.txt`
2. Enter a test string when prompted: `Hello World MiniOS!`
3. Verify file storage sizes using `laf` (should report `19 bytes`).
4. Read the file contents using `see notes.txt` (must print `Hello World MiniOS!`).

### 4.3. MiniEdit Fullscreen Text Editor Integration
1. Run `edit notes.txt`
2. The screen should switch to the editor layout. Type some test paragraphs.
3. Press `ESC` to save and close the editor.
4. Run `see notes.txt` and verify that the edited content was correctly written to memory.

---

## 5. Memory Heap & Allocation Leak Verification

Memory safety can be verified using the heap manager debugger commands:

1. Check baseline heap allocation: `memo`. Write down the heap used bytes value (Baseline).
2. Create and write to multiple dummy files (e.g., 5 files).
3. Check `memo` stats: the used heap memory bytes should increase proportionally.
4. Delete all created dummy files.
5. Check `memo` stats again: the heap used bytes must return exactly to the Baseline value, confirming that `kfree()` coalesced the heap blocks correctly with zero memory leaks.
