/* ============================================================================
 * kernel/filesystem.c — In-Memory Flat Filesystem
 *
 * Purpose : Implements a flat file table stored entirely in RAM. Supports
 *           up to FS_MAX_FILES files, each holding up to FS_MAX_FILESIZE
 *           bytes. All data is lost on reboot — this is intentional for v1.0.
 *
 * Design  : Simple array of file_entry structs. Each entry stores:
 *             - filename (null-terminated, max FS_MAX_FILENAME chars)
 *             - data     (raw bytes, max FS_MAX_FILESIZE)
 *             - size     (actual bytes used)
 *             - used     (1 = slot occupied, 0 = free)
 * ============================================================================ */

#include "../include/filesystem.h"
#include "../include/memory.h"
#include "../include/screen.h"

/* --------------------------------------------------------------------------
 * File entry structure
 * -------------------------------------------------------------------------- */
typedef struct {
    char  name[FS_MAX_FILENAME];
    char  data[FS_MAX_FILESIZE];
    int   size;
    int   used;
} file_entry_t;

/* --------------------------------------------------------------------------
 * Filesystem table
 * -------------------------------------------------------------------------- */
static file_entry_t fs_table[FS_MAX_FILES];

/* --------------------------------------------------------------------------
 * Internal helpers
 * -------------------------------------------------------------------------- */
static int fs_find(const char *name) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (fs_table[i].used && strcmp(fs_table[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

static int fs_free_slot(void) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (!fs_table[i].used) return i;
    }
    return -1;
}

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

void fs_init(void) {
    memset(fs_table, 0, sizeof(fs_table));
}

int fs_create(const char *name) {
    if (!name || strlen(name) == 0) {
        terminal_writestring("Error: file name cannot be empty.\n");
        return -1;
    }
    if (strlen(name) >= FS_MAX_FILENAME) {
        terminal_writestring("Error: file name too long (max 31 chars).\n");
        return -1;
    }
    if (fs_find(name) >= 0) {
        terminal_printf("Error: file '%s' already exists.\n", name);
        return -1;
    }
    int slot = fs_free_slot();
    if (slot < 0) {
        terminal_writestring("Error: filesystem full (max 16 files).\n");
        return -1;
    }

    strncpy(fs_table[slot].name, name, FS_MAX_FILENAME - 1);
    fs_table[slot].name[FS_MAX_FILENAME - 1] = '\0';
    fs_table[slot].size = 0;
    fs_table[slot].used = 1;
    return 0;
}

int fs_write(const char *name, const char *data, size_t len) {
    int idx = fs_find(name);
    if (idx < 0) {
        terminal_printf("Error: file '%s' not found.\n", name);
        return -1;
    }
    if (len > FS_MAX_FILESIZE) {
        len = FS_MAX_FILESIZE;   /* truncate silently */
    }
    memcpy(fs_table[idx].data, data, len);
    fs_table[idx].size = (int)len;
    return (int)len;
}

int fs_read(const char *name, char *buf, size_t buf_size) {
    int idx = fs_find(name);
    if (idx < 0) {
        return -1;
    }
    size_t n = (size_t)fs_table[idx].size;
    if (n > buf_size - 1) n = buf_size - 1;
    memcpy(buf, fs_table[idx].data, n);
    buf[n] = '\0';
    return (int)n;
}

void fs_list(void) {
    int count = 0;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (fs_table[i].used) {
            terminal_printf("  %-24s  (%d bytes)\n",
                            fs_table[i].name, fs_table[i].size);
            count++;
        }
    }
    if (count == 0) {
        terminal_writestring("  (no files)\n");
    } else {
        terminal_printf("  %d file(s)\n", count);
    }
}

int fs_delete(const char *name) {
    int idx = fs_find(name);
    if (idx < 0) return -1;
    memset(&fs_table[idx], 0, sizeof(file_entry_t));
    return 0;
}

int fs_get_total_files(void) {
    int count = 0;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (fs_table[i].used) {
            count++;
        }
    }
    return count;
}
