/* ============================================================================
 * kernel/keyboard.c — PS/2 Keyboard Driver (IRQ1)
 *
 * Purpose : Handles PS/2 keyboard input using interrupt-driven I/O on IRQ1.
 *           Reads raw scan codes from port 0x60, maps them to ASCII using a
 *           US QWERTY table, and stores them in a circular ring buffer.
 *           keyboard_getchar() blocks (spinning) until a char is available.
 *
 * Architecture:
 *   - keyboard_handler_internal() called by IRQ1 handler registered in IDT
 *   - Scan codes → ASCII via a lookup table
 *   - Circular 256-byte ring buffer (lock-free for single-CPU)
 * ============================================================================ */

#include "../include/keyboard.h"
#include "../include/screen.h"
#include "../include/idt.h"
#include "../include/pic.h"

/* --------------------------------------------------------------------------
 * Keyboard I/O port
 * -------------------------------------------------------------------------- */
#define KEYBOARD_DATA_PORT  0x60

/* --------------------------------------------------------------------------
 * US QWERTY scancode → ASCII map (scan set 1, make codes 0x00-0x58)
 * -------------------------------------------------------------------------- */
static const char scancode_map_lower[128] = {
    0,    27,  '1', '2', '3', '4', '5', '6', '7', '8',  /* 0x00-0x09 */
    '9', '0', '-', '=',   8,    9, 'q', 'w', 'e', 'r',  /* 0x0A-0x13 */
    't', 'y', 'u', 'i', 'o', 'p', '[', ']',  13,    0,  /* 0x14-0x1D */
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',  /* 0x1E-0x27 */
    '\'','`',   0, '\\','z', 'x', 'c', 'v', 'b', 'n',  /* 0x28-0x31 */
    'm', ',', '.', '/',   0,  '*',   0,  ' ',   0,   0,  /* 0x32-0x3B */
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,    /* 0x3C-0x45 */
    0,  '7', '8', '9', '-', '4', '5', '6', '+', '1',    /* 0x46-0x4F */
   '2', '3', '0', '.',   0,   0,   0,   0,   0,   0,    /* 0x50-0x59 */
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,    /* 0x5A-0x63 */
    0,   0,   0,   0,   0,   0,   0,   0               /* 0x64-0x6B */
};

static const char scancode_map_upper[128] = {
    0,    27,  '!', '@', '#', '$', '%', '^', '&', '*',
    '(', ')', '_', '+',   8,    9, 'Q', 'W', 'E', 'R',
    'T', 'Y', 'U', 'I', 'O', 'P', '{', '}',  13,    0,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',
    '"', '~',   0,  '|','Z', 'X', 'C', 'V', 'B', 'N',
    'M', '<', '>', '?',   0,  '*',   0,  ' ',   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,  '7', '8', '9', '-', '4', '5', '6', '+', '1',
   '2', '3', '0', '.',   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0
};

/* Shift key scan codes */
#define SCANCODE_LSHIFT_DOWN  0x2A
#define SCANCODE_RSHIFT_DOWN  0x36
#define SCANCODE_LSHIFT_UP    0xAA
#define SCANCODE_RSHIFT_UP    0xB6

/* Caps Lock */
#define SCANCODE_CAPSLOCK     0x3A

/* Num Lock */
#define SCANCODE_NUMLOCK      0x45

/* --------------------------------------------------------------------------
 * Ring buffer (256 bytes — power of 2 makes masking trivial)
 * -------------------------------------------------------------------------- */
#define KB_BUF_SIZE  256
#define KB_BUF_MASK  (KB_BUF_SIZE - 1)

static volatile char kb_buf[KB_BUF_SIZE];
static volatile int  kb_head = 0;   /* write index */
static volatile int  kb_tail = 0;   /* read  index */

/* Keyboard state */
static int shift_held   = 0;
static int capslock_on  = 0;
static int numlock_on   = 0;

/* --------------------------------------------------------------------------
 * IRQ1 handler — called by irq_handler in idt.c
 * -------------------------------------------------------------------------- */
static void keyboard_irq_handler(registers_t *regs) {
    (void)regs;

    uint8_t scancode = inb(KEYBOARD_DATA_PORT);

    /* Handle extended keys */
    static int e0_mode = 0;
    if (scancode == 0xE0) {
        e0_mode = 1;
        return;
    }
    int is_e0 = e0_mode;
    e0_mode = 0;

    /* Track modifier keys */
    if (scancode == SCANCODE_LSHIFT_DOWN || scancode == SCANCODE_RSHIFT_DOWN) {
        shift_held = 1;
        return;
    }
    if (scancode == SCANCODE_LSHIFT_UP || scancode == SCANCODE_RSHIFT_UP) {
        shift_held = 0;
        return;
    }
    if (scancode == SCANCODE_CAPSLOCK) {
        capslock_on = !capslock_on;
        return;
    }
    if (scancode == SCANCODE_NUMLOCK) {
        numlock_on = !numlock_on;
        return;
    }

    /* Ignore key-release events (bit 7 set) */
    if (scancode & 0x80) return;

    char c = 0;
    
    int is_numpad_nav = (!is_e0 && !numlock_on && scancode >= 0x47 && scancode <= 0x53 && scancode != 0x4A && scancode != 0x4E);

    /* Handle scrollback via extended keys */
    if (is_e0 || is_numpad_nav) {
        extern void terminal_scroll_up(int lines);
        extern void terminal_scroll_down(int lines);
        
        if (scancode == 0x48) { /* Up Arrow */
            if (shift_held) { terminal_scroll_up(1); return; }
            c = KEY_UP_ARROW;
        } else if (scancode == 0x50) { /* Down Arrow */
            if (shift_held) { terminal_scroll_down(1); return; }
            c = KEY_DOWN_ARROW;
        } else if (scancode == 0x4B) { /* Left Arrow */
            c = KEY_LEFT_ARROW;
        } else if (scancode == 0x4D) { /* Right Arrow */
            c = KEY_RIGHT_ARROW;
        } else if (scancode == 0x49) { /* Page Up */
            terminal_scroll_up(10);
            return;
        } else if (scancode == 0x51) { /* Page Down */
            terminal_scroll_down(10);
            return;
        } else {
            return; /* ignore other extended keys */
        }
    } else {
        /* Map scancode to ASCII */
        if (scancode < 128) {
            int use_upper = shift_held ^ capslock_on;
            c = use_upper ? scancode_map_upper[scancode] : scancode_map_lower[scancode];
        } else {
            return;
        }
    }

    if (c == 0) return;  /* unmapped */

    /* Store in ring buffer (drop if full) */
    int next_head = (kb_head + 1) & KB_BUF_MASK;
    if (next_head != kb_tail) {
        kb_buf[kb_head] = c;
        kb_head = next_head;
    }
}

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

void keyboard_install(void) {
    /* Flush the keyboard buffer in case there are unread keys from GRUB */
    while (inb(0x64) & 1) {
        inb(0x60);
    }

    idt_set_handler(33, keyboard_irq_handler);  /* IRQ1 = vector 33 */
    pic_unmask_irq(1);
}

void keyboard_handler_internal(void) {
    /* Provided for external calls if needed — IRQ path handles it */
}

char keyboard_getchar(void) {
    /* Spin-wait until a character is available */
    while (kb_head == kb_tail) {
        __asm__ volatile ("hlt");   /* halt until next interrupt */
    }
    char c = kb_buf[kb_tail];
    kb_tail = (kb_tail + 1) & KB_BUF_MASK;
    return c;
}

int keyboard_readline(char *buf, size_t len) {
    size_t i = 0;

    while (i < len - 1) {
        char c = keyboard_getchar();

        if (c == '\n' || c == '\r') {
            terminal_putchar('\n');
            break;
        }
        if (c == 8 /* backspace */) {
            if (i > 0) {
                i--;
                terminal_putchar(8);   /* screen.c handles backspace */
            }
            continue;
        }
        buf[i++] = c;
        terminal_putchar(c);
    }

    buf[i] = '\0';
    return (int)i;
}
