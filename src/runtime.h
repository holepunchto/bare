#ifndef PEAR_RUNTIME_H
#define PEAR_RUNTIME_H

#include <assert.h>
#include <js.h>
#include <js/ffi.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "../include/pear.h"
#include "addons.h"
#include "fs.h"
#include "pear.js.h"
#include "runtime.h"

#define PEAR_UV_CHECK(call) \
  { \
    int err = call; \
    if (err < 0) { \
      js_throw_error(env, uv_err_name(err), uv_strerror(err)); \
      return NULL; \
    } \
  }

#define PEAR_UV_ERROR_MAP_ITER(NAME, DESC) \
  { \
    js_value_t *key_val; \
    js_create_array_with_length(env, 2, &key_val); \
    js_value_t *val; \
    js_create_array_with_length(env, 2, &val); \
    js_value_t *name; \
    js_create_string_utf8(env, #NAME, -1, &name); \
    js_set_element(env, val, 0, name); \
    js_value_t *desc; \
    js_create_string_utf8(env, DESC, -1, &desc); \
    js_set_element(env, val, 1, desc); \
    js_value_t *key; \
    js_create_int32(env, UV_##NAME, &key); \
    js_set_element(env, key_val, 0, key); \
    js_set_element(env, key_val, 1, val); \
    js_set_element(env, arr, i++, key_val); \
  }

static js_value_t *
bindings_print (js_env_t *env, js_callback_info_t *info) {
  int err;

  js_value_t *argv[2];
  size_t argc = 2;

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);

  assert(argc == 2);

  uint32_t fd;
  err = js_get_value_uint32(env, argv[0], &fd);
  assert(err == 0);

  size_t data_len;
  err = js_get_value_string_utf8(env, argv[1], NULL, 0, &data_len);
  assert(err == 0);

  char *data = malloc(data_len);
  err = js_get_value_string_utf8(env, argv[1], data, data_len, &data_len);
  assert(err == 0);

  write(fd, data, data_len);
  free(data);

  return NULL;
}

static js_value_t *
bindings_load_addon (js_env_t *env, js_callback_info_t *info) {
  pear_t *pear;

  int err;

  js_value_t *argv[1];
  size_t argc = 1;

  err = js_get_callback_info(env, info, &argc, argv, NULL, (void **) &pear);
  assert(err == 0);

  assert(argc == 1);

  char specifier[PEAR_FS_MAX_PATH];
  err = js_get_value_string_utf8(env, argv[0], specifier, PEAR_FS_MAX_PATH, NULL);
  assert(err == 0);

  return pear_addons_load(pear, specifier);
}

static js_value_t *
bindings_resolve_addon (js_env_t *env, js_callback_info_t *info) {
  pear_t *pear;

  int err;

  js_value_t *argv[1];
  size_t argc = 1;

  err = js_get_callback_info(env, info, &argc, argv, NULL, (void **) &pear);
  assert(err == 0);

  assert(argc == 1);

  size_t specifier_len = PEAR_FS_MAX_PATH;
  char specifier[PEAR_FS_MAX_PATH];

  err = js_get_value_string_utf8(env, argv[0], specifier, PEAR_FS_MAX_PATH, NULL);
  assert(err == 0);

  err = pear_addons_resolve(pear, specifier, specifier, &specifier_len);
  if (err < 0) {
    js_throw_errorf(env, NULL, "Could not resolve addon %s", specifier);
    return NULL;
  }

  js_value_t *result;
  err = js_create_string_utf8(env, specifier, -1, &result);
  assert(err == 0);

  return result;
}

static js_value_t *
bindings_hrtime (js_env_t *env, js_callback_info_t *info) {
  int err;

  js_value_t *argv[2];
  size_t argc = 2;

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);

  assert(argc == 2);

  uint32_t *next;
  err = js_get_typedarray_info(env, argv[0], NULL, (void **) &next, NULL, NULL, NULL);
  assert(err == 0);

  uint32_t *prev;
  err = js_get_typedarray_info(env, argv[1], NULL, (void **) &prev, NULL, NULL, NULL);
  assert(err == 0);

  uint64_t then = prev[0] * 1e9 + prev[1];
  uint64_t now = uv_hrtime() - then;

  next[0] = now / ((uint32_t) 1e9);
  next[1] = now % ((uint32_t) 1e9);

  return NULL;
}

static js_value_t *
bindings_exit (js_env_t *env, js_callback_info_t *info) {
  int err;

  js_value_t *argv[1];
  size_t argc = 1;

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);

  int32_t code;
  err = js_get_value_int32(env, argv[0], &code);
  assert(err == 0);

  exit(code);

  return NULL;
}

static js_value_t *
bindings_cwd (js_env_t *env, js_callback_info_t *info) {
  int err;

  js_value_t *val;

  size_t cwd_len = PEAR_FS_MAX_PATH;
  char cwd[PEAR_FS_MAX_PATH];

  PEAR_UV_CHECK(uv_cwd(cwd, &cwd_len))

  err = js_create_string_utf8(env, cwd, cwd_len, &val);
  assert(err == 0);

  return val;
}

static js_value_t *
bindings_env (js_env_t *env, js_callback_info_t *info) {
  int err;

  uv_env_item_t *items;
  int count;

  PEAR_UV_CHECK(uv_os_environ(&items, &count))

  js_value_t *obj;
  err = js_create_object(env, &obj);
  assert(err == 0);

  for (int i = 0; i < count; i++) {
    uv_env_item_t *item = items + i;

    js_value_t *val;
    err = js_create_string_utf8(env, item->value, -1, &val);
    assert(err == 0);

    err = js_set_named_property(env, obj, item->name, val);
    assert(err == 0);
  }

  uv_os_free_environ(items, count);

  return obj;
}

static js_value_t *
bindings_set_title (js_env_t *env, js_callback_info_t *info) {
  int err;

  js_value_t *argv[1];
  size_t argc = 1;

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);

  assert(argc == 1);

  size_t data_len;
  err = js_get_value_string_utf8(env, argv[0], NULL, 0, &data_len);
  assert(err == 0);

  char *data = malloc(++data_len);
  err = js_get_value_string_utf8(env, argv[0], data, data_len, &data_len);
  assert(err == 0);

  err = uv_set_process_title(data);
  assert(err == 0);

  free(data);

  return NULL;
}

static js_value_t *
bindings_get_title (js_env_t *env, js_callback_info_t *info) {
  int er;

  js_value_t *result;

  char *title = malloc(256);
  err = uv_get_process_title(title, 256);
  if (err) memcpy(title, "pear", 5);

  err = js_create_string_utf8(env, title, -1, &result);
  assert(err == 0);

  free(title);

  return result;
}

static js_value_t *
bindings_suspend (js_env_t *env, js_callback_info_t *info) {
  pear_t *pear;

  int err = js_get_callback_info(env, info, NULL, NULL, NULL, (void **) &pear);
  assert(err == 0);

  pear_suspend(pear);

  return NULL;
}

static js_value_t *
bindings_resume (js_env_t *env, js_callback_info_t *info) {
  pear_t *pear;

  int err = js_get_callback_info(env, info, NULL, NULL, NULL, (void **) &pear);
  assert(err == 0);

  pear_resume(pear);

  return NULL;
}

static js_value_t *
bindings_exists (js_env_t *env, js_callback_info_t *info) {
  int err;

  pear_t *pear;

  js_value_t *argv[1];
  size_t argc = 1;

  err = js_get_callback_info(env, info, &argc, argv, NULL, (void **) &pear);
  assert(err == 0);

  assert(argc == 1);

  char path[PEAR_FS_MAX_PATH];
  err = js_get_value_string_utf8(env, argv[0], path, PEAR_FS_MAX_PATH, NULL);
  assert(err == 0);

  bool exists = pear_fs_exists_sync(pear, path);

  js_value_t *result;
  err = js_create_uint32(env, exists, &result);
  assert(err == 0);

  return result;
}

static js_value_t *
bindings_read (js_env_t *env, js_callback_info_t *info) {
  int err;

  pear_t *pear;

  js_value_t *argv[1];
  size_t argc = 1;

  err = js_get_callback_info(env, info, &argc, argv, NULL, (void **) &pear);
  assert(err == 0);

  assert(argc == 1);

  char path[PEAR_FS_MAX_PATH];
  err = js_get_value_string_utf8(env, argv[0], path, PEAR_FS_MAX_PATH, NULL);
  assert(err == 0);

  js_value_t *buffer;

  PEAR_UV_CHECK(pear_fs_read_sync(pear, path, &buffer))

  return buffer;
}

static int
trigger_fatal_exception (js_env_t *env) {
  js_value_t *exception;
  js_get_and_clear_last_exception(env, &exception);
  js_fatal_exception(env, exception);
  return 1;
}

static void
pear_on_uncaught_exception (js_env_t *env, js_value_t *error, void *data) {
  pear_t *pear = (pear_t *) data;

  int err;

  js_value_t *fn;
  err = js_get_named_property(env, pear->runtime.exports, "onuncaughtexception", &fn);
  if (err < 0) goto err;

  bool is_set;
  js_is_function(env, fn, &is_set);
  if (!is_set) goto err;

  js_value_t *global;
  js_get_global(env, &global);

  err = js_call_function(env, global, fn, 1, (js_value_t *[]){error}, NULL);
  if (err < 0) trigger_fatal_exception(env);

  return;

err : {
  js_value_t *stack;
  err = js_get_named_property(env, error, "stack", &stack);
  assert(err == 0);

  size_t len;
  err = js_get_value_string_utf8(env, stack, NULL, 0, &len);
  assert(err == 0);

  char *str = malloc(len + 1);
  err = js_get_value_string_utf8(env, stack, str, len + 1, NULL);
  assert(err == 0);

  fprintf(stderr, "Uncaught %s\n", str);
  exit(1);
}
}

static void
pear_on_unhandled_rejection (js_env_t *env, js_value_t *reason, js_value_t *promise, void *data) {
  pear_t *pear = (pear_t *) data;

  int err;

  js_value_t *fn;
  err = js_get_named_property(env, pear->runtime.exports, "onunhandledrejection", &fn);
  if (err < 0) goto err;

  bool is_set;
  js_is_function(env, fn, &is_set);
  if (!is_set) goto err;

  js_value_t *global;
  js_get_global(env, &global);

  err = js_call_function(env, global, fn, 2, (js_value_t *[]){reason, promise}, NULL);
  if (err < 0) trigger_fatal_exception(env);

  return;

err : {
  js_value_t *stack;
  err = js_get_named_property(env, reason, "stack", &stack);
  assert(err == 0);

  size_t len;
  err = js_get_value_string_utf8(env, stack, NULL, 0, &len);
  assert(err == 0);

  char *str = malloc(len + 1);
  err = js_get_value_string_utf8(env, stack, str, len + 1, NULL);
  assert(err == 0);

  fprintf(stderr, "Uncaught (in promise) %s\n", str);
  exit(1);
}
}

static inline int
pear_runtime_setup (pear_t *pear) {
  js_env_t *env = pear->env;

  int err;

  err = js_create_object(env, &pear->runtime.exports);
  assert(err == 0);

  err = js_on_uncaught_exception(env, pear_on_uncaught_exception, (void *) pear);
  assert(err == 0);

  err = js_on_unhandled_rejection(env, pear_on_unhandled_rejection, (void *) pear);
  assert(err == 0);

  js_value_t *exports = pear->runtime.exports;

  {
    js_value_t *versions;
    js_create_object(env, &versions);
    js_value_t *val;

    js_create_string_utf8(env, "0.0.0", -1, &val);
    js_set_named_property(env, versions, "pear", val);

    if (js_platform_version) {
      js_create_string_utf8(env, js_platform_version, -1, &val);
    } else {
      js_create_string_utf8(env, "unknown", -1, &val);
    }

    js_set_named_property(env, versions, js_platform_identifier, val);

    js_create_string_utf8(env, uv_version_string(), -1, &val);
    js_set_named_property(env, versions, "uv", val);

    js_set_named_property(env, exports, "versions", versions);
  }

  {
    js_value_t *val;
    js_create_int32(env, 0, &val);
    js_set_named_property(env, exports, "exitCode", val);
  }

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
    char exec_path[PEAR_FS_MAX_PATH];
    size_t exec_path_len = PEAR_FS_MAX_PATH;
    uv_exepath(exec_path, &exec_path_len);

    js_create_string_utf8(env, exec_path, exec_path_len, &exec_path_val);
    js_set_named_property(env, exports, "execPath", exec_path_val);
  }

  {
    js_value_t *val;
    js_value_t *str;

    js_create_array_with_length(env, pear->runtime.argc, &val);

    int idx = 0;

    js_set_element(env, val, idx++, exec_path_val);

    for (int i = 1; i < pear->runtime.argc; i++) {
      js_create_string_utf8(env, pear->runtime.argv[i], -1, &str);
      js_set_element(env, val, idx++, str);
    }

    js_set_named_property(env, exports, "argv", val);
  }

  {
    js_value_t *arr;
    int i = 0;

    js_create_array(env, &arr);
    UV_ERRNO_MAP(PEAR_UV_ERROR_MAP_ITER)

    js_set_named_property(env, exports, "errnos", arr);
  }

  {
    js_value_t *val;
    js_create_uint32(env, uv_os_getpid(), &val);
    js_set_named_property(env, exports, "pid", val);
  }

  {
    js_value_t *val;
    js_create_function(env, "setTitle", -1, bindings_set_title, NULL, &val);
    js_set_named_property(env, exports, "setTitle", val);
  }

  {
    js_value_t *val;
    js_create_function(env, "getTitle", -1, bindings_get_title, NULL, &val);
    js_set_named_property(env, exports, "getTitle", val);
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
    js_create_function(env, "loadAddon", -1, bindings_load_addon, (void *) pear, &val);
    js_set_named_property(env, exports, "loadAddon", val);
  }

  {
    js_value_t *val;
    js_create_function(env, "resolveAddon", -1, bindings_resolve_addon, (void *) pear, &val);
    js_set_named_property(env, exports, "resolveAddon", val);
  }

  {
    js_value_t *val;
    js_create_function(env, "suspend", -1, bindings_suspend, (void *) pear, &val);
    js_set_named_property(env, exports, "suspend", val);
  }

  {
    js_value_t *val;
    js_create_function(env, "resume", -1, bindings_resume, (void *) pear, &val);
    js_set_named_property(env, exports, "resume", val);
  }

  {
    js_value_t *val;
    js_create_function(env, "exists", -1, bindings_exists, (void *) pear, &val);
    js_set_named_property(env, exports, "exists", val);
  }

  {
    js_value_t *val;
    js_create_function(env, "read", -1, bindings_read, (void *) pear, &val);
    js_set_named_property(env, exports, "read", val);
  }

  {
    js_value_t *val;
    js_create_object(env, &val);
    js_set_named_property(env, exports, "data", val);
  }

  js_value_t *global;
  js_get_global(env, &global);

  js_set_named_property(env, global, "global", global);

  js_value_t *script;
  js_create_string_utf8(env, (const char *) pear_bundle, pear_bundle_len, &script);

  js_value_t *entry;
  err = js_run_script(env, "pear:pear.js", -1, 0, script, &entry);
  if (err < 0) return trigger_fatal_exception(env);

  err = js_call_function(env, global, entry, 1, &exports, NULL);
  if (err < 0) return trigger_fatal_exception(env);

  return 0;
}

static inline void
pear_runtime_before_teardown (pear_t *pear) {
  js_env_t *env = pear->env;

  js_value_t *fn;
  js_get_named_property(env, pear->runtime.exports, "onbeforeexit", &fn);

  bool is_set;
  js_is_function(env, fn, &is_set);
  if (!is_set) return;

  js_value_t *global;
  js_get_global(env, &global);

  int err = js_call_function(env, global, fn, 0, NULL, NULL);
  if (err < 0) trigger_fatal_exception(env);
}

static inline void
pear_runtime_teardown (pear_t *pear, int *exit_code) {
  js_env_t *env = pear->env;

  if (exit_code) *exit_code = 0;

  js_value_t *fn;
  js_get_named_property(env, pear->runtime.exports, "onexit", &fn);

  bool is_set;
  js_is_function(env, fn, &is_set);
  if (!is_set) return;

  js_value_t *global;
  js_get_global(env, &global);

  int err = js_call_function(env, global, fn, 0, NULL, NULL);
  if (err < 0) trigger_fatal_exception(env);

  if (exit_code) {
    js_value_t *val;
    js_get_named_property(env, pear->runtime.exports, "exitCode", &val);
    js_get_value_int32(env, val, exit_code);
  }
}

static inline int
pear_runtime_run (pear_t *pear, const char *filename, const uv_buf_t *source) {
  js_env_t *env = pear->env;

  int err;

  js_value_t *run;
  err = js_get_named_property(env, pear->runtime.exports, "run", &run);
  assert(err == 0);

  js_value_t *args[2];
  err = js_create_string_utf8(env, filename, -1, &args[0]);
  if (err < 0) return err;

  if (source) {
    js_value_t *arraybuffer;
    void *data;

    err = js_create_arraybuffer(env, source->len, &data, &arraybuffer);
    if (err < 0) return err;

    memcpy(data, source->base, source->len);

    err = js_create_typedarray(env, js_uint8_array, source->len, arraybuffer, 0, &args[1]);
    if (err < 0) return err;
  } else {
    js_get_undefined(env, &args[1]);
  }

  js_value_t *global;
  js_get_global(env, &global);

  err = js_call_function(env, global, run, 2, args, NULL);
  if (err < 0) return err;

  return 0;
}

static inline void
pear_runtime_suspend (pear_t *pear) {
  js_env_t *env = pear->env;

  js_value_t *fn;
  js_get_named_property(env, pear->runtime.exports, "onsuspend", &fn);

  bool is_set;
  js_is_function(env, fn, &is_set);
  if (!is_set) return;

  js_value_t *global;
  js_get_global(env, &global);

  int err = js_call_function(env, global, fn, 0, NULL, NULL);
  if (err < 0) trigger_fatal_exception(env);
}

static inline void
pear_runtime_resume (pear_t *pear) {
  js_env_t *env = pear->env;

  js_value_t *fn;
  js_get_named_property(env, pear->runtime.exports, "onresume", &fn);

  bool is_set;
  js_is_function(env, fn, &is_set);
  if (!is_set) return;

  js_value_t *global;
  js_get_global(env, &global);

  int err = js_call_function(env, global, fn, 0, NULL, NULL);
  if (err < 0) trigger_fatal_exception(env);
}

static inline int
pear_runtime_get_data (pear_t *pear, const char *key, void **result) {
  js_env_t *env = pear->env;

  int err;

  js_value_t *data, *external;

  err = js_get_named_property(env, pear->runtime.exports, "data", &data);
  assert(err == 0);

  err = js_get_named_property(env, data, key, &external);
  if (err < 0) return err;

  err = js_get_value_external(env, external, result);
  if (err < 0) return err;

  return 0;
}

static inline int
pear_runtime_set_data (pear_t *pear, const char *key, void *value) {
  js_env_t *env = pear->env;

  int err;

  js_value_t *data, *external;

  err = js_get_named_property(env, pear->runtime.exports, "data", &data);
  assert(err == 0);

  err = js_create_external(env, value, NULL, NULL, &external);
  if (err < 0) return err;

  err = js_set_named_property(env, data, key, external);
  if (err < 0) return err;

  return 0;
}

#endif // PEAR_RUNTIME_H
