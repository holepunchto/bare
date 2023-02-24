#ifndef PEAR_FS_H
#define PEAR_FS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <uv.h>

#define PEAR_FS_MAX_PATH 4096

static inline int
pear_fs_readdir_sync (pear_t *pear, const char *dirname, int entries_len, uv_dirent_t *entries) {
  uv_fs_t req;

  int num = 0;

  int err = uv_fs_opendir(pear->loop, &req, dirname, NULL);
  if (err < 0) {
    uv_fs_req_cleanup(&req);
    return err;
  }

  uv_dir_t *dir = (uv_dir_t *) req.ptr;
  uv_fs_req_cleanup(&req);

  dir->dirents = entries;
  dir->nentries = entries_len;

  num = uv_fs_readdir(pear->loop, &req, dir, NULL);
  if (num < 0) {
    uv_fs_req_cleanup(&req);
    return num;
  }

  err = uv_fs_closedir(pear->loop, &req, dir, NULL);
  if (err < 0) {
    uv_fs_req_cleanup(&req);
    return err;
  }

  return num;
}

#endif // PEAR_FS_H
