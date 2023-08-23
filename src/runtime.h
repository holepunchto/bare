#ifndef BARE_RUNTIME_H
#define BARE_RUNTIME_H

#include <assert.h>
#include <js.h>
#include <js/ffi.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <utf.h>
#include <uv.h>

#include "../include/bare.h"
#include "addons.h"
#include "bare.js.h"
#include "threads.h"
#include "types.h"

#ifdef BARE_PLATFORM_ANDROID
#include "runtime/android.h"
#else
#include "runtime/posix.h"
#endif

static inline void
bare_runtime_setup (bare_runtime_t *runtime);

static inline int
bare_runtime_run (bare_runtime_t *runtime, const char *filename, const uv_buf_t *source);

static void
bare_runtime_on_uncaught_exception (js_env_t *env, js_value_t *error, void *data) {
  bare_runtime_t *runtime = (bare_runtime_t *) data;

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

  utf8_t *str = malloc(len + 1);
  err = js_get_value_string_utf8(env, stack, str, len + 1, NULL);
  assert(err == 0);

  err = bare_runtime__print_error("Uncaught %s\n", str);
  assert(err >= 0);

  exit(1);
}
}

static void
bare_runtime_on_unhandled_rejection (js_env_t *env, js_value_t *reason, js_value_t *promise, void *data) {
  bare_runtime_t *runtime = (bare_runtime_t *) data;

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

  utf8_t *str = malloc(len + 1);
  err = js_get_value_string_utf8(env, stack, str, len + 1, NULL);
  assert(err == 0);

  err = bare_runtime__print_error("Uncaught (in promise) %s\n", str);
  assert(err >= 0);

  exit(1);
}
}

static inline void
bare_runtime_on_before_exit (bare_runtime_t *runtime) {
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
bare_runtime_on_exit (bare_runtime_t *runtime, int *exit_code) {
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
bare_runtime_on_suspend (bare_runtime_t *runtime) {
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
bare_runtime_on_idle (bare_runtime_t *runtime) {
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
bare_runtime_on_resume (bare_runtime_t *runtime) {
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

static inline void
bare_runtime_on_thread_setup (bare_thread_t *thread) {
  bare_runtime_setup(&thread->runtime);
}

static inline int
bare_runtime_on_thread_run (bare_thread_t *thread, uv_buf_t *source) {
  return bare_runtime_run(&thread->runtime, thread->filename, source);
}

static inline void
bare_runtime_on_thread_exit (bare_thread_t *thread) {
  bare_runtime_t *runtime = &thread->runtime;

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

static js_value_t *
bare_runtime_print_info (js_env_t *env, js_callback_info_t *info) {
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

  utf8_t *data = malloc(data_len);
  err = js_get_value_string_utf8(env, argv[0], data, data_len, &data_len);
  assert(err == 0);

  err = bare_runtime__print_info("%s", data);
  assert(err >= 0);

  free(data);

  return NULL;
}

static js_value_t *
bare_runtime_print_error (js_env_t *env, js_callback_info_t *info) {
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

  utf8_t *data = malloc(data_len);
  err = js_get_value_string_utf8(env, argv[0], data, data_len, &data_len);
  assert(err == 0);

  err = bare_runtime__print_error("%s", data);
  assert(err >= 0);

  free(data);

  return NULL;
}

static js_value_t *
bare_runtime_load_static_addon (js_env_t *env, js_callback_info_t *info) {
  bare_runtime_t *runtime;

  int err;

  js_value_t *argv[1];
  size_t argc = 1;

  err = js_get_callback_info(env, info, &argc, argv, NULL, (void **) &runtime);
  assert(err == 0);

  assert(argc == 1);

  utf8_t specifier[4096];
  err = js_get_value_string_utf8(env, argv[0], specifier, 4096, NULL);
  assert(err == 0);

  bare_module_t *mod = bare_addons_load_static(runtime->env, (char *) specifier);

  if (mod == NULL) return NULL;

  js_value_t *exports;
  err = js_create_object(runtime->env, &exports);
  assert(err == 0);

  return mod->init(runtime->env, exports);
}

static js_value_t *
bare_runtime_load_dynamic_addon (js_env_t *env, js_callback_info_t *info) {
  bare_runtime_t *runtime;

  int err;

  js_value_t *argv[1];
  size_t argc = 1;

  err = js_get_callback_info(env, info, &argc, argv, NULL, (void **) &runtime);
  assert(err == 0);

  assert(argc == 1);

  utf8_t specifier[4096];
  err = js_get_value_string_utf8(env, argv[0], specifier, 4096, NULL);
  assert(err == 0);

  bare_module_t *mod = bare_addons_load_dynamic(runtime->env, (char *) specifier);

  if (mod == NULL) return NULL;

  js_value_t *exports;
  err = js_create_object(runtime->env, &exports);
  assert(err == 0);

  return mod->init(runtime->env, exports);
}

static js_value_t *
bare_runtime_readdir (js_env_t *env, js_callback_info_t *info) {
  int err;

  js_value_t *argv[1];
  size_t argc = 1;

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);

  assert(argc == 1);

  utf8_t dirname[4096];
  err = js_get_value_string_utf8(env, argv[0], dirname, 4096, NULL);
  assert(err == 0);

  uv_loop_t *loop;
  js_get_env_loop(env, &loop);

  uv_fs_t req;

  err = uv_fs_opendir(loop, &req, (char *) dirname, NULL);

  uv_dir_t *dir = (uv_dir_t *) req.ptr;

  uv_fs_req_cleanup(&req);

  if (err < 0) {
    js_throw_error(env, uv_err_name(err), uv_strerror(err));
    return NULL;
  }

  uv_dirent_t entries[32];

  dir->dirents = entries;
  dir->nentries = 32;

  js_value_t *result;
  err = js_create_array(env, &result);
  assert(err == 0);

  uint32_t i = 0, len;

  do {
    err = uv_fs_readdir(loop, &req, dir, NULL);

    if (err < 0) {
      js_throw_error(env, uv_err_name(err), uv_strerror(err));
    } else {
      len = err;

      for (uint32_t j = 0; j < len; j++) {
        js_value_t *value;
        err = js_create_string_utf8(env, (utf8_t *) entries[j].name, -1, &value);
        assert(err == 0);

        err = js_set_element(env, result, i++, value);
        assert(err == 0);
      }
    }

    uv_fs_req_cleanup(&req);
  } while (err >= 0 && len);

  err = uv_fs_closedir(loop, &req, dir, NULL);

  uv_fs_req_cleanup(&req);

  if (err < 0) {
    js_throw_error(env, uv_err_name(err), uv_strerror(err));
    return NULL;
  }

  return result;
}

static js_value_t *
bare_runtime_set_title (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);

  assert(argc == 1);

  size_t data_len;
  err = js_get_value_string_utf8(env, argv[0], NULL, 0, &data_len);
  assert(err == 0);

  utf8_t *data = malloc(++data_len);
  err = js_get_value_string_utf8(env, argv[0], data, data_len, &data_len);
  assert(err == 0);

  err = uv_set_process_title((char *) data);
  assert(err == 0);

  free(data);

  return NULL;
}

static js_value_t *
bare_runtime_get_title (js_env_t *env, js_callback_info_t *info) {
  int err;

  char title[256];
  err = uv_get_process_title(title, 256);
  if (err) memcpy(title, "bare", 5);

  js_value_t *result;
  err = js_create_string_utf8(env, (utf8_t *) title, -1, &result);
  assert(err == 0);

  return result;
}

static js_value_t *
bare_runtime_exit (js_env_t *env, js_callback_info_t *info) {
  bare_runtime_t *runtime;

  int err;

  err = js_get_callback_info(env, info, NULL, NULL, NULL, (void **) &runtime);
  assert(err == 0);

  bare_exit(runtime->process, -1);

  return NULL;
}

static js_value_t *
bare_runtime_suspend (js_env_t *env, js_callback_info_t *info) {
  bare_runtime_t *runtime;

  int err;

  err = js_get_callback_info(env, info, NULL, NULL, NULL, (void **) &runtime);
  assert(err == 0);

  bare_suspend(runtime->process);

  return NULL;
}

static js_value_t *
bare_runtime_resume (js_env_t *env, js_callback_info_t *info) {
  bare_runtime_t *runtime;

  int err;

  err = js_get_callback_info(env, info, NULL, NULL, NULL, (void **) &runtime);
  assert(err == 0);

  bare_resume(runtime->process);

  return NULL;
}

static js_value_t *
bare_runtime_setup_thread (js_env_t *env, js_callback_info_t *info) {
  int err;

  bare_runtime_t *runtime;

  size_t argc = 4;
  js_value_t *argv[4];

  err = js_get_callback_info(env, info, &argc, argv, NULL, (void **) &runtime);
  assert(err == 0);

  assert(argc == 4);

  size_t len;
  err = js_get_value_string_utf8(env, argv[0], NULL, 0, &len);
  assert(err == 0);

  utf8_t *filename = malloc(len + 1);
  err = js_get_value_string_utf8(env, argv[0], filename, len + 1, NULL);
  assert(err == 0);

  bare_thread_source_t source = {bare_thread_source_none};
  bool has_source;

  err = js_is_typedarray(env, argv[1], &has_source);
  assert(err == 0);

  if (has_source) {
    err = js_get_typedarray_info(env, argv[1], NULL, (void **) &source.buffer.base, &source.buffer.len, NULL, NULL);
    assert(err == 0);

    source.type = bare_thread_source_buffer;
  }

  bare_thread_data_t data = {bare_thread_data_none};
  bool has_data;

  err = js_is_typedarray(env, argv[2], &has_data);
  assert(err == 0);

  if (has_data) {
    err = js_get_typedarray_info(env, argv[2], NULL, (void **) &data.buffer.base, &data.buffer.len, NULL, NULL);
    assert(err == 0);

    data.type = bare_thread_data_buffer;
  } else {
    err = js_is_arraybuffer(env, argv[2], &has_data);
    assert(err == 0);

    if (has_data) {
      err = js_get_arraybuffer_info(env, argv[2], (void **) &data.buffer.base, &data.buffer.len);
      assert(err == 0);

      data.type = bare_thread_data_arraybuffer;
    } else {
      err = js_is_sharedarraybuffer(env, argv[2], &has_data);
      assert(err == 0);

      if (has_data) {
        err = js_get_sharedarraybuffer_backing_store(env, argv[2], &data.backing_store);
        assert(err == 0);

        data.type = bare_thread_data_sharedarraybuffer;
      } else {
        err = js_is_external(env, argv[2], &has_data);
        assert(err == 0);

        if (has_data) {
          err = js_get_value_external(env, argv[2], &data.external);
          assert(err == 0);

          data.type = bare_thread_data_external;
        }
      }
    }
  }

  uint32_t stack_size;
  err = js_get_value_uint32(env, argv[3], &stack_size);
  assert(err == 0);

  bare_thread_t *thread = bare_threads_create(
    runtime,
    (char *) filename,
    source,
    data,
    stack_size,
    bare_runtime_on_thread_setup,
    bare_runtime_on_thread_run,
    bare_runtime_on_thread_exit
  );

  if (thread == NULL) return NULL;

  js_value_t *result;
  err = js_create_external(env, (void *) thread->id, NULL, NULL, &result);
  assert(err == 0);

  return result;
}

static js_value_t *
bare_runtime_join_thread (js_env_t *env, js_callback_info_t *info) {
  int err;

  bare_runtime_t *runtime;

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
bare_runtime_stop_current_thread (js_env_t *env, js_callback_info_t *info) {
  int err;

  uv_loop_t *loop;
  err = js_get_env_loop(env, &loop);
  assert(err == 0);

  uv_stop(loop);

  return NULL;
}

static inline void
bare_runtime_setup (bare_runtime_t *runtime) {
  int err;

  js_env_t *env = runtime->env;

  err = js_create_object(env, &runtime->exports);
  assert(err == 0);

  err = js_on_uncaught_exception(env, bare_runtime_on_uncaught_exception, (void *) runtime);
  assert(err == 0);

  err = js_on_unhandled_rejection(env, bare_runtime_on_unhandled_rejection, (void *) runtime);
  assert(err == 0);

  js_value_t *exports = runtime->exports;

  js_value_t *platform;
  err = js_create_string_utf8(env, (utf8_t *) BARE_PLATFORM, -1, &platform);
  assert(err == 0);

  err = js_set_named_property(env, exports, "platform", platform);
  assert(err == 0);

  js_value_t *arch;
  err = js_create_string_utf8(env, (utf8_t *) BARE_ARCH, -1, &arch);
  assert(err == 0);

  err = js_set_named_property(env, exports, "arch", arch);
  assert(err == 0);

  js_value_t *argv;
  err = js_create_array_with_length(env, runtime->argc, &argv);
  assert(err == 0);

  err = js_set_named_property(env, exports, "argv", argv);
  assert(err == 0);

  for (int i = 0; i < runtime->argc; i++) {
    js_value_t *val;
    err = js_create_string_utf8(env, (utf8_t *) runtime->argv[i], -1, &val);
    assert(err == 0);

    err = js_set_element(env, argv, i, val);
    assert(err == 0);
  }

  js_value_t *versions;
  err = js_create_object(env, &versions);
  assert(err == 0);

  err = js_set_named_property(env, exports, "versions", versions);
  assert(err == 0);

#define V(name, version) \
  { \
    js_value_t *val; \
    err = js_create_string_utf8(env, (utf8_t *) (version), -1, &val); \
    assert(err == 0); \
    err = js_set_named_property(env, versions, name, val); \
    assert(err == 0); \
  }
  V("bare", BARE_VERSION);
  V("modules", BARE_STRING(BARE_MODULE_VERSION));
  V("uv", uv_version_string());
  V(js_platform_identifier, js_platform_version ? js_platform_version : "unknown");
#undef V

  js_value_t *exit_code;
  err = js_create_int32(env, 0, &exit_code);
  assert(err == 0);

  err = js_set_named_property(env, exports, "exitCode", exit_code);
  assert(err == 0);

#define V(name, fn) \
  { \
    js_value_t *val; \
    err = js_create_function(env, name, -1, fn, (void *) runtime, &val); \
    assert(err == 0); \
    err = js_set_named_property(env, exports, name, val); \
    assert(err == 0); \
  }
  V("setTitle", bare_runtime_set_title);
  V("getTitle", bare_runtime_get_title);
  V("printInfo", bare_runtime_print_info);
  V("printError", bare_runtime_print_error);
  V("loadStaticAddon", bare_runtime_load_static_addon);
  V("loadDynamicAddon", bare_runtime_load_dynamic_addon);
  V("readdir", bare_runtime_readdir);
  V("exit", bare_runtime_exit);
  V("suspend", bare_runtime_suspend);
  V("resume", bare_runtime_resume);
  V("setupThread", bare_runtime_setup_thread);
  V("joinThread", bare_runtime_join_thread);
  V("stopCurrentThread", bare_runtime_stop_current_thread);
#undef V

#define V(name, bool) \
  { \
    js_value_t *val; \
    err = js_get_boolean(env, bool, &val); \
    assert(err == 0); \
    err = js_set_named_property(env, exports, name, val); \
    assert(err == 0); \
  }
  V("isMainThread", &runtime->process->runtime == runtime);
  V("isTTY", uv_guess_handle(1) == UV_TTY);
#undef V

  js_value_t *global;
  js_get_global(env, &global);

  err = js_set_named_property(env, global, "global", global);
  assert(err == 0);

  js_value_t *script;
  err = js_create_string_utf8(env, bare_bundle, bare_bundle_len, &script);
  assert(err == 0);

  js_value_t *entry;
  err = js_run_script(env, "bare:bare.js", -1, 0, script, &entry);
  assert(err == 0);

  err = js_call_function(env, global, entry, 1, &exports, NULL);
  assert(err == 0);
}

static inline int
bare_runtime_run (bare_runtime_t *runtime, const char *filename, const uv_buf_t *source) {
  js_env_t *env = runtime->env;

  int err;

  js_value_t *run;
  err = js_get_named_property(env, runtime->exports, "run", &run);
  assert(err == 0);

  js_value_t *args[2];
  err = js_create_string_utf8(env, (utf8_t *) filename, -1, &args[0]);
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

#endif // BARE_RUNTIME_H
