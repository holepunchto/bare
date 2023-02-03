#include <stdlib.h>
#include <string.h>

#include "fs.h"

void
pear_fs_path_join (const char *a, const char *b, char *out) {
  size_t len_a = strlen(a);
  size_t len_b = strlen(b);

  if (out != a) memcpy(out, a, len_a);

  out += len_a;
  *out = PEAR_FS_SEP[0];

  memcpy(out + 1, b, len_b + 1);
}

int
pear_fs_realpath_sync (uv_loop_t *loop, const char *path, size_t *len, char **res) {
  uv_fs_t req;
  uv_fs_realpath(loop, &req, path, NULL);

  int err = req.result;

  if (err < 0) {
    uv_fs_req_cleanup(&req);
    return err;
  }

  size_t l = strlen(req.ptr);
  if (len != NULL) *len = l;
  *res = (char *) malloc(l + 1);
  strcpy(*res, req.ptr);
  uv_fs_req_cleanup(&req);

  return 0;
}

int
pear_fs_readdir_sync (uv_loop_t *loop, const char *dirname, int entries_len, uv_dirent_t *entries) {
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
