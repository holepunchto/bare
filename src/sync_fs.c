#include <string.h>

#include "sync_fs.h"

void
pearjs_sync_fs_path_join (const char *a, const char *b, char *out) {
  size_t len_a = strlen(a);
  size_t len_b = strlen(b);

  if (out != a) memcpy(out, a, len_a);

  out += len_a;
  *out = PEARJS_SYNC_FS_SEP[0];

  memcpy(out + 1, b, len_b);
}

int
pearjs_sync_fs_readdir (uv_loop_t *loop, const char *dirname, int entries_len, uv_dirent_t *entries) {
  uv_fs_t req;

  int num = 0;

  int err = uv_fs_opendir(loop, &req, dirname, NULL);
  if (err < 0) {
    uv_fs_req_cleanup(&req);
    return err;
  }

  uv_dir_t *dir = (uv_dir_t *) req.ptr;
  uv_fs_req_cleanup(&req);

  dir->dirents = entries;
  dir->nentries = entries_len;

  num = uv_fs_readdir(loop, &req, dir, NULL);
  if (num < 0) {
    uv_fs_req_cleanup(&req);
    return num;
  }

  err = uv_fs_closedir(loop, &req, dir, NULL);
  if (err < 0) {
    uv_fs_req_cleanup(&req);
    return err;
  }

  return num;
}

int
pearjs_sync_fs_stat (uv_loop_t *loop, const char *path, int *type, size_t *len) {
  uv_fs_t req;
  uv_fs_stat(loop, &req, path, NULL);

  if (req.result >= 0) {
    uv_stat_t *st = req.ptr;

    if (st->st_mode & S_IFREG) {
      if (type != NULL) *type = PEARJS_SYNC_FS_FILE;
    } else if (st->st_mode & S_IFDIR) {
      if (type != NULL) *type = PEARJS_SYNC_FS_DIR;
    }

    if (len != NULL) *len = st->st_size;
    return 0;
  }

  int err = req.result;
  uv_fs_req_cleanup(&req);
  return err;
}
