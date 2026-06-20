/* ============================================================================
 * include/editor.h — MiniEdit Text Editor Interface
 *
 * Purpose : Public API for the MiniEdit built-in text editor.
 *           The editor lives in kernel/editor.c and is completely decoupled
 *           from the shell. The shell just calls editor_open() and gets out
 *           of the way.
 *
 * Usage (from shell):
 *   editor_open("myfile.txt");
 *
 * Key bindings inside the editor:
 *   Any printable key  — insert character at end of buffer
 *   Backspace          — delete last character
 *   Enter              — insert a newline
 *   ESC                — save file and return to the shell
 * ============================================================================ */

#ifndef EDITOR_H
#define EDITOR_H

/*
 * editor_open — launch the MiniEdit text editor for the given file.
 *
 * If the file already exists its contents are loaded so the user can
 * continue editing. If the file does not exist it is created automatically.
 * The function blocks until the user presses ESC, at which point the buffer
 * is saved and control returns to the caller (the shell).
 *
 * Parameters:
 *   filename  — name of the file to open or create (must not be NULL)
 */
void editor_open(const char *filename);

#endif /* EDITOR_H */
