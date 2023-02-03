#ifndef PEAR_FS_H
#define PEAR_FS_H

#include <uv.h>

#define PEAR_FS_FILE     1
#define PEAR_FS_DIR      2
#define PEAR_FS_MAX_PATH 4096
#define PEAR_FS_SEP      "/"

void
pear_fs_path_join (const char *a, const char *b, char *out);

int
pear_fs_realpath_sync (uv_loop_t *loop, const char *path, size_t *len, char **res);

int
pear_fs_readdir_sync (uv_loop_t *loop, const char *path, int entries_len, uv_dirent_t *entries);

#endif // PEAR_FS_H
