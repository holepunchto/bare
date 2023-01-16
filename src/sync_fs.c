#include <string.h>
#include <stdlib.h>

#include "sync_fs.h"

void
pear_sync_fs_path_join (const char *a, const char *b, char *out) {
  size_t len_a = strlen(a);
  size_t len_b = strlen(b);

  if (out != a) memcpy(out, a, len_a);

  out += len_a;
  *out = PEAR_SYNC_FS_SEP[0];

  memcpy(out + 1, b, len_b + 1);
}

int
pear_sync_fs_read_file (uv_loop_t *loop, const char *path, size_t *size, char **data) {
  uv_fs_t req;
  uv_fs_open(loop, &req, path, UV_FS_O_RDONLY, 0, NULL);

  int fd = req.result;
  uv_fs_req_cleanup(&req);

  if (fd < 0) return fd;

  uv_fs_fstat(loop, &req, fd, NULL);
  uv_stat_t *st = req.ptr;

  size_t len = st->st_size;
  char *base = malloc(len);

  uv_buf_t buf = {
    .base = base,
    .len = len,
  };

  uv_fs_req_cleanup(&req);

  int64_t read = 0;

  while (1) {
    uv_fs_read(loop, &req, fd, &buf, 1, read, NULL);

    int res = req.result;
    uv_fs_req_cleanup(&req);

    if (res < 0) {
      free(base);
      uv_fs_close(loop, &req, fd, NULL);
      uv_fs_req_cleanup(&req);
      return res;
    }

    buf.base += res;
    buf.len -= res;

    read += res;
    if (res == 0 || read == len) break;
  }

  uv_fs_close(loop, &req, fd, NULL);
  uv_fs_req_cleanup(&req);

  *data = base;
  *size = read;

  return 0;
}

int
pear_sync_fs_realpath (uv_loop_t *loop, const char *path, size_t *len, char **res) {
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
pear_sync_fs_readdir (uv_loop_t *loop, const char *dirname, int entries_len, uv_dirent_t *entries) {
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
pear_sync_fs_stat (uv_loop_t *loop, const char *path, int *type, size_t *len) {
  uv_fs_t req;
  uv_fs_stat(loop, &req, path, NULL);

  if (req.result >= 0) {
    uv_stat_t *st = req.ptr;

    if (st->st_mode & S_IFREG) {
      if (type != NULL) *type = PEAR_SYNC_FS_FILE;
    } else if (st->st_mode & S_IFDIR) {
      if (type != NULL) *type = PEAR_SYNC_FS_DIR;
    }

    if (len != NULL) *len = st->st_size;
    return 0;
  }

  int err = req.result;
  uv_fs_req_cleanup(&req);
  return err;
}
