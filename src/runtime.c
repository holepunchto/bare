#include <assert.h>
#include <js.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <utf.h>
#include <uv.h>

#include "../include/bare.h"

#include "addon.h"
#include "bare.js.h"
#include "runtime.h"
#include "thread.h"
#include "types.h"

#ifdef BARE_PLATFORM_ANDROID
#include "runtime/android.h"
#else
#include "runtime/posix.h"
#endif

static inline bool
bare_runtime_is_main_thread (bare_runtime_t *runtime) {
  return runtime->process->runtime == runtime;
}

static void
bare_runtime_on_uncaught_exception (js_env_t *env, js_value_t *error, void *data) {
  int err;

  bare_runtime_t *runtime = (bare_runtime_t *) data;

  js_value_t *fn;
  err = js_get_named_property(env, runtime->exports, "onuncaughtexception", &fn);
  if (err < 0) goto err;

  bool is_set;
  err = js_is_function(env, fn, &is_set);
  if (err < 0 || !is_set) goto err;

  js_value_t *global;
  err = js_get_global(env, &global);
  assert(err == 0);

  js_call_function(env, global, fn, 1, (js_value_t *[]){error}, NULL);

  return;

err: {
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
  int err;

  bare_runtime_t *runtime = (bare_runtime_t *) data;

  js_value_t *fn;
  err = js_get_named_property(env, runtime->exports, "onunhandledrejection", &fn);
  if (err < 0) goto err;

  bool is_set;
  err = js_is_function(env, fn, &is_set);
  if (err < 0 || !is_set) goto err;

  js_value_t *global;
  err = js_get_global(env, &global);
  assert(err == 0);

  js_call_function(env, global, fn, 2, (js_value_t *[]){reason, promise}, NULL);

  return;

err: {
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
  int err;

  js_env_t *env = runtime->env;

  js_value_t *fn;
  err = js_get_named_property(env, runtime->exports, "onbeforeexit", &fn);
  assert(err == 0);

  bool is_set;
  err = js_is_function(env, fn, &is_set);
  assert(err == 0);

  if (is_set) {
    js_value_t *global;
    err = js_get_global(env, &global);
    assert(err == 0);

    js_call_function(env, global, fn, 0, NULL, NULL);
  }

  if (bare_runtime_is_main_thread(runtime)) {
    if (runtime->process->on_before_exit) {
      runtime->process->on_before_exit((bare_t *) runtime->process);
    }
  }
}

static inline void
bare_runtime_on_exit (bare_runtime_t *runtime, int *exit_code) {
  int err;

  js_env_t *env = runtime->env;

  runtime->exiting = true;

  if (exit_code) *exit_code = 0;

  js_value_t *fn;
  err = js_get_named_property(env, runtime->exports, "onexit", &fn);
  assert(err == 0);

  bool is_set;
  err = js_is_function(env, fn, &is_set);
  assert(err == 0);

  if (is_set) {
    js_value_t *global;
    err = js_get_global(env, &global);
    assert(err == 0);

    js_call_function(env, global, fn, 0, NULL, NULL);
  }

  if (bare_runtime_is_main_thread(runtime)) {
    if (runtime->process->on_exit) {
      runtime->process->on_exit((bare_t *) runtime->process);
    }
  }

  if (exit_code) {
    js_value_t *val;
    err = js_get_named_property(env, runtime->exports, "exitCode", &val);
    assert(err == 0);

    err = js_get_value_int32(env, val, exit_code);
    assert(err == 0);
  }
}

static inline void
bare_runtime_on_teardown (bare_runtime_t *runtime) {
  int err;

  js_env_t *env = runtime->env;

  js_value_t *fn;
  err = js_get_named_property(env, runtime->exports, "onteardown", &fn);
  assert(err == 0);

  bool is_set;
  err = js_is_function(env, fn, &is_set);
  assert(err == 0);

  if (is_set) {
    js_value_t *global;
    err = js_get_global(env, &global);
    assert(err == 0);

    js_call_function(env, global, fn, 0, NULL, NULL);
  }
}

static inline void
bare_runtime_on_suspend (bare_runtime_t *runtime) {
  int err;

  js_env_t *env = runtime->env;

  js_value_t *fn;
  err = js_get_named_property(env, runtime->exports, "onsuspend", &fn);
  assert(err == 0);

  bool is_set;
  err = js_is_function(env, fn, &is_set);
  assert(err == 0);

  if (is_set) {
    js_value_t *global;
    err = js_get_global(env, &global);
    assert(err == 0);

    js_call_function(env, global, fn, 0, NULL, NULL);
  }

  if (bare_runtime_is_main_thread(runtime)) {
    if (runtime->process->on_suspend) {
      runtime->process->on_suspend((bare_t *) runtime->process);
    }
  }
}

static void
bare_runtime_on_suspend_signal (uv_async_t *handle) {
  bare_runtime_t *runtime = (bare_runtime_t *) handle->data;

  runtime->suspended = true;

  uv_unref((uv_handle_t *) &runtime->signals.suspend);

  bare_runtime_on_suspend(runtime);
}

static inline void
bare_runtime_on_idle (bare_runtime_t *runtime) {
  int err;

  js_env_t *env = runtime->env;

  js_value_t *fn;
  err = js_get_named_property(env, runtime->exports, "onidle", &fn);
  assert(err == 0);

  bool is_set;
  err = js_is_function(env, fn, &is_set);
  assert(err == 0);

  if (is_set) {
    js_value_t *global;
    err = js_get_global(env, &global);
    assert(err == 0);

    js_call_function(env, global, fn, 0, NULL, NULL);
  }

  if (bare_runtime_is_main_thread(runtime)) {
    if (runtime->process->on_idle) {
      runtime->process->on_idle((bare_t *) runtime->process);
    }
  }
}

static inline void
bare_runtime_on_resume (bare_runtime_t *runtime) {
  int err;

  js_env_t *env = runtime->env;

  js_value_t *fn;
  err = js_get_named_property(env, runtime->exports, "onresume", &fn);
  assert(err == 0);

  bool is_set;
  err = js_is_function(env, fn, &is_set);
  assert(err == 0);

  if (is_set) {
    js_value_t *global;
    err = js_get_global(env, &global);
    assert(err == 0);

    js_call_function(env, global, fn, 0, NULL, NULL);
  }

  if (bare_runtime_is_main_thread(runtime)) {
    if (runtime->process->on_resume) {
      runtime->process->on_resume((bare_t *) runtime->process);
    }
  }
}

static void
bare_runtime_on_resume_signal (uv_async_t *handle) {
  bare_runtime_t *runtime = (bare_runtime_t *) handle->data;

  runtime->suspended = false;

  uv_unref((uv_handle_t *) &runtime->signals.resume);

  bare_runtime_on_resume(runtime);
}

static void
bare_runtime_on_handle_close (uv_handle_t *handle) {
  bare_runtime_t *runtime = (bare_runtime_t *) handle->data;

  if (--runtime->active_handles == 0) {
    free(runtime);
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
  int err;

  bare_runtime_t *runtime;

  js_value_t *argv[1];
  size_t argc = 1;

  err = js_get_callback_info(env, info, &argc, argv, NULL, (void **) &runtime);
  assert(err == 0);

  assert(argc == 1);

  utf8_t specifier[4096];
  err = js_get_value_string_utf8(env, argv[0], specifier, 4096, NULL);
  assert(err == 0);

  bare_module_t *mod = bare_addon_load_static(runtime, (char *) specifier);

  if (mod == NULL) return NULL;

  js_value_t *handle;
  err = js_create_external(runtime->env, mod, NULL, NULL, &handle);
  assert(err == 0);

  return handle;
}

static js_value_t *
bare_runtime_load_dynamic_addon (js_env_t *env, js_callback_info_t *info) {
  int err;

  bare_runtime_t *runtime;

  js_value_t *argv[1];
  size_t argc = 1;

  err = js_get_callback_info(env, info, &argc, argv, NULL, (void **) &runtime);
  assert(err == 0);

  assert(argc == 1);

  utf8_t specifier[4096];
  err = js_get_value_string_utf8(env, argv[0], specifier, 4096, NULL);
  assert(err == 0);

  bare_module_t *mod = bare_addon_load_dynamic(runtime, (char *) specifier);

  if (mod == NULL) return NULL;

  js_value_t *handle;
  err = js_create_external(runtime->env, mod, NULL, NULL, &handle);
  assert(err == 0);

  return handle;
}

static js_value_t *
bare_runtime_init_addon (js_env_t *env, js_callback_info_t *info) {
  int err;

  js_value_t *argv[2];
  size_t argc = 2;

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);

  assert(argc == 2);

  bare_module_t *mod;
  err = js_get_value_external(env, argv[0], (void **) &mod);
  assert(err == 0);

  js_value_t *exports = argv[1];

  return mod->init(env, exports);
}

static js_value_t *
bare_runtime_unload_addon (js_env_t *env, js_callback_info_t *info) {
  int err;

  bare_runtime_t *runtime;

  js_value_t *argv[1];
  size_t argc = 1;

  err = js_get_callback_info(env, info, &argc, argv, NULL, (void **) &runtime);
  assert(err == 0);

  assert(argc == 1);

  bare_module_t *mod;
  err = js_get_value_external(env, argv[0], (void **) &mod);
  assert(err == 0);

  bool unloaded = bare_addon_unload(runtime, mod);

  js_value_t *result;
  err = js_get_boolean(env, unloaded, &result);
  assert(err == 0);

  return result;
}

static js_value_t *
bare_runtime_readdir (js_env_t *env, js_callback_info_t *info) {
  int err;

  bare_runtime_t *runtime;

  js_value_t *argv[1];
  size_t argc = 1;

  err = js_get_callback_info(env, info, &argc, argv, NULL, (void **) &runtime);
  assert(err == 0);

  assert(argc == 1);

  utf8_t path[4096];
  err = js_get_value_string_utf8(env, argv[0], path, 4096, NULL);
  assert(err == 0);

  uv_fs_t req;

  err = uv_fs_opendir(runtime->loop, &req, (char *) path, NULL);

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
    err = uv_fs_readdir(runtime->loop, &req, dir, NULL);

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

  err = uv_fs_closedir(runtime->loop, &req, dir, NULL);

  uv_fs_req_cleanup(&req);

  if (err < 0) {
    js_throw_error(env, uv_err_name(err), uv_strerror(err));

    return NULL;
  }

  return result;
}

static js_value_t *
bare_runtime_terminate (js_env_t *env, js_callback_info_t *info) {
  int err;

  bare_runtime_t *runtime;

  err = js_get_callback_info(env, info, NULL, NULL, NULL, (void **) &runtime);
  assert(err == 0);

  err = js_terminate_execution(env);
  assert(err == 0);

  if (!runtime->exiting) uv_stop(runtime->loop);

  return NULL;
}

static js_value_t *
bare_runtime_suspend (js_env_t *env, js_callback_info_t *info) {
  int err;

  bare_runtime_t *runtime;

  err = js_get_callback_info(env, info, NULL, NULL, NULL, (void **) &runtime);
  assert(err == 0);

  uv_ref((uv_handle_t *) &runtime->signals.suspend);

  err = uv_async_send(&runtime->process->runtime->signals.suspend);
  assert(err == 0);

  return NULL;
}

static js_value_t *
bare_runtime_resume (js_env_t *env, js_callback_info_t *info) {
  int err;

  bare_runtime_t *runtime;

  err = js_get_callback_info(env, info, NULL, NULL, NULL, (void **) &runtime);
  assert(err == 0);

  err = uv_async_send(&runtime->process->runtime->signals.resume);
  assert(err == 0);

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

  utf8_t filename[4096];
  err = js_get_value_string_utf8(env, argv[0], filename, 4096, NULL);
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

  bare_thread_t *thread;
  err = bare_thread_create(runtime, (char *) filename, source, data, stack_size, &thread);
  if (err < 0) return NULL;

  js_value_t *result;
  err = js_create_external(env, (void *) thread, NULL, NULL, &result);
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

  bare_thread_t *thread;
  err = js_get_value_external(env, argv[0], (void **) &thread);
  assert(err == 0);

  bare_thread_join(thread);

  return NULL;
}

static js_value_t *
bare_runtime_suspend_thread (js_env_t *env, js_callback_info_t *info) {
  int err;

  bare_runtime_t *runtime;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, (void **) &runtime);
  assert(err == 0);

  assert(argc == 1);

  bare_thread_t *thread;
  err = js_get_value_external(env, argv[0], (void **) &thread);
  assert(err == 0);

  err = bare_thread_suspend(thread);
  assert(err == 0);

  return NULL;
}

static js_value_t *
bare_runtime_resume_thread (js_env_t *env, js_callback_info_t *info) {
  int err;

  bare_runtime_t *runtime;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, (void **) &runtime);
  assert(err == 0);

  assert(argc == 1);

  bare_thread_t *thread;
  err = js_get_value_external(env, argv[0], (void **) &thread);
  assert(err == 0);

  err = bare_thread_resume(thread);
  assert(err == 0);

  return NULL;
}

int
bare_runtime_setup (uv_loop_t *loop, bare_process_t *process, bare_runtime_t *runtime) {
  int err;

  runtime->loop = loop;
  runtime->process = process;

  err = js_create_env(runtime->loop, runtime->process->platform, NULL, &runtime->env);
  assert(err == 0);

  runtime->suspended = false;
  runtime->exiting = false;

  err = uv_async_init(runtime->loop, &runtime->signals.suspend, bare_runtime_on_suspend_signal);
  assert(err == 0);

  runtime->signals.suspend.data = (void *) runtime;

  uv_unref((uv_handle_t *) &runtime->signals.suspend);

  err = uv_async_init(runtime->loop, &runtime->signals.resume, bare_runtime_on_resume_signal);
  assert(err == 0);

  runtime->signals.resume.data = (void *) runtime;

  uv_unref((uv_handle_t *) &runtime->signals.resume);

  runtime->active_handles = 2;

  js_env_t *env = runtime->env;

  err = js_create_object(env, &runtime->exports);
  assert(err == 0);

  err = js_on_uncaught_exception(env, bare_runtime_on_uncaught_exception, (void *) runtime);
  assert(err == 0);

  err = js_on_unhandled_rejection(env, bare_runtime_on_unhandled_rejection, (void *) runtime);
  assert(err == 0);

  js_value_t *exports = runtime->exports;

  js_value_t *argv;
  err = js_create_array_with_length(env, runtime->process->argc, &argv);
  assert(err == 0);

  err = js_set_named_property(env, exports, "argv", argv);
  assert(err == 0);

  for (int i = 0, n = runtime->process->argc; i < n; i++) {
    js_value_t *val;
    err = js_create_string_utf8(env, (utf8_t *) runtime->process->argv[i], -1, &val);
    assert(err == 0);

    err = js_set_element(env, argv, i, val);
    assert(err == 0);
  }

  const char *platform_identifier;
  err = js_get_platform_identifier(runtime->process->platform, &platform_identifier);
  assert(err == 0);

  const char *platform_version;
  err = js_get_platform_version(runtime->process->platform, &platform_version);
  assert(err == 0);

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
  V("uv", uv_version_string());
  V(platform_identifier, platform_version);
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

  V("printInfo", bare_runtime_print_info);
  V("printError", bare_runtime_print_error);

  V("loadStaticAddon", bare_runtime_load_static_addon);
  V("loadDynamicAddon", bare_runtime_load_dynamic_addon);
  V("initAddon", bare_runtime_init_addon);
  V("unloadAddon", bare_runtime_unload_addon);

  V("readdir", bare_runtime_readdir);

  V("terminate", bare_runtime_terminate);
  V("suspend", bare_runtime_suspend);
  V("resume", bare_runtime_resume);

  V("setupThread", bare_runtime_setup_thread);
  V("joinThread", bare_runtime_join_thread);
  V("suspendThread", bare_runtime_suspend_thread);
  V("resumeThread", bare_runtime_resume_thread);
#undef V

#define V(name, bool) \
  { \
    js_value_t *val; \
    err = js_get_boolean(env, bool, &val); \
    assert(err == 0); \
    err = js_set_named_property(env, exports, name, val); \
    assert(err == 0); \
  }

  V("isMainThread", bare_runtime_is_main_thread(runtime));
  V("isTTY", uv_guess_handle(1) == UV_TTY);
#undef V

  js_value_t *global;
  err = js_get_global(env, &global);
  assert(err == 0);

  err = js_set_named_property(env, global, "global", global);
  assert(err == 0);

  js_value_t *script;
  err = js_create_string_utf8(env, bare_bundle, bare_bundle_len, &script);
  assert(err == 0);

  js_value_t *entry;
  err = js_run_script(env, "bare:src", -1, 0, script, &entry);
  assert(err == 0);

  err = js_call_function(env, global, entry, 1, &exports, NULL);
  assert(err == 0);

  return 0;
}

int
bare_runtime_teardown (bare_runtime_t *runtime, int *exit_code) {
  int err;

  bare_runtime_on_exit(runtime, exit_code);

  err = uv_run(runtime->loop, UV_RUN_DEFAULT);
  assert(err == 0);

  bare_runtime_on_teardown(runtime);

  err = js_destroy_env(runtime->env);
  assert(err == 0);

  uv_ref((uv_handle_t *) &runtime->signals.suspend);

  uv_ref((uv_handle_t *) &runtime->signals.resume);

  uv_close((uv_handle_t *) &runtime->signals.suspend, bare_runtime_on_handle_close);

  uv_close((uv_handle_t *) &runtime->signals.resume, bare_runtime_on_handle_close);

  return 0;
}

int
bare_runtime_run (bare_runtime_t *runtime, const char *filename, bare_source_t source) {
  int err;

  js_env_t *env = runtime->env;

  js_value_t *run;
  err = js_get_named_property(env, runtime->exports, "run", &run);
  assert(err == 0);

  js_value_t *args[2];

  err = js_create_string_utf8(env, (utf8_t *) filename, -1, &args[0]);
  if (err < 0) return err;

  switch (source.type) {
  case bare_source_none:
    err = js_get_undefined(env, &args[1]);
    assert(err == 0);
    break;

  case bare_source_buffer: {
    js_value_t *arraybuffer;

    void *data;
    err = js_create_arraybuffer(env, source.buffer.len, &data, &arraybuffer);
    assert(err == 0);

    memcpy(data, source.buffer.base, source.buffer.len);

    err = js_create_typedarray(env, js_uint8_array, source.buffer.len, arraybuffer, 0, &args[1]);
    if (err < 0) return err;
    break;
  }

  case bare_source_arraybuffer: {
    size_t len;
    err = js_get_arraybuffer_info(env, source.arraybuffer, NULL, &len);
    assert(err == 0);

    err = js_create_typedarray(env, js_uint8_array, len, source.arraybuffer, 0, &args[1]);
    if (err < 0) return err;
    break;
  }
  }

  js_value_t *global;
  err = js_get_global(env, &global);
  assert(err == 0);

  err = js_call_function(env, global, run, 2, args, NULL);
  if (err < 0) return err;

  do {
    err = uv_run(runtime->loop, UV_RUN_DEFAULT);
    if (err != 0) break;

    if (runtime->suspended) {
      bare_runtime_on_idle(runtime);

      if (uv_loop_alive(runtime->loop)) continue;

      uv_ref((uv_handle_t *) &runtime->signals.resume);
    } else {
      bare_runtime_on_before_exit(runtime);
    }
  } while (uv_loop_alive(runtime->loop));

  return 0;
}
