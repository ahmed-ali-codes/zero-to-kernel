/* ============================================================================
 * include/shell.h — MiniOS Interactive Shell Interface
 *
 * Purpose : Public API for the command-line shell. The shell reads a line,
 *           parses it, and dispatches to a built-in command handler.
 * ============================================================================ */

#ifndef SHELL_H
#define SHELL_H

/* Start the shell main loop — never returns */
void shell_run(void);

#endif /* SHELL_H */
