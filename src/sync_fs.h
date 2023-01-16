#ifndef PEAR_SYNC_FS_H
#define PEAR_SYNC_FS_H

#include <uv.h>

#define PEAR_SYNC_FS_FILE 1
#define PEAR_SYNC_FS_DIR 2
#define PEAR_SYNC_FS_MAX_PATH 4096
#define PEAR_SYNC_FS_SEP "/"

void
pear_sync_fs_path_join (const char *a, const char *b, char *out);

int
pear_sync_fs_realpath (uv_loop_t *loop, const char *path, size_t *len, char **res);

int
pear_sync_fs_stat (uv_loop_t *loop, const char *path, int *type, size_t *len);

int
pear_sync_fs_readdir (uv_loop_t *loop, const char *path, int entries_len, uv_dirent_t *entries);

#endif
