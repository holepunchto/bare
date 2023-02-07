#ifndef PEAR_FS_H
#define PEAR_FS_H

#include <stdbool.h>
#include <stddef.h>
#include <uv.h>

#define PEAR_FS_MAX_PATH 4096
#define PEAR_FS_SEP      "/"

void
pear_fs_path_join (const char *a, const char *b, char *out);

int
pear_fs_realpath_sync (uv_loop_t *loop, const char *path, size_t *len, char **res);

bool
pear_fs_exists_sync (uv_loop_t *loop, const char *path);

int
pear_fs_read_sync (uv_loop_t *loop, const char *path, size_t *size, char **data);

int
pear_fs_readdir_sync (uv_loop_t *loop, const char *path, int entries_len, uv_dirent_t *entries);

#endif // PEAR_FS_H
