#ifndef PEAR_RUNTIME_H
#define PEAR_RUNTIME_H

#include <assert.h>
#include <js.h>
#include <js/ffi.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "../include/pear.h"
#include "addons.h"
#include "pear.js.h"
#include "types.h"

#ifdef PEAR_PLATFORM_ANDROID
#include "runtime/android.h"
#else
#include "runtime/posix.h"
#endif

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

static void
pear_runtime_on_uncaught_exception (js_env_t *env, js_value_t *error, void *data) {
  pear_runtime_t *runtime = (pear_runtime_t *) data;

  int err;

  js_value_t *fn;
  err = js_get_named_property(env, runtime->exports, "onuncaughtexception", &fn);
  if (err < 0) goto err;

  bool is_set;
  js_is_function(env, fn, &is_set);
  if (!is_set) goto err;

  js_value_t *global;
  js_get_global(env, &global);

  err = js_call_function(env, global, fn, 1, (js_value_t *[]){error}, NULL);
  if (err < 0) goto err;

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

  err = pear_runtime__print_error("Uncaught %s\n", str);
  assert(err >= 0);

  exit(1);
}
}

static void
pear_runtime_on_unhandled_rejection (js_env_t *env, js_value_t *reason, js_value_t *promise, void *data) {
  pear_runtime_t *runtime = (pear_runtime_t *) data;

  int err;

  js_value_t *fn;
  err = js_get_named_property(env, runtime->exports, "onunhandledrejection", &fn);
  if (err < 0) goto err;

  bool is_set;
  js_is_function(env, fn, &is_set);
  if (!is_set) goto err;

  js_value_t *global;
  js_get_global(env, &global);

  err = js_call_function(env, global, fn, 2, (js_value_t *[]){reason, promise}, NULL);
  if (err < 0) goto err;

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

  err = pear_runtime__print_error("Uncaught (in promise) %s\n", str);
  assert(err >= 0);

  exit(1);
}
}

static inline void
pear_runtime_on_before_exit (pear_runtime_t *runtime) {
  js_env_t *env = runtime->env;

  js_value_t *fn;
  js_get_named_property(env, runtime->exports, "onbeforeexit", &fn);

  bool is_set;
  js_is_function(env, fn, &is_set);

  if (is_set) {
    js_value_t *global;
    js_get_global(env, &global);

    int err = js_call_function(env, global, fn, 0, NULL, NULL);
    assert(err == 0);
  }

  if (runtime->process->on_before_exit) runtime->process->on_before_exit(runtime->process);
}

static inline void
pear_runtime_on_exit (pear_runtime_t *runtime, int *exit_code) {
  uv_rwlock_rdlock(&runtime->process->locks.threads);

  while (runtime->process->threads) {
    uv_thread_t id = runtime->process->threads->thread.id;

    uv_rwlock_rdunlock(&runtime->process->locks.threads);

    uv_thread_join(&id);

    uv_rwlock_rdlock(&runtime->process->locks.threads);
  }

  uv_rwlock_rdunlock(&runtime->process->locks.threads);

  js_env_t *env = runtime->env;

  if (exit_code) *exit_code = 0;

  js_value_t *fn;
  js_get_named_property(env, runtime->exports, "onexit", &fn);

  bool is_set;
  js_is_function(env, fn, &is_set);

  if (is_set) {
    js_value_t *global;
    js_get_global(env, &global);

    int err = js_call_function(env, global, fn, 0, NULL, NULL);
    assert(err == 0);
  }

  if (runtime->process->on_exit) runtime->process->on_exit(runtime->process);

  if (exit_code) {
    js_value_t *val;
    js_get_named_property(env, runtime->exports, "exitCode", &val);
    js_get_value_int32(env, val, exit_code);
  }
}

static inline void
pear_runtime_on_thread_exit (pear_runtime_t *runtime) {
  js_env_t *env = runtime->env;

  js_value_t *fn;
  js_get_named_property(env, runtime->exports, "onthreadexit", &fn);

  bool is_set;
  js_is_function(env, fn, &is_set);

  if (is_set) {
    js_value_t *global;
    js_get_global(env, &global);

    int err = js_call_function(env, global, fn, 0, NULL, NULL);
    assert(err == 0);
  }
}

static inline void
pear_runtime_on_suspend (pear_runtime_t *runtime) {
  js_env_t *env = runtime->env;

  js_value_t *fn;
  js_get_named_property(env, runtime->exports, "onsuspend", &fn);

  bool is_set;
  js_is_function(env, fn, &is_set);

  if (is_set) {
    js_value_t *global;
    js_get_global(env, &global);

    int err = js_call_function(env, global, fn, 0, NULL, NULL);
    assert(err == 0);
  }

  if (runtime->process->on_suspend) runtime->process->on_suspend(runtime->process);
}

static inline void
pear_runtime_on_idle (pear_runtime_t *runtime) {
  js_env_t *env = runtime->env;

  js_value_t *fn;
  js_get_named_property(env, runtime->exports, "onidle", &fn);

  bool is_set;
  js_is_function(env, fn, &is_set);

  if (is_set) {
    js_value_t *global;
    js_get_global(env, &global);

    int err = js_call_function(env, global, fn, 0, NULL, NULL);
    assert(err == 0);
  }

  if (runtime->process->on_idle) runtime->process->on_idle(runtime->process);
}

static inline void
pear_runtime_on_resume (pear_runtime_t *runtime) {
  js_env_t *env = runtime->env;

  js_value_t *fn;
  js_get_named_property(env, runtime->exports, "onresume", &fn);

  bool is_set;
  js_is_function(env, fn, &is_set);

  if (is_set) {
    js_value_t *global;
    js_get_global(env, &global);

    int err = js_call_function(env, global, fn, 0, NULL, NULL);
    assert(err == 0);
  }

  if (runtime->process->on_resume) runtime->process->on_resume(runtime->process);
}

static js_value_t *
pear_runtime_print_info (js_env_t *env, js_callback_info_t *info) {
  int err;

  js_value_t *argv[1];
  size_t argc = 1;

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);

  assert(argc == 1);

  size_t data_len;
  err = js_get_value_string_utf8(env, argv[0], NULL, 0, &data_len);
  assert(err == 0);

  data_len += 1 /* NULL */;

  char *data = malloc(data_len);
  err = js_get_value_string_utf8(env, argv[0], data, data_len, &data_len);
  assert(err == 0);

  err = pear_runtime__print_info("%s", data);
  assert(err >= 0);

  free(data);

  return NULL;
}

static js_value_t *
pear_runtime_print_error (js_env_t *env, js_callback_info_t *info) {
  int err;

  js_value_t *argv[1];
  size_t argc = 1;

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);

  assert(argc == 1);

  size_t data_len;
  err = js_get_value_string_utf8(env, argv[0], NULL, 0, &data_len);
  assert(err == 0);

  data_len += 1 /* NULL */;

  char *data = malloc(data_len);
  err = js_get_value_string_utf8(env, argv[0], data, data_len, &data_len);
  assert(err == 0);

  err = pear_runtime__print_error("%s", data);
  assert(err >= 0);

  free(data);

  return NULL;
}

static js_value_t *
pear_runtime_load_addon (js_env_t *env, js_callback_info_t *info) {
  pear_runtime_t *runtime;

  int err;

  js_value_t *argv[1];
  size_t argc = 1;

  err = js_get_callback_info(env, info, &argc, argv, NULL, (void **) &runtime);
  assert(err == 0);

  assert(argc == 1);

  char specifier[4096];
  err = js_get_value_string_utf8(env, argv[0], specifier, 4096, NULL);
  assert(err == 0);

  return pear_addons_load(runtime, specifier);
}

static js_value_t *
pear_runtime_resolve_addon (js_env_t *env, js_callback_info_t *info) {
  pear_runtime_t *runtime;

  int err;

  js_value_t *argv[1];
  size_t argc = 1;

  err = js_get_callback_info(env, info, &argc, argv, NULL, (void **) &runtime);
  assert(err == 0);

  assert(argc == 1);

  size_t specifier_len = 4096;
  char specifier[4096];

  err = js_get_value_string_utf8(env, argv[0], specifier, 4096, NULL);
  assert(err == 0);

  err = pear_addons_resolve(runtime, specifier, specifier, &specifier_len);
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
pear_runtime_hrtime (js_env_t *env, js_callback_info_t *info) {
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
pear_runtime_cwd (js_env_t *env, js_callback_info_t *info) {
  int err;

  js_value_t *val;

  size_t cwd_len = 4096;
  char cwd[4096];

  err = uv_cwd(cwd, &cwd_len);
  if (err < 0) {
    js_throw_error(env, uv_err_name(err), uv_strerror(err));
    return NULL;
  }

  err = js_create_string_utf8(env, cwd, cwd_len, &val);
  assert(err == 0);

  return val;
}

static js_value_t *
pear_runtime_chdir (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);

  assert(argc == 1);

  size_t dir_len;
  err = js_get_value_string_utf8(env, argv[0], NULL, 0, &dir_len);
  assert(err == 0);

  char *dir = malloc(++dir_len);
  err = js_get_value_string_utf8(env, argv[0], dir, dir_len, &dir_len);
  assert(err == 0);

  err = uv_chdir(dir);

  free(dir);

  if (err < 0) {
    js_throw_error(env, uv_err_name(err), uv_strerror(err));
    return NULL;
  }

  return NULL;
}

static js_value_t *
pear_runtime_get_env_keys (js_env_t *env, js_callback_info_t *info) {
  pear_runtime_t *runtime;

  int err;

  err = js_get_callback_info(env, info, NULL, NULL, NULL, (void **) &runtime);
  assert(err == 0);

  uv_env_item_t *items;
  int len;

  uv_rwlock_rdlock(&runtime->process->locks.env);

  err = uv_os_environ(&items, &len);

  uv_rwlock_rdunlock(&runtime->process->locks.env);

  if (err < 0) {
    js_throw_error(env, uv_err_name(err), uv_strerror(err));
    return NULL;
  }

  js_value_t *result;
  err = js_create_array_with_length(env, len, &result);
  assert(err == 0);

  for (int i = 0; i < len; i++) {
    uv_env_item_t *item = &items[i];

    js_value_t *val;
    err = js_create_string_utf8(env, item->name, -1, &val);
    assert(err == 0);

    err = js_set_element(env, result, i, val);
    assert(err == 0);
  }

  uv_os_free_environ(items, len);

  return result;
}

static js_value_t *
pear_runtime_get_env (js_env_t *env, js_callback_info_t *info) {
  pear_runtime_t *runtime;

  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, (void **) &runtime);
  assert(err == 0);

  assert(argc == 1);

  size_t name_len;
  err = js_get_value_string_utf8(env, argv[0], NULL, 0, &name_len);
  assert(err == 0);

  char *name = malloc(++name_len);
  err = js_get_value_string_utf8(env, argv[0], name, name_len, &name_len);
  assert(err == 0);

  uv_rwlock_rdlock(&runtime->process->locks.env);

  size_t value_len = 1;
  err = uv_os_getenv(name, "", &value_len);

  js_value_t *result;

  if (err == UV_ENOENT) {
    err = js_get_undefined(env, &result);
    assert(err == 0);
  } else {
    assert(err == UV_ENOBUFS);

    char *value = malloc(value_len);
    err = uv_os_getenv(name, value, &value_len);
    assert(err == 0);

    err = js_create_string_utf8(env, value, value_len, &result);
    assert(err == 0);

    free(value);
  }

  uv_rwlock_rdunlock(&runtime->process->locks.env);

  free(name);

  return result;
}

static js_value_t *
pear_runtime_has_env (js_env_t *env, js_callback_info_t *info) {
  pear_runtime_t *runtime;

  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, (void **) &runtime);
  assert(err == 0);

  assert(argc == 1);

  size_t name_len;
  err = js_get_value_string_utf8(env, argv[0], NULL, 0, &name_len);
  assert(err == 0);

  char *name = malloc(++name_len);
  err = js_get_value_string_utf8(env, argv[0], name, name_len, &name_len);
  assert(err == 0);

  uv_rwlock_rdlock(&runtime->process->locks.env);

  size_t value_len = 1;
  err = uv_os_getenv(name, "", &value_len);

  uv_rwlock_rdunlock(&runtime->process->locks.env);

  if (err != 0 && err != UV_ENOENT && err != UV_ENOBUFS) {
    js_throw_error(env, uv_err_name(err), uv_strerror(err));
    free(name);
    return NULL;
  }

  free(name);

  js_value_t *result;
  err = js_get_boolean(env, err != UV_ENOENT, &result);
  assert(err == 0);

  return result;
}

static js_value_t *
pear_runtime_set_env (js_env_t *env, js_callback_info_t *info) {
  pear_runtime_t *runtime;

  int err;

  size_t argc = 2;
  js_value_t *argv[2];

  err = js_get_callback_info(env, info, &argc, argv, NULL, (void **) &runtime);
  assert(err == 0);

  assert(argc == 2);

  size_t name_len;
  err = js_get_value_string_utf8(env, argv[0], NULL, 0, &name_len);
  assert(err == 0);

  char *name = malloc(++name_len);
  err = js_get_value_string_utf8(env, argv[0], name, name_len, &name_len);
  assert(err == 0);

  size_t value_len;
  err = js_get_value_string_utf8(env, argv[1], NULL, 0, &value_len);
  assert(err == 0);

  char *value = malloc(++value_len);
  err = js_get_value_string_utf8(env, argv[1], value, value_len, &value_len);
  assert(err == 0);

  uv_rwlock_wrlock(&runtime->process->locks.env);

  err = uv_os_setenv(name, value);

  uv_rwlock_wrunlock(&runtime->process->locks.env);

  if (err < 0) {
    js_throw_error(env, uv_err_name(err), uv_strerror(err));
    free(name);
    free(value);
    return NULL;
  }

  free(name);
  free(value);

  return NULL;
}

static js_value_t *
pear_runtime_unset_env (js_env_t *env, js_callback_info_t *info) {
  pear_runtime_t *runtime;

  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, (void **) &runtime);
  assert(err == 0);

  assert(argc == 1);

  size_t name_len;
  err = js_get_value_string_utf8(env, argv[0], NULL, 0, &name_len);
  assert(err == 0);

  char *name = malloc(++name_len);
  err = js_get_value_string_utf8(env, argv[0], name, name_len, &name_len);
  assert(err == 0);

  uv_rwlock_wrlock(&runtime->process->locks.env);

  err = uv_os_unsetenv(name);

  uv_rwlock_wrunlock(&runtime->process->locks.env);

  if (err < 0) {
    js_throw_error(env, uv_err_name(err), uv_strerror(err));
    free(name);
    return NULL;
  }

  free(name);

  return NULL;
}

static js_value_t *
pear_runtime_set_title (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

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
pear_runtime_get_title (js_env_t *env, js_callback_info_t *info) {
  int err;

  char *title = malloc(256);
  err = uv_get_process_title(title, 256);
  if (err) memcpy(title, "pear", 5);

  js_value_t *result;
  err = js_create_string_utf8(env, title, -1, &result);
  assert(err == 0);

  free(title);

  return result;
}

static js_value_t *
pear_runtime_exit (js_env_t *env, js_callback_info_t *info) {
  pear_runtime_t *runtime;

  int err;

  err = js_get_callback_info(env, info, NULL, NULL, NULL, (void **) &runtime);
  assert(err == 0);

  pear_exit(runtime->process, -1);

  return NULL;
}

static js_value_t *
pear_runtime_suspend (js_env_t *env, js_callback_info_t *info) {
  pear_runtime_t *runtime;

  int err;

  err = js_get_callback_info(env, info, NULL, NULL, NULL, (void **) &runtime);
  assert(err == 0);

  pear_suspend(runtime->process);

  return NULL;
}

static js_value_t *
pear_runtime_resume (js_env_t *env, js_callback_info_t *info) {
  pear_runtime_t *runtime;

  int err;

  err = js_get_callback_info(env, info, NULL, NULL, NULL, (void **) &runtime);
  assert(err == 0);

  pear_resume(runtime->process);

  return NULL;
}

static void
pear_runtime_on_thread (void *data);

static js_value_t *
pear_runtime_setup_thread (js_env_t *env, js_callback_info_t *info) {
  int err;

  pear_runtime_t *runtime;

  size_t argc = 4;
  js_value_t *argv[4];

  err = js_get_callback_info(env, info, &argc, argv, NULL, (void **) &runtime);
  assert(err == 0);

  assert(argc == 4);

  uv_loop_t *loop = malloc(sizeof(uv_loop_t));

  err = uv_loop_init(loop);
  if (err < 0) {
    js_throw_error(env, uv_err_name(err), uv_strerror(err));
    free(loop);
    return NULL;
  }

  size_t str_len;
  err = js_get_value_string_utf8(env, argv[0], NULL, 0, &str_len);
  assert(err == 0);

  char *str = malloc(str_len + 1);
  err = js_get_value_string_utf8(env, argv[0], str, str_len + 1, NULL);
  assert(err == 0);

  size_t source_len = 0;
  void *source = NULL;
  bool has_source;

  err = js_is_typedarray(env, argv[1], &has_source);
  assert(err == 0);

  if (has_source) {
    err = js_get_typedarray_info(env, argv[1], NULL, &source, &source_len, NULL, NULL);
    assert(err == 0);
  }

  size_t data_len = 0;
  void *data = NULL;
  bool has_data;

  err = js_is_typedarray(env, argv[2], &has_data);
  assert(err == 0);

  if (has_data) {
    err = js_get_typedarray_info(env, argv[2], NULL, &data, &data_len, NULL, NULL);
    assert(err == 0);
  }

  uint32_t stack_size;
  err = js_get_value_uint32(env, argv[3], &stack_size);
  assert(err == 0);

  pear_thread_list_t *next = malloc(sizeof(pear_thread_list_t));

  next->previous = NULL;
  next->next = NULL;

  pear_thread_t *thread = &next->thread;

  err = uv_sem_init(&thread->ready, 0);
  assert(err == 0);

  thread->filename = str;

  thread->source = uv_buf_init(source, source_len);
  thread->has_source = has_source;

  thread->data = uv_buf_init(data, data_len);
  thread->has_data = has_data;

  thread->runtime.loop = loop;

  thread->runtime.process = runtime->process;

  thread->runtime.platform = runtime->platform;
  thread->runtime.env = NULL;

  thread->runtime.argc = runtime->argc;
  thread->runtime.argv = runtime->argv;

  uv_rwlock_wrlock(&runtime->process->locks.threads);

  if (runtime->process->threads) {
    next->next = runtime->process->threads;
    next->next->previous = next;
  }

  runtime->process->threads = next;

  uv_rwlock_wrunlock(&runtime->process->locks.threads);

  uv_thread_options_t options = {
    .flags = UV_THREAD_HAS_STACK_SIZE,
    .stack_size = stack_size,
  };

  err = uv_thread_create_ex(&thread->id, &options, pear_runtime_on_thread, (void *) thread);

  if (err < 0) {
    js_throw_error(env, uv_err_name(err), uv_strerror(err));
    free(next);
    free(loop);
    return NULL;
  }

  uv_sem_wait(&thread->ready);

  js_value_t *result;
  err = js_create_external(env, (void *) thread->id, NULL, NULL, &result);
  assert(err == 0);

  return result;
}

static js_value_t *
pear_runtime_join_thread (js_env_t *env, js_callback_info_t *info) {
  int err;

  pear_runtime_t *runtime;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, (void **) &runtime);
  assert(err == 0);

  assert(argc == 1);

  uv_thread_t thread;
  err = js_get_value_external(env, argv[0], (void **) &thread);
  assert(err == 0);

  uv_thread_join(&thread);

  return NULL;
}

static js_value_t *
pear_runtime_stop_current_thread (js_env_t *env, js_callback_info_t *info) {
  int err;

  uv_loop_t *loop;
  err = js_get_env_loop(env, &loop);
  assert(err == 0);

  uv_stop(loop);

  return NULL;
}

static inline void
pear_runtime_setup (pear_runtime_t *runtime) {
  int err;

  js_env_t *env = runtime->env;

  err = js_create_object(env, &runtime->exports);
  assert(err == 0);

  err = js_on_uncaught_exception(env, pear_runtime_on_uncaught_exception, (void *) runtime);
  assert(err == 0);

  err = js_on_unhandled_rejection(env, pear_runtime_on_unhandled_rejection, (void *) runtime);
  assert(err == 0);

  js_value_t *exports = runtime->exports;

  {
    js_value_t *versions;
    js_create_object(env, &versions);

    js_value_t *val;

    js_create_string_utf8(env, PEAR_VERSION, -1, &val);
    js_set_named_property(env, versions, "pear", val);

    js_create_string_utf8(env, PEAR_STRING(PEAR_MODULE_VERSION), -1, &val);
    js_set_named_property(env, versions, "modules", val);

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
    char exec_path[4096];
    size_t exec_path_len = 4096;
    uv_exepath(exec_path, &exec_path_len);

    js_create_string_utf8(env, exec_path, exec_path_len, &exec_path_val);
    js_set_named_property(env, exports, "execPath", exec_path_val);
  }
  {
    js_value_t *val;
    js_value_t *str;

    js_create_array_with_length(env, runtime->argc, &val);

    int idx = 0;

    js_set_element(env, val, idx++, exec_path_val);

    for (int i = 1; i < runtime->argc; i++) {
      js_create_string_utf8(env, runtime->argv[i], -1, &str);
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
    js_create_function(env, "setTitle", -1, pear_runtime_set_title, NULL, &val);
    js_set_named_property(env, exports, "setTitle", val);
  }
  {
    js_value_t *val;
    js_create_function(env, "getTitle", -1, pear_runtime_get_title, NULL, &val);
    js_set_named_property(env, exports, "getTitle", val);
  }
  {
    js_value_t *val;
    js_create_function(env, "cwd", -1, pear_runtime_cwd, NULL, &val);
    js_set_named_property(env, exports, "cwd", val);
  }
  {
    js_value_t *val;
    js_create_function(env, "chdir", -1, pear_runtime_chdir, NULL, &val);
    js_set_named_property(env, exports, "chdir", val);
  }
  {
    js_value_t *val;
    js_create_function(env, "getEnvKeys", -1, pear_runtime_get_env_keys, (void *) runtime, &val);
    js_set_named_property(env, exports, "getEnvKeys", val);
  }
  {
    js_value_t *val;
    js_create_function(env, "getEnv", -1, pear_runtime_get_env, (void *) runtime, &val);
    js_set_named_property(env, exports, "getEnv", val);
  }
  {
    js_value_t *val;
    js_create_function(env, "hasEnv", -1, pear_runtime_has_env, (void *) runtime, &val);
    js_set_named_property(env, exports, "hasEnv", val);
  }
  {
    js_value_t *val;
    js_create_function(env, "setEnv", -1, pear_runtime_set_env, (void *) runtime, &val);
    js_set_named_property(env, exports, "setEnv", val);
  }
  {
    js_value_t *val;
    js_create_function(env, "unsetEnv", -1, pear_runtime_unset_env, (void *) runtime, &val);
    js_set_named_property(env, exports, "unsetEnv", val);
  }
  {
    js_value_t *val;
    js_create_function(env, "printInfo", -1, pear_runtime_print_info, NULL, &val);
    js_set_named_property(env, exports, "printInfo", val);
  }
  {
    js_value_t *val;
    js_create_function(env, "printError", -1, pear_runtime_print_error, NULL, &val);
    js_set_named_property(env, exports, "printError", val);
  }
  {
    js_value_t *val;
    js_create_function(env, "hrtime", -1, pear_runtime_hrtime, NULL, &val);
    js_set_named_property(env, exports, "hrtime", val);
  }
  {
    js_value_t *val;
    js_create_function(env, "loadAddon", -1, pear_runtime_load_addon, (void *) runtime, &val);
    js_set_named_property(env, exports, "loadAddon", val);
  }
  {
    js_value_t *val;
    js_create_function(env, "resolveAddon", -1, pear_runtime_resolve_addon, (void *) runtime, &val);
    js_set_named_property(env, exports, "resolveAddon", val);
  }
  {
    js_value_t *val;
    js_create_function(env, "exit", -1, pear_runtime_exit, (void *) runtime, &val);
    js_set_named_property(env, exports, "exit", val);
  }
  {
    js_value_t *val;
    js_create_function(env, "suspend", -1, pear_runtime_suspend, (void *) runtime, &val);
    js_set_named_property(env, exports, "suspend", val);
  }
  {
    js_value_t *val;
    js_create_function(env, "resume", -1, pear_runtime_resume, (void *) runtime, &val);
    js_set_named_property(env, exports, "resume", val);
  }
  {
    js_value_t *val;
    js_create_function(env, "setupThread", -1, pear_runtime_setup_thread, (void *) runtime, &val);
    js_set_named_property(env, exports, "setupThread", val);
  }
  {
    js_value_t *val;
    js_create_function(env, "joinThread", -1, pear_runtime_join_thread, (void *) runtime, &val);
    js_set_named_property(env, exports, "joinThread", val);
  }
  {
    js_value_t *val;
    js_create_function(env, "stopCurrentThread", -1, pear_runtime_stop_current_thread, (void *) runtime, &val);
    js_set_named_property(env, exports, "stopCurrentThread", val);
  }
  {
    js_value_t *val;
    js_get_boolean(env, &runtime->process->runtime == runtime, &val);
    js_set_named_property(env, exports, "isMainThread", val);
  }

  js_value_t *global;
  js_get_global(env, &global);

  js_set_named_property(env, global, "global", global);

  js_value_t *script;
  js_create_string_utf8(env, (const char *) pear_bundle, pear_bundle_len, &script);

  js_value_t *entry;
  err = js_run_script(env, "pear:pear.js", -1, 0, script, &entry);
  assert(err == 0);

  err = js_call_function(env, global, entry, 1, &exports, NULL);
  assert(err == 0);
}

static inline int
pear_runtime_run (pear_runtime_t *runtime, const char *filename, const uv_buf_t *source) {
  js_env_t *env = runtime->env;

  int err;

  js_value_t *run;
  err = js_get_named_property(env, runtime->exports, "run", &run);
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

static void
pear_runtime_on_thread (void *data) {
  pear_thread_t *thread = (pear_thread_t *) data;

  int err;

  err = js_create_env(thread->runtime.loop, thread->runtime.platform, &thread->runtime.env);
  assert(err == 0);

  pear_runtime_setup(&thread->runtime);

  js_value_t *thread_data;

  if (thread->has_data) {
    void *data;
    err = js_create_arraybuffer(thread->runtime.env, thread->data.len, &data, &thread_data);
    assert(err == 0);

    memcpy(data, thread->data.base, thread->data.len);
  } else {
    err = js_get_null(thread->runtime.env, &thread_data);
    assert(err == 0);
  }

  err = js_set_named_property(thread->runtime.env, thread->runtime.exports, "threadData", thread_data);
  assert(err == 0);

  uv_sem_post(&thread->ready);

  err = pear_runtime_run(&thread->runtime, thread->filename, thread->has_source ? &thread->source : NULL);
  assert(err == 0);

  err = uv_run(thread->runtime.loop, UV_RUN_DEFAULT);
  assert(err >= 0);

  pear_runtime_on_thread_exit(&thread->runtime);

  err = js_destroy_env(thread->runtime.env);
  assert(err == 0);

  do {
    err = uv_loop_close(thread->runtime.loop);

    if (err == UV_EBUSY) {
      uv_run(thread->runtime.loop, UV_RUN_ONCE);
    }
  } while (err == UV_EBUSY);

  uv_rwlock_wrlock(&thread->runtime.process->locks.threads);

  pear_thread_list_t *node = (pear_thread_list_t *) thread;

  if (node->previous) {
    node->previous->next = node->next;
  } else {
    thread->runtime.process->threads = node->next;
  }

  if (node->next) {
    node->next->previous = node->previous;
  }

  uv_rwlock_wrunlock(&thread->runtime.process->locks.threads);

  uv_sem_destroy(&thread->ready);

  free(thread->runtime.loop);
  free(thread);
}

#endif // PEAR_RUNTIME_H
