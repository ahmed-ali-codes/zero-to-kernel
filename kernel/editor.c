/* ============================================================================
 * kernel/editor.c — MiniEdit Text Editor
 *
 * Purpose  : A lightweight, interactive, full-screen text editor for MiniOS.
 *            Launched by the shell command 'edit [filename]'.
 *
 * Design overview:
 *   The editor keeps the entire file content in a single in-memory buffer
 *   (edit_buffer[]) while the user types.  On ESC the buffer is flushed to
 *   the filesystem in one write.  The maximum file size is bounded by
 *   FS_MAX_FILESIZE, which the editor enforces gracefully.
 *
 *   The implementation is split into small, single-purpose functions so that
 *   each piece of behaviour is easy to read, test, and change independently:
 *
 *     load_file_or_create()      — fill the buffer from disk (or start fresh)
 *     draw_editor_header()       — render the top status/hint bar
 *     draw_editor_divider()      — render the separator line
 *     display_file_content()     — print existing text into the edit area
 *     handle_backspace_key()     — erase the last character
 *     handle_enter_key()         — append a newline
 *     handle_printable_char()    — append a printable character
 *     save_and_show_result()     — write buffer to disk, print outcome
 *     run_editor_input_loop()    — block on keyboard until ESC is pressed
 *     editor_open()              — public entry point (called by shell)
 *
 * Key bindings:
 *   Any printable character  — insert at end of buffer
 *   Backspace  (ASCII  8)    — delete the last character
 *   Enter      (ASCII 13/10) — insert a newline
 *   ESC        (ASCII 27)    — save file and exit editor
 * ============================================================================ */

#include "../include/editor.h"
#include "../include/filesystem.h"
#include "../include/keyboard.h"
#include "../include/screen.h"

/* --------------------------------------------------------------------------
 * Internal constants
 * -------------------------------------------------------------------------- */

/* ASCII control codes used by the editor */
#define KEY_ESC            27
#define KEY_BACKSPACE       8
#define KEY_CARRIAGE_RETURN '\r'
#define KEY_NEWLINE         '\n'

/* Width of the divider line (matches the 80-column VGA text mode width) */
#define EDITOR_DIVIDER_WIDTH 80

/* --------------------------------------------------------------------------
 * Internal state
 *
 * edit_buffer holds the file contents while the editor is running.
 * edit_buffer_len tracks how many bytes are currently in the buffer.
 * Both are reset at the start of every editor_open() call.
 * -------------------------------------------------------------------------- */
static char edit_buffer[FS_MAX_FILESIZE + 1]; /* +1 so we can always null-terminate */
static int  edit_buffer_len = 0;

/* --------------------------------------------------------------------------
 * draw_editor_header
 *
 * Renders the top status bar of the editor.  The bar is drawn in inverted
 * colours (black text on cyan background) to visually separate it from the
 * editable content area, similar to how nano and micro look.
 *
 * Shows: editor name, the open filename, and keyboard shortcut hints.
 * -------------------------------------------------------------------------- */
static void draw_editor_header(const char *filename)
{
    terminal_setcolor(COLOR_BLACK, COLOR_LIGHT_CYAN);
    terminal_writestring(" MiniEdit  |  ");
    terminal_writestring(filename);
    terminal_writestring("  |  [ESC] Save & Quit   [BKSP] Delete   [ENTER] New line ");
    terminal_setcolor(COLOR_WHITE, COLOR_BLACK);
    terminal_putchar('\n');
}

/* --------------------------------------------------------------------------
 * draw_editor_divider
 *
 * Renders a full-width horizontal line in dark grey.  Used twice: once
 * below the header to frame the content area, and once above the save
 * confirmation message when the editor closes.
 * -------------------------------------------------------------------------- */
static void draw_editor_divider(void)
{
    terminal_setcolor(COLOR_DARK_GREY, COLOR_BLACK);
    for (int i = 0; i < EDITOR_DIVIDER_WIDTH; i++) {
        terminal_putchar('-');
    }
    terminal_setcolor(COLOR_WHITE, COLOR_BLACK);
    terminal_putchar('\n');
}

/* --------------------------------------------------------------------------
 * display_file_content
 *
 * Writes whatever is currently in edit_buffer to the terminal so the user
 * can see the existing file content before they start typing.  Does nothing
 * when the buffer is empty (new file).
 * -------------------------------------------------------------------------- */
static void display_file_content(void)
{
    if (edit_buffer_len > 0) {
        terminal_write(edit_buffer, (size_t)edit_buffer_len);
    }
}

/* --------------------------------------------------------------------------
 * load_file_or_create
 *
 * Tries to read the named file into edit_buffer[].
 *   - If the file exists  : its bytes are loaded and edit_buffer_len is set.
 *   - If the file is new  : fs_create() is called to register it, and the
 *                           buffer is left empty so the user starts fresh.
 *
 * Returns  0 on success.
 * Returns -1 if the file had to be created but creation failed (filesystem
 *            full, name too long, etc.).  The error message is printed here
 *            so the caller does not need to know the reason.
 * -------------------------------------------------------------------------- */
static int load_file_or_create(const char *filename)
{
    int bytes_read = fs_read(filename, edit_buffer, FS_MAX_FILESIZE + 1);

    if (bytes_read >= 0) {
        /* Existing file — load it */
        edit_buffer[bytes_read] = '\0';
        edit_buffer_len         = bytes_read;
        return 0;
    }

    /* File does not exist — create it as an empty file */
    if (fs_create(filename) != 0) {
        terminal_printf("Error: cannot open or create '%s'\n", filename);
        return -1;
    }

    edit_buffer[0]  = '\0';
    edit_buffer_len = 0;
    return 0;
}

/* --------------------------------------------------------------------------
 * handle_backspace_key
 *
 * Removes the last character from edit_buffer and erases it from the screen
 * by sending a backspace character to the terminal driver.
 * Does nothing when the buffer is already empty.
 * -------------------------------------------------------------------------- */
static void handle_backspace_key(void)
{
    if (edit_buffer_len > 0) {
        edit_buffer_len--;
        edit_buffer[edit_buffer_len] = '\0';
        terminal_putchar(KEY_BACKSPACE);
    }
}

/* --------------------------------------------------------------------------
 * handle_enter_key
 *
 * Appends '\n' to the buffer and advances the terminal cursor to the next
 * line.  Silently ignored when the buffer is full.
 * -------------------------------------------------------------------------- */
static void handle_enter_key(void)
{
    if (edit_buffer_len < FS_MAX_FILESIZE) {
        edit_buffer[edit_buffer_len++] = '\n';
        edit_buffer[edit_buffer_len]   = '\0';
        terminal_putchar('\n');
    }
}

/* --------------------------------------------------------------------------
 * handle_printable_char
 *
 * Appends a regular printable character to the buffer and echoes it to the
 * screen.  When the buffer is completely full the character is rejected and
 * a brief red '!' flash is shown — the '!' is immediately erased so it
 * does not stay on screen, acting as a visual bell.
 * -------------------------------------------------------------------------- */
static void handle_printable_char(char c)
{
    if (edit_buffer_len < FS_MAX_FILESIZE) {
        edit_buffer[edit_buffer_len++] = c;
        edit_buffer[edit_buffer_len]   = '\0';
        terminal_putchar(c);
    } else {
        /* Buffer is full — flash a warning character as a visual bell */
        terminal_setcolor(COLOR_LIGHT_RED, COLOR_BLACK);
        terminal_putchar('!');
        terminal_putchar(KEY_BACKSPACE); /* erase immediately */
        terminal_setcolor(COLOR_WHITE, COLOR_BLACK);
    }
}

/* --------------------------------------------------------------------------
 * save_and_show_result
 *
 * Writes the buffer back to the filesystem and prints the outcome.
 * A green message confirms success and shows the byte count.
 * A red message is shown if the write fails (should be rare).
 * -------------------------------------------------------------------------- */
static void save_and_show_result(const char *filename)
{
    int bytes_written = fs_write(filename, edit_buffer, (size_t)edit_buffer_len);

    terminal_putchar('\n');
    draw_editor_divider();

    if (bytes_written >= 0) {
        terminal_setcolor(COLOR_LIGHT_GREEN, COLOR_BLACK);
        terminal_printf("  Saved: %s  (%d bytes)\n", filename, bytes_written);
    } else {
        terminal_setcolor(COLOR_LIGHT_RED, COLOR_BLACK);
        terminal_printf("  Error: could not save '%s'\n", filename);
    }

    terminal_setcolor(COLOR_WHITE, COLOR_BLACK);
}

/* --------------------------------------------------------------------------
 * run_editor_input_loop
 *
 * Blocks and reads keyboard input one character at a time, dispatching each
 * key to the right handler.  Runs until the user presses ESC, which
 * triggers a save and breaks the loop.
 * -------------------------------------------------------------------------- */
static void run_editor_input_loop(const char *filename)
{
    while (1) {
        char key = keyboard_getchar();

        if (key == KEY_ESC) {
            save_and_show_result(filename);
            break;
        }

        if (key == KEY_BACKSPACE) {
            handle_backspace_key();
            continue;
        }

        if (key == KEY_CARRIAGE_RETURN || key == KEY_NEWLINE) {
            handle_enter_key();
            continue;
        }

        if (key >= 32 && key < 127) {
            handle_printable_char(key);
        }
    }
}

/* --------------------------------------------------------------------------
 * editor_open  (public — declared in include/editor.h)
 *
 * The single entry point for the MiniEdit editor.  Called by the shell
 * when the user types 'edit [filename]'.
 *
 * Steps:
 *   1. Validate the filename.
 *   2. Load the file into the buffer (or create it if it does not exist).
 *   3. Clear the screen and draw the editor UI.
 *   4. Display any pre-existing file content.
 *   5. Hand off to the input loop and wait for ESC.
 * -------------------------------------------------------------------------- */
void editor_open(const char *filename)
{
    if (!filename || *filename == '\0') {
        terminal_writestring("Usage: edit [filename]\n");
        return;
    }

    /* Step 1: Prepare the file buffer */
    if (load_file_or_create(filename) != 0) {
        return; /* error already printed inside load_file_or_create */
    }

    /* Step 2: Draw the editor interface */
    terminal_clear();
    draw_editor_header(filename);
    draw_editor_divider();

    /* Step 3: Show pre-existing content so the user can keep editing */
    display_file_content();

    /* Step 4: Run until the user presses ESC */
    run_editor_input_loop(filename);
}
