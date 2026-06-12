/* ============================================================================
 * include/filesystem.h — In-Memory Flat Filesystem Interface
 *
 * Purpose : Simple flat file table stored entirely in RAM. Supports up to
 *           16 files, each holding up to 256 bytes of data. All files are
 *           lost on reboot — this is intentional for v1.0.
 * ============================================================================ */

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stddef.h>

/* --------------------------------------------------------------------------
 * Filesystem constraints
 * -------------------------------------------------------------------------- */
#define FS_MAX_FILES      16
#define FS_MAX_FILENAME   32
#define FS_MAX_FILESIZE   256

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

/* Initialise the filesystem — call once during kernel startup */
void fs_init(void);

/* Create a new empty file. Returns 0 on success, -1 on error. */
int fs_create(const char *name);

/* Write data to a file, replacing its contents.
 * Returns number of bytes written, or -1 on error. */
int fs_write(const char *name, const char *data, size_t len);

/* Read a file's contents into `buf` (max `buf_size` bytes).
 * Returns number of bytes read, or -1 if not found. */
int fs_read(const char *name, char *buf, size_t buf_size);

/* List all files to the terminal */
void fs_list(void);

/* Get the total number of files currently stored */
int fs_get_total_files(void);

/* Delete a file. Returns 0 on success, -1 if not found. */
int fs_delete(const char *name);

#endif /* FILESYSTEM_H */
