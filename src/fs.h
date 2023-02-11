#ifndef PEAR_FS_H
#define PEAR_FS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <uv.h>

#define PEAR_FS_MAX_PATH 4096
#define PEAR_FS_SEP      "/"

static inline void
pear_fs_path_join (const char *a, const char *b, char *out) {
  size_t len_a = strlen(a);
  size_t len_b = strlen(b);

  if (out != a) memcpy(out, a, len_a);

  out += len_a;
  *out = PEAR_FS_SEP[0];

  memcpy(out + 1, b, len_b + 1);
}

static inline int
pear_fs_realpath_sync (pear_t *pear, const char *path, size_t *len, char **res) {
  uv_fs_t req;
  uv_fs_realpath(pear->loop, &req, path, NULL);

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

static inline bool
pear_fs_exists_sync (pear_t *pear, const char *path) {
  uv_fs_t req;
  uv_fs_access(pear->loop, &req, path, 0, NULL);

  int err = req.result;
  uv_fs_req_cleanup(&req);

  return err == 0;
}

static inline int
pear_fs_read_sync (pear_t *pear, const char *path, js_value_t **result) {
  uv_fs_t req;
  uv_fs_open(pear->loop, &req, path, UV_FS_O_RDONLY, 0, NULL);

  int fd = req.result;
  uv_fs_req_cleanup(&req);

  if (fd < 0) return fd;

  uv_fs_fstat(pear->loop, &req, fd, NULL);
  uv_stat_t *st = req.ptr;

  size_t len = st->st_size;
  char *base;

  js_create_arraybuffer(pear->env, len, (void **) &base, result);

  uv_buf_t buf = {
    .base = base,
    .len = len,
  };

  uv_fs_req_cleanup(&req);

  int64_t read = 0;

  while (true) {
    uv_fs_read(pear->loop, &req, fd, &buf, 1, read, NULL);

    int res = req.result;
    uv_fs_req_cleanup(&req);

    if (res < 0) {
      uv_fs_close(pear->loop, &req, fd, NULL);
      uv_fs_req_cleanup(&req);
      return res;
    }

    buf.base += res;
    buf.len -= res;

    read += res;
    if (res == 0 || read == len) break;
  }

  uv_fs_close(pear->loop, &req, fd, NULL);
  uv_fs_req_cleanup(&req);

  return 0;
}

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
