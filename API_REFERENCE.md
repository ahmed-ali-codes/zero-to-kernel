# MiniOS Kernel API Reference

This document provides function signatures, descriptions, parameters, return values, and usage details for the public subsystems of MiniOS.

---

## 1. VGA Screen Driver (`screen.h`)

Declared in [screen.h](file:///Users/apple/Documents/Ecotrustia-data/Own-projects-planning/mini-os/MiniOS/include/screen.h). All functions print to the VGA memory buffer at `0xB8000`.

### `terminal_init`
```c
void terminal_init(void);
```
- **Description**: Initializes the VGA text terminal. Clears memory buffers, sets default colors to white text on a black background, and sets the hardware cursor to `(0, 0)`.

### `terminal_clear`
```c
void terminal_clear(void);
```
- **Description**: Clears the display screen by filling the 80x25 characters with blank characters (' ') with current color attributes.

### `terminal_putchar`
```c
void terminal_putchar(char c);
```
- **Description**: Prints a single character `c` to the screen at the current cursor position. Handles special control characters:
  - `\n`: Carriage return and moves cursor to the next line.
  - `\t`: Inserts four spaces.
  - `\b` (Backspace): Moves the cursor back one slot.
  - Auto-scrolls the screen if printing reaches the bottom row.

### `terminal_writestring`
```c
void terminal_writestring(const char *str);
```
- **Description**: Writes a null-terminated ASCII string to the console.
- **Parameters**: 
  - `str`: Pointer to the string character buffer.

### `terminal_printf`
```c
void terminal_printf(const char *fmt, ...);
```
- **Description**: Prints a formatted string to the console. Supported formats:
  - `%s`: Character string.
  - `%d` / `%u`: Signed and unsigned integers.
  - `%x`: Hexadecimal format.
  - `%c`: Individual character.

---

## 2. Heap Memory Manager (`memory.h`)

Declared in [memory.h](file:///Users/apple/Documents/Ecotrustia-data/Own-projects-planning/mini-os/MiniOS/include/memory.h). Manages dynamic memory allocation.

### `memory_init`
```c
void memory_init(void);
```
- **Description**: Initializes the 256 KiB heap memory region. Configures a single large free block spanning the whole arena.

### `kmalloc`
```c
void *kmalloc(size_t size);
```
- **Description**: Allocates a block of `size` bytes from the kernel heap.
- **Parameters**: 
  - `size`: Bytes to allocate.
- **Return**: Pointer to the allocated block, or `NULL` if heap memory is exhausted.

### `kfree`
```c
void kfree(void *ptr);
```
- **Description**: Deallocates memory previously reserved by `kmalloc`.
- **Parameters**:
  - `ptr`: Pointer returned by a previous `kmalloc` call.

### `memory_print_stats`
```c
void memory_print_stats(void);
```
- **Description**: Prints diagnostic heap information showing:
  - Total heap size.
  - Allocated/active bytes.
  - Available free space.

---

## 3. Keyboard Input Subsystem (`keyboard.h`)

Declared in [keyboard.h](file:///Users/apple/Documents/Ecotrustia-data/Own-projects-planning/mini-os/MiniOS/include/keyboard.h).

### `keyboard_getchar`
```c
char keyboard_getchar(void);
```
- **Description**: Blocks the current execution thread until a key-down event is registered, then returns the corresponding ASCII character.
- **Return**: The ASCII code of the key pressed. Handles Arrow keys via special control constants:
  - `KEY_UP_ARROW` (`0x11`)
  - `KEY_DOWN_ARROW` (`0x12`)
  - `KEY_RIGHT_ARROW` (`0x13`)
  - `KEY_LEFT_ARROW` (`0x14`)

### `keyboard_readline`
```c
int keyboard_readline(char *buf, size_t len);
```
- **Description**: Reads a full line of keyboard input until the user presses Enter (`\n`). Echoes printable characters to the screen. Supports Backspace.
- **Parameters**:
  - `buf`: Target character array buffer.
  - `len`: Capacity of the target buffer.
- **Return**: Number of characters read (excluding null terminator).

---

## 4. In-Memory Filesystem (`filesystem.h`)

Declared in [filesystem.h](file:///Users/apple/Documents/Ecotrustia-data/Own-projects-planning/mini-os/MiniOS/include/filesystem.h).

### `fs_init`
```c
void fs_init(void);
```
- **Description**: Initializes the file table in memory. Clears all file entries and marks the system as empty. Creates the default `/root` directory.

### `fs_create`
```c
int fs_create(const char *name);
```
- **Description**: Registers a new empty file in the file table.
- **Parameters**:
  - `name`: Absolute path of the file (e.g., `/root/notes.txt`).
- **Return**: `0` on success, `-1` on failure (e.g., duplicate filename or file table full).

### `fs_create_dir`
```c
int fs_create_dir(const char *name);
```
- **Description**: Creates a new virtual directory block path.
- **Return**: `0` on success, `-1` on error.

### `fs_write`
```c
int fs_write(const char *name, const char *data, size_t len);
```
- **Description**: Writes contents to a file, replacing its existing contents.
- **Parameters**:
  - `name`: Path of the target file.
  - `data`: Pointer to the source data buffer.
  - `len`: Length of data to write (maximum 256 bytes).
- **Return**: Number of bytes written, or `-1` if the file does not exist.

### `fs_read`
```c
int fs_read(const char *name, char *buf, size_t buf_size);
```
- **Description**: Reads contents of a file into the target buffer.
- **Parameters**:
  - `name`: Path of the target file.
  - `buf`: Destination memory buffer.
  - `buf_size`: Max bytes to write to `buf`.
- **Return**: Number of bytes read, or `-1` if the file does not exist.

### `fs_delete`
```c
int fs_delete(const char *name);
```
- **Description**: Permanently deletes a file or directory from the file table.
- **Return**: `0` on success, `-1` if the entry is not found.

---

## 5. MiniEdit Text Editor (`editor.h`)

Declared in [editor.h](file:///Users/apple/Documents/Ecotrustia-data/Own-projects-planning/mini-os/MiniOS/include/editor.h).

### `editor_open`
```c
void editor_open(const char *filename);
```
- **Description**: Launches the fullscreen console text editor (MiniEdit) for the specified file path. Loads content if the file exists, or sets up a new blank buffer. Blocks execution until `ESC` is pressed, which saves changes and returns control to the shell.
- **Parameters**:
  - `filename`: Absolute path to open or create.
