#ifndef PEARJS_SYNC_FS_H
#define PEARJS_SYNC_FS_H

#include <uv.h>

#define PEARJS_SYNC_FS_FILE 1
#define PEARJS_SYNC_FS_DIR 2
#define PEARJS_SYNC_FS_MAX_PATH 4096
#define PEARJS_SYNC_FS_SEP "/"

void
pearjs_sync_fs_path_join (const char *a, const char *b, char *out);

int
pearjs_sync_fs_stat (uv_loop_t *loop, const char *path, int *type, size_t *len);

int
pearjs_sync_fs_readdir (uv_loop_t *loop, const char *path, int entries_len, uv_dirent_t *entries);

#endif
