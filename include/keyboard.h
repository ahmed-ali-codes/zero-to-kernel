/* ============================================================================
 * include/keyboard.h — PS/2 Keyboard Driver Interface
 *
 * Purpose : API for the IRQ1-driven PS/2 keyboard driver. Provides a
 *           blocking getchar() and a readline() for the shell.
 * ============================================================================ */

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stddef.h>

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

/* Special control characters used by keyboard_getchar() for arrow keys */
#define KEY_UP_ARROW    0x11
#define KEY_DOWN_ARROW  0x12
#define KEY_RIGHT_ARROW 0x13
#define KEY_LEFT_ARROW  0x14

/* Install the keyboard IRQ handler (call after IDT + PIC are ready) */
void keyboard_install(void);

/* Internal IRQ1 handler — called by the interrupt dispatcher */
void keyboard_handler_internal(void);

/* Block until a printable character is available; returns it */
char keyboard_getchar(void);

/* Read a full line of input into `buf` (max `len-1` chars + null).
 * Echoes characters to screen. Returns number of chars read (excl. newline). */
int keyboard_readline(char *buf, size_t len);

#endif /* KEYBOARD_H */
