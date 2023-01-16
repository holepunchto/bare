#include <pear.h>
#include <js.h>
#include <uv.h>
#include <stdlib.h>
#include <unistd.h>

#include "runtime.h"
#include "addons.h"
#include "sync_fs.h"
#include "../build/bootstrap.h"

#define PEAR_UV_CHECK(call) \
  { \
    int err = call; \
    if (err < 0) { \
      js_throw_error(env, uv_err_name(err), uv_strerror(err)); \
      return NULL; \
    } \
  }

static js_value_t *
bindings_print (js_env_t *env, js_callback_info_t *info) {
  js_value_t *argv[2];
  size_t argc = 2;

  js_get_callback_info(env, info, &argc, argv, NULL, NULL);

  uint32_t fd;
  size_t data_len;

  js_get_value_uint32(env, argv[0], &fd);
  js_get_value_string_utf8(env, argv[1], NULL, 0, &data_len);

  char *data = malloc(++data_len);

  js_get_value_string_utf8(env, argv[1], data, data_len, &data_len);

  write(fd, data, data_len);
  free(data);

  return NULL;
}

static js_value_t *
bindings_load_addon (js_env_t *env, js_callback_info_t *info) {
  js_value_t *argv[2];
  size_t argc = 2;

  js_get_callback_info(env, info, &argc, argv, NULL, NULL);

  char addon_file[4096];
  uint32_t mode;

  js_get_value_string_utf8(env, argv[0], addon_file, 4096, NULL);
  js_get_value_uint32(env, argv[1], &mode);

  return pear_addons_load(env, addon_file, (int) mode);
}

static js_value_t *
bindings_resolve_addon (js_env_t *env, js_callback_info_t *info) {
  js_value_t *argv[1];
  size_t argc = 1;

  js_get_callback_info(env, info, &argc, argv, NULL, NULL);

  char addon_file[4096];
  uv_loop_t *loop;

  js_get_env_loop(env, &loop);
  js_get_value_string_utf8(env, argv[0], addon_file, 4096, NULL);

  int err = pear_addons_resolve(loop, addon_file, addon_file);
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
bindings_hrtime (js_env_t *env, js_callback_info_t *info) {
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
bindings_exit (js_env_t *env, js_callback_info_t *info) {
  js_value_t *argv[1];
  size_t argc = 1;

  js_get_callback_info(env, info, &argc, argv, NULL, NULL);

  int32_t code;
  js_get_value_int32(env, argv[0], &code);

  exit(code);

  return NULL;
}

static js_value_t *
bindings_cwd (js_env_t *env, js_callback_info_t *info) {
  js_value_t *val;

  char cwd[PEAR_SYNC_FS_MAX_PATH];
  size_t cwd_len;

  PEAR_UV_CHECK(uv_cwd(cwd, &cwd_len))

  js_create_string_utf8(env, cwd, cwd_len, &val);
  return val;
}

static js_value_t *
bindings_env (js_env_t *env, js_callback_info_t *info) {
  uv_env_item_t *items;
  int count;

  PEAR_UV_CHECK(uv_os_environ(&items, &count))

  js_value_t *obj;
  js_create_object(env, &obj);

  for (int i = 0; i < count; i++) {
    uv_env_item_t *item = items + i;

    js_value_t *val;
    js_create_string_utf8(env, item->value, -1, &val);
    js_set_named_property(env, obj, item->name, val);
  }

  uv_os_free_environ(items, count);

  return obj;
}

static js_value_t *
bindings_string_to_buffer (js_env_t *env, js_callback_info_t *info) {
  js_value_t *argv[1];
  size_t argc = 1;

  js_get_callback_info(env, info, &argc, argv, NULL, NULL);

  size_t str_len;
  js_get_value_string_utf8(env, argv[0], NULL, -1, &str_len);

  char *buf;
  js_value_t *result;
  str_len++; // to fit the NULL

  js_create_arraybuffer(env, str_len, (void **) &buf, &result);

  js_get_value_string_utf8(env, argv[0], buf, str_len, NULL);

  return result;
}

static int
trigger_fatal_exception (js_env_t *env) {
  js_value_t *exception;
  js_get_and_clear_last_exception(env, &exception);
  js_fatal_exception(env, exception);
  return 1;
}

static void
pear_on_uncaught_exception (js_env_t * env, js_value_t *error, void *data) {
  js_value_t *exports = data;
  js_value_t *fn;

  int err = js_get_named_property(env, exports, "onfatalexception", &fn);

  if (err < 0) {
    fprintf(stderr, "Error in internal bootstrap.js setup, likely a syntax error\n");
    return;
  }

  bool is_set;
  js_is_function(env, fn, &is_set);

  if (!is_set) {
    fprintf(stderr, "Fatal exception, but no handler set, exiting...\n");
    exit(1);
    return;
  }

  err = js_call_function(env, exports, fn, 1, &error, NULL);
  if (err < 0) trigger_fatal_exception(env);
}

int
pear_runtime_setup (js_env_t *env, pear_runtime_t *config) {
  int err;

  uv_loop_t *loop;
  js_get_env_loop(env, &loop);

  js_create_object(env, &(config->exports));
  js_value_t *exports = config->exports;

  js_on_uncaught_exception(env, pear_on_uncaught_exception, exports);

  {
    js_value_t *val;
    js_create_string_utf8(env, PEAR_PLATFORM, -1, &val);
    js_set_named_property(env, exports, "platform", val);
  }

  {
    js_value_t *val;
    js_create_string_utf8(env, PEAR_ARCH, -1, &val);
    js_set_named_property(env, exports, "arch", val);
  }

  js_value_t *exec_path_val;

  {
    char exec_path[PEAR_SYNC_FS_MAX_PATH];
    size_t exec_path_len;
    uv_exepath(exec_path, &exec_path_len);

    js_create_string_utf8(env, exec_path, exec_path_len, &exec_path_val);
    js_set_named_property(env, exports, "execPath", exec_path_val);
  }

  {
    js_value_t *val;
    js_value_t *str;

    // TODO: the +2 will prob change when we evolve a bit
    js_create_array_with_length(env, config->argc + 2, &val);

    int idx = 0;

    js_create_string_utf8(env, config->main, -1, &str);

    js_set_element(env, val, idx++, exec_path_val);
    js_set_element(env, val, idx++, str);

    for (int i = 0; i < config->argc; i++) {
      char *a = config->argv[i];

      js_create_string_utf8(env, a, -1, &str);
      js_set_element(env, val, idx++, str);
    }

    js_set_named_property(env, exports, "argv", val);
  }

  {
    js_value_t *val;

    js_create_uint32(env, PEAR_ADDONS_DYNAMIC, &val);
    js_set_named_property(env, exports, "ADDONS_DYNAMIC", val);
  }

  {
    js_value_t *val;

    js_create_uint32(env, PEAR_ADDONS_STATIC, &val);
    js_set_named_property(env, exports, "ADDONS_STATIC", val);
  }

  {
    js_value_t *val;

    js_create_uint32(env, PEAR_ADDONS_RESOLVE, &val);
    js_set_named_property(env, exports, "ADDONS_RESOLVE", val);
  }

  {
    js_value_t *val;
    uv_pid_t pid = uv_os_getpid();

    js_create_uint32(env, pid, &val);
    js_set_named_property(env, exports, "pid", val);
  }

  {
    js_value_t *val;
    js_create_function(env, "cwd", -1, bindings_cwd, NULL, &val);
    js_set_named_property(env, exports, "cwd", val);
  }

  {
    js_value_t *val;
    js_create_function(env, "env", -1, bindings_env, NULL, &val);
    js_set_named_property(env, exports, "env", val);
  }

  {
    js_value_t *val;
    js_create_function(env, "print", -1, bindings_print, NULL, &val);
    js_set_named_property(env, exports, "print", val);
  }

  {
    js_value_t *val;
    js_create_function(env, "hrtime", -1, bindings_hrtime, NULL, &val);
    js_set_named_property(env, exports, "hrtime", val);
  }

  {
    js_value_t *val;
    js_create_function(env, "exit", -1, bindings_exit, NULL, &val);
    js_set_named_property(env, exports, "exit", val);
  }

  {
    js_value_t *val;
    js_create_function(env, "stringToBuffer", -1, bindings_string_to_buffer, NULL, &val);
    js_set_named_property(env, exports, "stringToBuffer", val);
  }

  {
    js_value_t *val;
    js_create_function(env, "loadAddon", -1, bindings_load_addon, NULL, &val);
    js_set_named_property(env, exports, "loadAddon", val);
  }

  {
    js_value_t *val;
    js_create_function(env, "resolveAddon", -1, bindings_resolve_addon, NULL, &val);
    js_set_named_property(env, exports, "resolveAddon", val);
  }

  { // TODO: replace me
    js_value_t *val;
    js_create_function(env, "existsSync", -1, exists_sync, NULL, &val);
    js_set_named_property(env, exports, "existsSync", val);
  }

  { // TODO: replace me
    js_value_t *val;
    js_create_function(env, "readSourceSync", -1, read_source_sync, NULL, &val);
    js_set_named_property(env, exports, "readSourceSync", val);
  }

  {
    js_value_t *val;
    js_create_string_utf8(env, config->main, -1, &val);
    js_set_named_property(env, exports, "main", val);
  }

  js_value_t *global;
  js_get_global(env, &global);

  js_set_named_property(env, global, "global", global);

  js_value_t *script;
  js_create_string_utf8(env, (const char *) pear_bootstrap, pear_bootstrap_len, &script);

  js_value_t *bootstrap;
  err = js_run_script(env, script, &bootstrap);
  if (err < 0) return trigger_fatal_exception(env);

  err = js_call_function(env, global, bootstrap, 1, &exports, NULL);
  if (err < 0) return trigger_fatal_exception(env);

  return 0;
}

void
pear_runtime_teardown (js_env_t *env, pear_runtime_t *config) {
  js_value_t *fn;
  js_get_named_property(env, config->exports, "onexit", &fn);

  bool is_set;
  js_is_function(env, fn, &is_set);
  if (!is_set) return;

  int err = js_call_function(env, config->exports, fn, 0, NULL, NULL);
  if (err < 0) trigger_fatal_exception(env);
}
