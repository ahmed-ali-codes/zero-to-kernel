/* ============================================================================
 * kernel/screen.c — VGA Text Mode Screen Driver
 *
 * Purpose : Provides all text output for MiniOS via direct memory-mapped
 *           I/O to the VGA text buffer at physical address 0xB8000.
 *
 * The VGA text buffer is a 2-byte-per-cell array: [char][attr]
 *   attr = (background << 4) | foreground  (4 bits each, VGA colour index)
 *
 * Supports: character output, string output, colour, auto-scroll,
 *           clear screen, and a minimal printf (supports %s %d %u %x %c).
 * ============================================================================ */

#include "../include/screen.h"
#include "../include/memory.h"  /* for itoa/utoa */
#include "../include/pic.h"     /* for outb */

/* --------------------------------------------------------------------------
 * VGA constants
 * -------------------------------------------------------------------------- */
#define VGA_ADDRESS   0xB8000
#define VGA_COLS      80
#define VGA_ROWS_MAX  25
#define VGA_ROWS      24  /* Use 24 rows to avoid QEMU macOS bottom crop bug */

/* --------------------------------------------------------------------------
 * Terminal state
 * -------------------------------------------------------------------------- */
static volatile uint16_t *vga_buf = (uint16_t *)VGA_ADDRESS;
static int  term_row   = 0;
static int  term_col   = 0;
static uint8_t term_color = 0;   /* packed (bg << 4) | fg */

/* --------------------------------------------------------------------------
 * Internal helpers
 * -------------------------------------------------------------------------- */

static inline uint8_t make_color(vga_color_t fg, vga_color_t bg) {
    return (uint8_t)((bg << 4) | (fg & 0x0F));
}

static inline uint16_t make_vga_entry(char c, uint8_t color) {
    return (uint16_t)((uint16_t)color << 8) | (uint8_t)c;
}

/* Update the hardware VGA cursor position */
static void update_cursor(void) {
    uint16_t pos = term_row * VGA_COLS + term_col;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

#define HISTORY_LINES 200
static uint16_t history_buf[HISTORY_LINES][VGA_COLS];
static int history_head = 0;
static int history_count = 0;
static int view_scroll = 0;
static uint16_t live_snapshot[VGA_ROWS_MAX][VGA_COLS];

static void save_history_line(volatile uint16_t *line) {
    for (int i=0; i<VGA_COLS; i++) history_buf[history_head][i] = line[i];
    history_head = (history_head + 1) % HISTORY_LINES;
    if (history_count < HISTORY_LINES) history_count++;
}

static void redraw_scroll(void) {
    if (view_scroll == 0) {
        for(int r=0; r<VGA_ROWS; r++) {
            for(int c=0; c<VGA_COLS; c++) vga_buf[r * VGA_COLS + c] = live_snapshot[r][c];
        }
        outb(0x3D4, 0x0A);
        outb(0x3D5, (inb(0x3D5) & 0xC0) | 14);
        outb(0x3D4, 0x0B);
        outb(0x3D5, (inb(0x3D5) & 0xE0) | 15);
        update_cursor();
    } else {
        for(int r=0; r<VGA_ROWS; r++) {
            if (r < view_scroll) {
                int hist_idx = (history_head - view_scroll + r + HISTORY_LINES) % HISTORY_LINES;
                for(int c=0; c<VGA_COLS; c++) vga_buf[r * VGA_COLS + c] = history_buf[hist_idx][c];
            } else {
                int live_r = r - view_scroll;
                for(int c=0; c<VGA_COLS; c++) vga_buf[r * VGA_COLS + c] = live_snapshot[live_r][c];
            }
        }
        outb(0x3D4, 0x0A);
        outb(0x3D5, 0x20); /* hide cursor */
    }
}

void terminal_scroll_up(int lines) {
    if (view_scroll == 0) {
        for(int r=0; r<VGA_ROWS; r++) {
            for(int c=0; c<VGA_COLS; c++) live_snapshot[r][c] = vga_buf[r * VGA_COLS + c];
        }
    }
    view_scroll += lines;
    if (view_scroll > history_count) view_scroll = history_count;
    if (view_scroll > 0) redraw_scroll();
}

void terminal_scroll_down(int lines) {
    if (view_scroll > 0) {
        view_scroll -= lines;
        if (view_scroll < 0) view_scroll = 0;
        redraw_scroll();
    }
}

/* Scroll the screen up by one line */
static void scroll(void) {
    save_history_line(&vga_buf[0]);
    /* Move every row up by one */
    for (int row = 1; row < VGA_ROWS; row++) {
        for (int col = 0; col < VGA_COLS; col++) {
            vga_buf[(row - 1) * VGA_COLS + col] = vga_buf[row * VGA_COLS + col];
        }
    }
    /* Clear the last row */
    uint16_t blank = make_vga_entry(' ', term_color);
    for (int col = 0; col < VGA_COLS; col++) {
        vga_buf[(VGA_ROWS - 1) * VGA_COLS + col] = blank;
    }
    term_row = VGA_ROWS - 1;
    update_cursor();
}

/* --------------------------------------------------------------------------
 * Public API Implementation
 * -------------------------------------------------------------------------- */

void terminal_init(void) {
    term_color = make_color(COLOR_LIGHT_GREY, COLOR_BLACK);
    term_row   = 0;
    term_col   = 0;
    terminal_clear();
}

void terminal_clear(void) {
    if (view_scroll > 0) {
        view_scroll = 0;
        redraw_scroll();
    }
    uint16_t blank = make_vga_entry(' ', term_color);
    for (int i = 0; i < VGA_ROWS_MAX * VGA_COLS; i++) {
        vga_buf[i] = blank;
    }
    term_row = 0;
    term_col = 0;
    update_cursor();
}

void terminal_setcolor(vga_color_t fg, vga_color_t bg) {
    term_color = make_color(fg, bg);
}

void terminal_putchar(char c) {
    if (view_scroll > 0) {
        view_scroll = 0;
        redraw_scroll();
    }

    if (c == '\n') {
        term_col = 0;
        term_row++;
        if (term_row >= VGA_ROWS) scroll();
        update_cursor();
        outb(0x3F8, '\r');
        outb(0x3F8, '\n');
        return;
    }
    if (c == '\r') {
        term_col = 0;
        update_cursor();
        outb(0x3F8, '\r');
        return;
    }
    if (c == '\b') {
        /* Backspace */
        if (term_col > 0) {
            term_col--;
        } else if (term_row > 0) {
            term_row--;
            term_col = VGA_COLS - 1;
        }
        vga_buf[term_row * VGA_COLS + term_col] = make_vga_entry(' ', term_color);
        update_cursor();
        outb(0x3F8, '\b');
        outb(0x3F8, ' ');
        outb(0x3F8, '\b');
        return;
    }
    if (c == '\t') {
        /* Advance to next 8-column tab stop */
        int next_tab = (term_col + 8) & ~7;
        while (term_col < next_tab) terminal_putchar(' ');
        update_cursor();
        return;
    }

    /* Normal printable character */
    vga_buf[term_row * VGA_COLS + term_col] = make_vga_entry(c, term_color);
    term_col++;
    
    outb(0x3F8, c);

    if (term_col >= VGA_COLS) {
        term_col = 0;
        term_row++;
        if (term_row >= VGA_ROWS) scroll();
    }
    update_cursor();
}

void terminal_write(const char *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        terminal_putchar(data[i]);
    }
}

void terminal_writestring(const char *str) {
    while (*str) {
        terminal_putchar(*str++);
    }
}

void terminal_writecolor(const char *str, vga_color_t fg, vga_color_t bg) {
    uint8_t saved = term_color;
    term_color = make_color(fg, bg);
    terminal_writestring(str);
    term_color = saved;
}

void terminal_setcursor(int row, int col) {
    if (row >= 0 && row < VGA_ROWS) term_row = row;
    if (col >= 0 && col < VGA_COLS) term_col = col;
    update_cursor();
}

int terminal_row(void) { return term_row; }
int terminal_col(void) { return term_col; }

/* --------------------------------------------------------------------------
 * Minimal printf: supports %s %d %u %x %c %%
 * -------------------------------------------------------------------------- */
void terminal_printf(const char *fmt, ...) {
    __builtin_va_list args;
    __builtin_va_start(args, fmt);

    char numbuf[32];

    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            int zero_pad = 0;
            int left_justify = 0;
            int width = 0;

            if (*fmt == '-') { left_justify = 1; fmt++; }
            if (*fmt == '0') { zero_pad = 1; fmt++; }
            while (*fmt >= '0' && *fmt <= '9') {
                width = width * 10 + (*fmt - '0');
                fmt++;
            }

            switch (*fmt) {
                case 's': {
                    const char *s = __builtin_va_arg(args, const char *);
                    if (!s) s = "(null)";
                    int len = strlen(s);
                    if (!left_justify) {
                        for (int i = len; i < width; i++) terminal_putchar(' ');
                    }
                    terminal_writestring(s);
                    if (left_justify) {
                        for (int i = len; i < width; i++) terminal_putchar(' ');
                    }
                    break;
                }
                case 'd': {
                    int val = __builtin_va_arg(args, int);
                    itoa(val, numbuf, 10);
                    int len = strlen(numbuf);
                    if (val >= 0 && zero_pad) {
                        for (int i = len; i < width; i++) terminal_putchar('0');
                    } else if (!left_justify) {
                        for (int i = len; i < width; i++) terminal_putchar(' ');
                    }
                    terminal_writestring(numbuf);
                    if (left_justify) {
                        for (int i = len; i < width; i++) terminal_putchar(' ');
                    }
                    break;
                }
                case 'u': {
                    unsigned int val = __builtin_va_arg(args, unsigned int);
                    utoa(val, numbuf, 10);
                    int len = strlen(numbuf);
                    if (zero_pad) {
                        for (int i = len; i < width; i++) terminal_putchar('0');
                    } else if (!left_justify) {
                        for (int i = len; i < width; i++) terminal_putchar(' ');
                    }
                    terminal_writestring(numbuf);
                    if (left_justify) {
                        for (int i = len; i < width; i++) terminal_putchar(' ');
                    }
                    break;
                }
                case 'x': {
                    unsigned int val = __builtin_va_arg(args, unsigned int);
                    utoa(val, numbuf, 16);
                    terminal_writestring(numbuf);
                    break;
                }
                case 'c': {
                    char c = (char)__builtin_va_arg(args, int);
                    terminal_putchar(c);
                    break;
                }
                case '%':
                    terminal_putchar('%');
                    break;
                default:
                    terminal_putchar('%');
                    terminal_putchar(*fmt);
                    break;
            }
        } else {
            terminal_putchar(*fmt);
        }
        if (*fmt) fmt++;
    }

    __builtin_va_end(args);
}
