/* ============================================================================
 * include/memory.h — Kernel Heap Memory Manager Interface
 *
 * Purpose : Simple heap allocator API. Provides kmalloc/kfree and basic
 *           memory utility functions (memset, memcpy, strlen, etc.).
 *           The heap lives in a region defined by the linker script.
 * ============================================================================ */

#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdint.h>

/* --------------------------------------------------------------------------
 * Heap allocator
 * -------------------------------------------------------------------------- */

/* Initialise the heap — call once during kernel startup */
void memory_init(void);

/* Allocate `size` bytes from the heap. Returns NULL if out of memory. */
void *kmalloc(size_t size);

/* Free a previously allocated block. */
void kfree(void *ptr);

/* Print heap usage statistics to the terminal */
void memory_print_stats(void);

/* --------------------------------------------------------------------------
 * PRNG Utilities
 * -------------------------------------------------------------------------- */
void srand(uint32_t seed);
uint32_t rand(void);

/* --------------------------------------------------------------------------
 * Memory utility functions (no libc in kernel!)
 * -------------------------------------------------------------------------- */
void  *memset(void *dest, int val, size_t n);
void  *memcpy(void *dest, const void *src, size_t n);
int    memcmp(const void *a, const void *b, size_t n);

size_t strlen(const char *s);
int    strcmp(const char *s1, const char *s2);
int    strncmp(const char *s1, const char *s2, size_t n);
int    atoi(const char *str);
char  *strcpy(char *dest, const char *src);
char  *strncpy(char *dest, const char *src, size_t n);
char  *strchr(const char *s, int c);

/* Simple integer-to-string helpers */
void  itoa(int value, char *buf, int base);
void  utoa(unsigned int value, char *buf, int base);

#endif /* MEMORY_H */
