/* ============================================================================
 * include/screen.h — VGA Text Mode Screen Driver Interface
 *
 * Purpose : Public API for the MiniOS screen driver. All kernel code that
 *           needs to display text should use these functions — never write
 *           directly to 0xB8000 outside of screen.c.
 * ============================================================================ */

#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

/* --------------------------------------------------------------------------
 * VGA colour constants
 * -------------------------------------------------------------------------- */
typedef enum {
    COLOR_BLACK         = 0,
    COLOR_BLUE          = 1,
    COLOR_GREEN         = 2,
    COLOR_CYAN          = 3,
    COLOR_RED           = 4,
    COLOR_MAGENTA       = 5,
    COLOR_BROWN         = 6,
    COLOR_LIGHT_GREY    = 7,
    COLOR_DARK_GREY     = 8,
    COLOR_LIGHT_BLUE    = 9,
    COLOR_LIGHT_GREEN   = 10,
    COLOR_LIGHT_CYAN    = 11,
    COLOR_LIGHT_RED     = 12,
    COLOR_LIGHT_MAGENTA = 13,
    COLOR_YELLOW        = 14,
    COLOR_WHITE         = 15,
} vga_color_t;

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

/* Initialise the terminal — clears screen, sets default colours */
void terminal_init(void);

/* Clear the entire screen */
void terminal_clear(void);

/* Set the default foreground / background colour for subsequent writes */
void terminal_setcolor(vga_color_t fg, vga_color_t bg);

/* Write a single character at the current cursor position */
void terminal_putchar(char c);

/* Write a null-terminated string */
void terminal_writestring(const char *str);

/* Write exactly `len` bytes from `data` */
void terminal_write(const char *data, size_t len);

/* printf-style output (%s, %d, %u, %x, %c supported) */
void terminal_printf(const char *fmt, ...);

/* Set cursor position explicitly (bounds-checked) */
void terminal_setcursor(int row, int col);

/* Get current cursor row / col */
int terminal_row(void);
int terminal_col(void);

/* Write a string in a specific colour without changing the default */
void terminal_writecolor(const char *str, vga_color_t fg, vga_color_t bg);

#endif /* SCREEN_H */
