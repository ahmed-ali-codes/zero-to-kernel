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
    int   is_dir;
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
    fs_create_dir("/root");
}

int fs_create(const char *name) {
    if (!name || strlen(name) == 0) {
        terminal_writestring("Error: file name cannot be empty.\n");
        return -1;
    }
    if (strlen(name) >= FS_MAX_FILENAME) {
        terminal_writestring("Error: file name too long (max 63 chars).\n");
        return -1;
    }
    if (fs_find(name) >= 0) {
        terminal_printf("Error: file '%s' already exists.\n", name);
        return -1;
    }
    int slot = fs_free_slot();
    if (slot < 0) {
        terminal_writestring("Error: filesystem full (max 32 files).\n");
        return -1;
    }

    strncpy(fs_table[slot].name, name, FS_MAX_FILENAME - 1);
    fs_table[slot].name[FS_MAX_FILENAME - 1] = '\0';
    fs_table[slot].size = 0;
    fs_table[slot].used = 1;
    fs_table[slot].is_dir = 0;
    return 0;
}

int fs_create_dir(const char *name) {
    if (!name || strlen(name) == 0) return -1;
    if (strlen(name) >= FS_MAX_FILENAME) return -1;
    if (fs_find(name) >= 0) return -1;
    int slot = fs_free_slot();
    if (slot < 0) return -1;

    strncpy(fs_table[slot].name, name, FS_MAX_FILENAME - 1);
    fs_table[slot].name[FS_MAX_FILENAME - 1] = '\0';
    fs_table[slot].size = 0;
    fs_table[slot].used = 1;
    fs_table[slot].is_dir = 1;
    return 0;
}

int fs_is_dir(const char *name) {
    if (!name) return 0;
    if (strcmp(name, "/") == 0) return 1;
    int idx = fs_find(name);
    if (idx >= 0 && fs_table[idx].is_dir) return 1;
    return 0;
}

int fs_write(const char *name, const char *data, size_t len) {
    int idx = fs_find(name);
    if (idx < 0) {
        terminal_printf("Error: file '%s' not found.\n", name);
        return -1;
    }
    if (fs_table[idx].is_dir) {
        terminal_printf("Error: '%s' is a directory.\n", name);
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
    if (idx < 0 || fs_table[idx].is_dir) {
        return -1;
    }
    size_t n = (size_t)fs_table[idx].size;
    if (n > buf_size - 1) n = buf_size - 1;
    memcpy(buf, fs_table[idx].data, n);
    buf[n] = '\0';
    return (int)n;
}

void fs_list_dir(const char *dir) {
    int count = 0;
    int dir_len = strlen(dir);
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (fs_table[i].used) {
            const char *fname = fs_table[i].name;
            int match = 0;
            const char *child_name = NULL;
            if (strcmp(dir, "/") == 0) {
                if (fname[0] == '/') {
                    child_name = fname + 1;
                    match = 1;
                }
            } else {
                if (strncmp(fname, dir, dir_len) == 0 && fname[dir_len] == '/') {
                    child_name = fname + dir_len + 1;
                    match = 1;
                }
            }
            if (match && child_name && strchr(child_name, '/') == NULL) {
                if (fs_table[i].is_dir) {
                    terminal_setcolor(COLOR_LIGHT_BLUE, COLOR_BLACK);
                    terminal_printf("  %-24s  <DIR>\n", child_name);
                    terminal_setcolor(COLOR_WHITE, COLOR_BLACK);
                } else {
                    terminal_printf("  %-24s  (%d bytes)\n", child_name, fs_table[i].size);
                }
                count++;
            }
        }
    }
    if (count == 0) {
        terminal_writestring("  (no files)\n");
    } else {
        terminal_printf("  %d item(s)\n", count);
    }
}

void fs_list_notes(const char *dir) {
    int count = 0;
    int dir_len = strlen(dir);
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (fs_table[i].used && !fs_table[i].is_dir) {
            const char *fname = fs_table[i].name;
            int match = 0;
            const char *child_name = NULL;
            if (strcmp(dir, "/") == 0) {
                if (fname[0] == '/') {
                    child_name = fname + 1;
                    match = 1;
                }
            } else {
                if (strncmp(fname, dir, dir_len) == 0 && fname[dir_len] == '/') {
                    child_name = fname + dir_len + 1;
                    match = 1;
                }
            }
            if (match && child_name && strchr(child_name, '/') == NULL) {
                if (strncmp(child_name, "note_", 5) == 0) {
                    terminal_printf("  %-24s  (%d bytes)\n", child_name + 5, fs_table[i].size);
                    count++;
                }
            }
        }
    }
    if (count == 0) {
        terminal_writestring("  (no notes)\n");
    } else {
        terminal_printf("  %d note(s)\n", count);
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

static void fs_print_tree_internal(const char *dir, int depth, int *is_last_arr) {
    int child_indices[FS_MAX_FILES];
    int child_count = 0;

    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (fs_table[i].used) {
            const char *fname = fs_table[i].name;
            int match = 0;
            const char *child_name = NULL;
            if (strcmp(dir, "/") == 0) {
                if (fname[0] == '/') {
                    child_name = fname + 1;
                    match = 1;
                }
            } else {
                int dir_len = strlen(dir);
                if (strncmp(fname, dir, dir_len) == 0 && fname[dir_len] == '/') {
                    child_name = fname + dir_len + 1;
                    match = 1;
                }
            }
            if (match && child_name && strlen(child_name) > 0 && strchr(child_name, '/') == NULL) {
                child_indices[child_count++] = i;
            }
        }
    }

    /* Sort children alphabetically */
    for (int i = 0; i < child_count - 1; i++) {
        for (int j = 0; j < child_count - i - 1; j++) {
            const char *name1 = fs_table[child_indices[j]].name;
            const char *name2 = fs_table[child_indices[j + 1]].name;
            if (strcmp(name1, name2) > 0) {
                int temp = child_indices[j];
                child_indices[j] = child_indices[j + 1];
                child_indices[j + 1] = temp;
            }
        }
    }

    /* Print each child */
    for (int k = 0; k < child_count; k++) {
        int idx = child_indices[k];
        
        /* Indentation for ancestors */
        for (int d = 0; d < depth; d++) {
            if (is_last_arr[d]) {
                terminal_printf("    ");
            } else {
                terminal_printf("%c   ", 179); /* │ */
            }
        }

        int is_last = (k == child_count - 1);
        if (is_last) {
            terminal_printf("%c%c%c ", 192, 196, 196); /* └── */
        } else {
            terminal_printf("%c%c%c ", 195, 196, 196); /* ├── */
        }

        const char *child_relative_name = NULL;
        if (strcmp(dir, "/") == 0) {
            child_relative_name = fs_table[idx].name + 1;
        } else {
            child_relative_name = fs_table[idx].name + strlen(dir) + 1;
        }

        if (fs_table[idx].is_dir) {
            terminal_setcolor(COLOR_LIGHT_BLUE, COLOR_BLACK);
            terminal_printf("%s/\n", child_relative_name);
            terminal_setcolor(COLOR_WHITE, COLOR_BLACK);
            
            is_last_arr[depth] = is_last;
            fs_print_tree_internal(fs_table[idx].name, depth + 1, is_last_arr);
        } else {
            terminal_printf("%s\n", child_relative_name);
        }
    }
}

void fs_print_tree(const char *dir) {
    terminal_setcolor(COLOR_LIGHT_BLUE, COLOR_BLACK);
    terminal_printf("%s\n", dir);
    terminal_setcolor(COLOR_WHITE, COLOR_BLACK);

    int is_last_arr[FS_MAX_FILES];
    memset(is_last_arr, 0, sizeof(is_last_arr));
    fs_print_tree_internal(dir, 0, is_last_arr);
}

