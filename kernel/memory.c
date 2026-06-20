/* ============================================================================
 * kernel/memory.c — Kernel Heap Allocator
 *
 * Purpose : Implements kmalloc/kfree on top of a heap region defined by
 *           the linker script symbol _heap_start. Uses a simple block-list
 *           allocator: each allocation is preceded by a small header that
 *           records its size and whether it's free.
 *
 * Design  : Bump-first: we advance a pointer through the heap. kfree marks
 *           blocks free. kmalloc first checks for a free block of sufficient
 *           size before bumping. This is simple enough to understand and debug.
 * ============================================================================ */

#include "../include/memory.h"
#include "../include/screen.h"

/* --------------------------------------------------------------------------
 * Heap layout
 * -------------------------------------------------------------------------- */
#define HEAP_SIZE    (1024 * 256)   /* 256 KiB heap */
#define ALIGN_BYTES  8              /* 8-byte alignment */

typedef struct block_header {
    size_t               size;      /* usable bytes (not counting header) */
    int                  free;      /* 1 = available, 0 = allocated        */
    struct block_header *next;      /* next block in the linked list        */
} block_header_t;

/* The heap lives in BSS — a static array. The linker symbol approach works
 * on real hardware; for maximum portability with QEMU we use a static array. */
static uint8_t heap_storage[HEAP_SIZE] __attribute__((aligned(ALIGN_BYTES)));
static block_header_t *heap_head = 0;
static size_t heap_used = 0;

/* --------------------------------------------------------------------------
 * Internal helpers
 * -------------------------------------------------------------------------- */

static size_t align_up(size_t n) {
    return (n + ALIGN_BYTES - 1) & ~(size_t)(ALIGN_BYTES - 1);
}

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

void memory_init(void) {
    heap_head = (block_header_t *)heap_storage;
    heap_head->size = HEAP_SIZE - sizeof(block_header_t);
    heap_head->free = 1;
    heap_head->next = 0;
    heap_used = 0;
}

void *kmalloc(size_t size) {
    if (size == 0) return 0;
    size = align_up(size);

    block_header_t *cur = heap_head;
    while (cur) {
        if (cur->free && cur->size >= size) {
            /* Split if there's enough leftover space for a new block */
            size_t leftover = cur->size - size;
            if (leftover >= sizeof(block_header_t) + ALIGN_BYTES) {
                block_header_t *new_block = (block_header_t *)((uint8_t *)(cur + 1) + size);
                new_block->size = leftover - sizeof(block_header_t);
                new_block->free = 1;
                new_block->next = cur->next;
                cur->size = size;
                cur->next = new_block;
            }
            cur->free = 0;
            heap_used += cur->size + sizeof(block_header_t);
            return (void *)(cur + 1);
        }
        cur = cur->next;
    }
    return 0;   /* Out of memory */
}

void kfree(void *ptr) {
    if (!ptr) return;
    block_header_t *hdr = (block_header_t *)ptr - 1;
    hdr->free = 1;
    heap_used -= (hdr->size + sizeof(block_header_t));

    /* Coalesce adjacent free blocks */
    block_header_t *cur = heap_head;
    while (cur && cur->next) {
        if (cur->free && cur->next->free) {
            cur->size += sizeof(block_header_t) + cur->next->size;
            cur->next  = cur->next->next;
        } else {
            cur = cur->next;
        }
    }
}

void memory_print_stats(void) {
    size_t total = HEAP_SIZE;
    size_t used  = heap_used;
    size_t free_  = total - used;
    terminal_printf("Heap total : %u bytes\n", (unsigned)total);
    terminal_printf("Heap used  : %u bytes\n", (unsigned)used);
    terminal_printf("Heap free  : %u bytes\n", (unsigned)free_);
}

/* --------------------------------------------------------------------------
 * Memory utilities (no libc available in the kernel)
 * -------------------------------------------------------------------------- */

void *memset(void *dest, int val, size_t n) {
    uint8_t *p = (uint8_t *)dest;
    while (n--) *p++ = (uint8_t)val;
    return dest;
}

void *memcpy(void *dest, const void *src, size_t n) {
    uint8_t       *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    while (n--) *d++ = *s++;
    return dest;
}

int memcmp(const void *a, const void *b, size_t n) {
    const uint8_t *p = (const uint8_t *)a;
    const uint8_t *q = (const uint8_t *)b;
    while (n--) {
        if (*p != *q) return (int)*p - (int)*q;
        p++; q++;
    }
    return 0;
}

size_t strlen(const char *s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

int strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char *a, const char *b, size_t n) {
    while (n && *a && (*a == *b)) { a++; b++; n--; }
    if (n == 0) return 0;
    return (unsigned char)*a - (unsigned char)*b;
}

char *strcpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++));
    return dest;
}

char *strncpy(char *dest, const char *src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i]; i++) dest[i] = src[i];
    for (; i < n; i++) dest[i] = '\0';
    return dest;
}

char *strchr(const char *s, int c) {
    while (*s) {
        if (*s == (char)c) return (char *)s;
        s++;
    }
    return 0;
}

char *strcat(char *dest, const char *src) {
    char *d = dest;
    while (*d) d++;
    while (*src) *d++ = *src++;
    *d = '\0';
    return dest;
}

char *strncat(char *dest, const char *src, size_t n) {
    char *d = dest;
    while (*d) d++;
    while (n-- && *src) *d++ = *src++;
    *d = '\0';
    return dest;
}

char *strrchr(const char *s, int c) {
    const char *last = 0;
    do {
        if (*s == (char)c) last = s;
    } while (*s++);
    return (char *)last;
}

/* Integer to ASCII string (supports base 10 and 16) */
void itoa(int value, char *buf, int base) {
    char tmp[32];
    int  i = 0;
    int  negative = 0;
    unsigned int uval;

    if (value < 0 && base == 10) {
        negative = 1;
        uval = (unsigned int)(-value);
    } else {
        uval = (unsigned int)value;
    }

    if (uval == 0) {
        buf[0] = '0'; buf[1] = '\0';
        return;
    }

    while (uval > 0) {
        int rem = uval % base;
        tmp[i++] = (rem < 10) ? ('0' + rem) : ('a' + rem - 10);
        uval /= base;
    }
    if (negative) tmp[i++] = '-';

    int j = 0;
    while (i-- > 0) buf[j++] = tmp[i];
    buf[j] = '\0';
}

void utoa(unsigned int value, char *buf, int base) {
    char tmp[32];
    int  i = 0;

    if (value == 0) {
        buf[0] = '0'; buf[1] = '\0';
        return;
    }

    while (value > 0) {
        int rem = value % base;
        tmp[i++] = (rem < 10) ? ('0' + rem) : ('a' + rem - 10);
        value /= base;
    }

    int j = 0;
    while (i-- > 0) buf[j++] = tmp[i];
    buf[j] = '\0';
}

int atoi(const char *str) {
    int res = 0;
    int sign = 1;
    int i = 0;

    if (str[0] == '-') {
        sign = -1;
        i++;
    } else if (str[0] == '+') {
        i++;
    }

    for (; str[i] != '\0'; ++i) {
        if (str[i] >= '0' && str[i] <= '9') {
            res = res * 10 + str[i] - '0';
        } else {
            break;
        }
    }
    return sign * res;
}

/* --------------------------------------------------------------------------
 * PRNG Utilities (LCG Algorithm)
 * -------------------------------------------------------------------------- */

static uint32_t rand_seed = 1;

void srand(uint32_t seed) {
    rand_seed = seed;
}

uint32_t rand(void) {
    /* POSIX.1-2001 standard LCG values */
    rand_seed = rand_seed * 1103515245 + 12345;
    return (uint32_t)((rand_seed / 65536) % 32768);
}
