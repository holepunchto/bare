#include <pearjs.h>
#include <js.h>
#include <uv.h>
#include <stdlib.h>
#include <stdio.h>

#include "addons.h"

static js_value_t *
process_stdout (js_env_t *env, js_callback_info_t *info) {
  js_value_t *argv[1];
  size_t argc = 1;

  js_get_callback_info(env, info, &argc, argv, NULL, NULL);

  char data[65536];
  size_t data_len;

  js_get_value_string_utf8(env, argv[0], data, 65536, &data_len);

  fprintf(stdout, "%s", data);

  return NULL;
}

static js_value_t *
process_stderr (js_env_t *env, js_callback_info_t *info) {
  js_value_t *argv[1];
  size_t argc = 1;

  js_get_callback_info(env, info, &argc, argv, NULL, NULL);

  char data[65536];
  size_t data_len;

  js_get_value_string_utf8(env, argv[0], data, 65536, &data_len);

  fprintf(stderr, "%s", data);

  return NULL;
}

static js_value_t *
process_load_addon (js_env_t *env, js_callback_info_t *info) {
  js_value_t *argv[2];
  size_t argc = 2;

  js_get_callback_info(env, info, &argc, argv, NULL, NULL);

  char addon_file[4096];
  uint32_t resolve;

  js_get_value_string_utf8(env, argv[0], addon_file, 4096, NULL);
  js_get_value_uint32(env, argv[1], &resolve);

  return pearjs_addons_load(env, addon_file, (bool) resolve);
}

static js_value_t *
process_resolve_addon (js_env_t *env, js_callback_info_t *info) {
  js_value_t *argv[1];
  size_t argc = 1;

  js_get_callback_info(env, info, &argc, argv, NULL, NULL);

  char addon_file[4096];
  uv_loop_t *loop;

  js_get_env_loop(env, &loop);
  js_get_value_string_utf8(env, argv[0], addon_file, 4096, NULL);

  int err = pearjs_addons_resolve(loop, addon_file, addon_file);
  if (err < 0) {
    js_throw_error(env, NULL, "Could not resolve addon");
    return NULL;
  }

  js_value_t *result;
  js_create_string_utf8(env, addon_file, -1, &result);

  return result;
}

static int
read_file_sync (uv_loop_t *loop, char *name, size_t *size, char **data) {
  uv_fs_t req;
  uv_fs_open(loop, &req, name, UV_FS_O_RDONLY, 0, NULL);

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

static js_value_t *
exists_sync (js_env_t *env, js_callback_info_t *info) {
  js_value_t *argv[1];
  size_t argc = 1;

  js_get_callback_info(env, info, &argc, argv, NULL, NULL);

  char path[4096];

  js_get_value_string_utf8(env, argv[0], path, 4096, NULL);

  uv_loop_t *loop;
  js_get_env_loop(env, &loop);

  uv_fs_t req;
  uv_fs_stat(loop, &req, path, NULL);

  int type = 0;

  if (req.result >= 0) {
    uv_stat_t *st = req.ptr;

    if (st->st_mode & S_IFREG) {
      type = 1;
    } else if (st->st_mode & S_IFDIR) {
      type = 2;
    }
  }

  uv_fs_req_cleanup(&req);

  js_value_t *result;
  js_create_uint32(env, type, &result);

  return result;
}

static js_value_t *
read_source_sync (js_env_t *env, js_callback_info_t *info) {
  js_value_t *argv[1];
  size_t argc = 1;

  js_get_callback_info(env, info, &argc, argv, NULL, NULL);

  char path[4096];

  js_get_value_string_utf8(env, argv[0], path, 4096, NULL);

  uv_loop_t *loop;
  js_get_env_loop(env, &loop);

  size_t size;
  char *data;

  js_value_t *result;

  int err = read_file_sync(loop, path, &size, &data);
  if (err < 0) { // TODO: update when we can create errors
    js_get_null(env, &result);
  } else {
    js_create_string_utf8(env, data, size, &result);
    free(data);
  }

  return result;
}

static js_value_t *
process_hrtime (js_env_t *env, js_callback_info_t *info) {
  js_value_t *argv[2];
  size_t argc = 2;

  js_get_callback_info(env, info, &argc, argv, NULL, NULL);

  size_t arr_len;
  uint32_t *arr;

  size_t prev_len;
  uint32_t *prev;

  js_get_typedarray_info(env, argv[0], NULL, (void **) &arr, &arr_len, NULL, NULL);
  js_get_typedarray_info(env, argv[1], NULL, (void **) &prev, &prev_len, NULL, NULL);

  if (arr_len < 2 || prev_len < 2) return NULL;

  uint64_t p = prev[0] * 1e9 + prev[1];
  uint64_t now = uv_hrtime() - p;

  arr[0] = now / ((uint32_t) 1e9);
  arr[1] = now % ((uint32_t) 1e9);

  return NULL;
}

static js_value_t *
process_exit (js_env_t *env, js_callback_info_t *info) {
  js_value_t *argv[1];
  size_t argc = 1;

  js_get_callback_info(env, info, &argc, argv, NULL, NULL);

  int32_t code;
  js_get_value_int32(env, argv[0], &code);

  exit(code);

  return NULL;
}

static void
pearjs_on_uncaught_exception (js_env_t * env, js_value_t *error, void *data) {
  js_value_t *proc = data;
  js_value_t *fn;

  int err = js_get_named_property(env, proc, "_onfatalexception", &fn);
  if (err < 0) {
    fprintf(stderr, "Error in internal bootstrap.js setup, likely a syntax error\n");
    return;
  }

  err = js_make_callback(env, proc, fn, 1, &error, NULL);
  if (err < 0) {
    js_value_t *exception;
    js_get_and_clear_last_exception(env, &exception);
    js_fatal_exception(env, exception);
  }
}

int
pearjs_runtime_setup (uv_loop_t *loop, js_env_t *env, const char *entry_point) {
  int err;

  js_value_t *proc;
  js_create_object(env, &proc);

  js_on_uncaught_exception(env, pearjs_on_uncaught_exception, proc);

  {
    js_value_t *val;
    js_create_string_utf8(env, PEARJS_PLATFORM, -1, &val);
    js_set_named_property(env, proc, "platform", val);
  }

  {
    js_value_t *val;
    js_create_string_utf8(env, PEARJS_ARCH, -1, &val);
    js_set_named_property(env, proc, "arch", val);
  }

  {
    js_value_t *val;
    js_create_function(env, "stdout", -1, process_stdout, NULL, &val);
    js_set_named_property(env, proc, "_stdout", val);
  }

  {
    js_value_t *val;
    js_create_function(env, "stderr", -1, process_stderr, NULL, &val);
    js_set_named_property(env, proc, "_stderr", val);
  }

  {
    js_value_t *val;
    js_create_function(env, "hrtime", -1, process_hrtime, NULL, &val);
    js_set_named_property(env, proc, "_hrtime", val);
  }

  {
    js_value_t *val;
    js_create_function(env, "exit", -1, process_exit, NULL, &val);
    js_set_named_property(env, proc, "_exit", val);
  }

  {
    js_value_t *val;
    js_create_function(env, "loadAddon", -1, process_load_addon, NULL, &val);
    js_set_named_property(env, proc, "_loadAddon", val);
  }

  {
    js_value_t *val;
    js_create_function(env, "resolveAddon", -1, process_resolve_addon, NULL, &val);
    js_set_named_property(env, proc, "_resolveAddon", val);
  }

  {
    js_value_t *val;
    js_create_function(env, "existsSync", -1, exists_sync, NULL, &val);
    js_set_named_property(env, proc, "_existsSync", val);
  }

  {
    js_value_t *val;
    js_create_function(env, "readSourceSync", -1, read_source_sync, NULL, &val);
    js_set_named_property(env, proc, "_readSourceSync", val);
  }

  {
    js_value_t *val;
    js_create_string_utf8(env, entry_point, -1, &val);
    js_set_named_property(env, proc, "_entryPoint", val);
  }

  js_value_t *global;
  js_get_global(env, &global);

  js_set_named_property(env, global, "process", proc);
  js_set_named_property(env, global, "global", global);

  return 0;
}
