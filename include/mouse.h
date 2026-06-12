/* ============================================================================
 * include/mouse.h — PS/2 Mouse Driver with IntelliMouse Extensions
 *
 * Purpose : Initializes the PS/2 mouse on IRQ12, enables the scroll wheel
 *           Z-axis via the magic sequence, and triggers scrollback when the
 *           wheel is moved.
 * ============================================================================ */

#ifndef MOUSE_H
#define MOUSE_H

void mouse_install(void);

#endif /* MOUSE_H */
