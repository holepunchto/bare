#include <assert.h>
#include <js.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utf.h>
#include <uv.h>

#include "../include/bare.h"

#if BARE_ANDROID_USE_LOGCAT
#include <android/log.h>
#endif

#include "addon.h"
#include "bare.js.h"
#include "runtime.h"
#include "thread.h"
#include "types.h"

static inline bool
bare_runtime_is_main_thread (bare_runtime_t *runtime) {
  return runtime->process->runtime == runtime;
}

static void
bare_runtime_on_uncaught_exception (js_env_t *env, js_value_t *error, void *data) {
  int err;

  bare_runtime_t *runtime = (bare_runtime_t *) data;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *exports;
  err = js_get_reference_value(env, runtime->exports, &exports);
  assert(err == 0);

  js_value_t *fn;
  err = js_get_named_property(env, exports, "onuncaughtexception", &fn);
  if (err < 0) goto err;

  bool is_set;
  err = js_is_function(env, fn, &is_set);
  if (err < 0 || !is_set) goto err;

  js_value_t *global;
  err = js_get_global(env, &global);
  assert(err == 0);

  js_value_t *args[1] = {error};

  js_call_function(env, global, fn, 1, args, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);

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

  err = js_close_handle_scope(env, scope);
  assert(err == 0);

#if BARE_ANDROID_USE_LOGCAT
  err = __android_log_print(ANDROID_LOG_FATAL, "bare", "Uncaught %s", str);
  assert(err == 1);
#else
  err = fprintf(stderr, "Uncaught %s\n", str);
  assert(err >= 0);
#endif

  abort();
}
}

static void
bare_runtime_on_unhandled_rejection (js_env_t *env, js_value_t *reason, js_value_t *promise, void *data) {
  int err;

  bare_runtime_t *runtime = (bare_runtime_t *) data;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *exports;
  err = js_get_reference_value(env, runtime->exports, &exports);
  assert(err == 0);

  js_value_t *fn;
  err = js_get_named_property(env, exports, "onunhandledrejection", &fn);
  if (err < 0) goto err;

  bool is_set;
  err = js_is_function(env, fn, &is_set);
  if (err < 0 || !is_set) goto err;

  js_value_t *global;
  err = js_get_global(env, &global);
  assert(err == 0);

  js_value_t *args[2] = {reason, promise};

  js_call_function(env, global, fn, 2, args, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);

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

  err = js_close_handle_scope(env, scope);
  assert(err == 0);

#if BARE_ANDROID_USE_LOGCAT
  err = __android_log_print(ANDROID_LOG_FATAL, "bare", "Uncaught (in promise) %s", str);
  assert(err == 1);
#else
  err = fprintf(stderr, "Uncaught (in promise) %s\n", str);
  assert(err >= 0);
#endif

  abort();
}
}

static inline void
bare_runtime_on_before_exit (bare_runtime_t *runtime) {
  int err;

  js_env_t *env = runtime->env;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *exports;
  err = js_get_reference_value(env, runtime->exports, &exports);
  assert(err == 0);

  js_value_t *fn;
  err = js_get_named_property(env, exports, "onbeforeexit", &fn);
  assert(err == 0);

  js_value_t *global;
  err = js_get_global(env, &global);
  assert(err == 0);

  js_call_function(env, global, fn, 0, NULL, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);

  if (bare_runtime_is_main_thread(runtime)) {
    if (runtime->process->on_before_exit) {
      runtime->process->on_before_exit((bare_t *) runtime->process);
    }
  }
}

static inline void
bare_runtime_on_exit (bare_runtime_t *runtime) {
  int err;

  js_env_t *env = runtime->env;

  runtime->exiting = true;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *exports;
  err = js_get_reference_value(env, runtime->exports, &exports);
  assert(err == 0);

  js_value_t *fn;
  err = js_get_named_property(env, exports, "onexit", &fn);
  assert(err == 0);

  js_value_t *global;
  err = js_get_global(env, &global);
  assert(err == 0);

  js_call_function(env, global, fn, 0, NULL, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);

  if (bare_runtime_is_main_thread(runtime)) {
    if (runtime->process->on_exit) {
      runtime->process->on_exit((bare_t *) runtime->process);
    }
  }
}

static inline void
bare_runtime_on_teardown (bare_runtime_t *runtime, int *exit_code) {
  int err;

  js_env_t *env = runtime->env;

  if (exit_code) *exit_code = 0;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *exports;
  err = js_get_reference_value(env, runtime->exports, &exports);
  assert(err == 0);

  js_value_t *fn;
  err = js_get_named_property(env, exports, "onteardown", &fn);
  assert(err == 0);

  js_value_t *global;
  err = js_get_global(env, &global);
  assert(err == 0);

  js_call_function(env, global, fn, 0, NULL, NULL);

  if (exit_code) {
    js_value_t *val;
    err = js_get_named_property(env, exports, "exitCode", &val);
    assert(err == 0);

    err = js_get_value_int32(env, val, exit_code);
    assert(err == 0);
  }

  err = js_close_handle_scope(env, scope);
  assert(err == 0);

  if (bare_runtime_is_main_thread(runtime)) {
    if (runtime->process->on_teardown) {
      runtime->process->on_teardown((bare_t *) runtime->process);
    }
  }
}

static inline void
bare_runtime_on_suspend (bare_runtime_t *runtime) {
  int err;

  js_env_t *env = runtime->env;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *exports;
  err = js_get_reference_value(env, runtime->exports, &exports);
  assert(err == 0);

  js_value_t *fn;
  err = js_get_named_property(env, exports, "onsuspend", &fn);
  assert(err == 0);

  js_value_t *global;
  err = js_get_global(env, &global);
  assert(err == 0);

  js_value_t *args[1];

  err = js_create_int32(env, runtime->linger, &args[0]);
  assert(err == 0);

  js_call_function(env, global, fn, 1, args, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);

  if (bare_runtime_is_main_thread(runtime)) {
    if (runtime->process->on_suspend) {
      runtime->process->on_suspend((bare_t *) runtime->process, runtime->linger);
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

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *exports;
  err = js_get_reference_value(env, runtime->exports, &exports);
  assert(err == 0);

  js_value_t *fn;
  err = js_get_named_property(env, exports, "onidle", &fn);
  assert(err == 0);

  js_value_t *global;
  err = js_get_global(env, &global);
  assert(err == 0);

  js_call_function(env, global, fn, 0, NULL, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);

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

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *exports;
  err = js_get_reference_value(env, runtime->exports, &exports);
  assert(err == 0);

  js_value_t *fn;
  err = js_get_named_property(env, exports, "onresume", &fn);
  assert(err == 0);

  js_value_t *global;
  err = js_get_global(env, &global);
  assert(err == 0);

  js_call_function(env, global, fn, 0, NULL, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);

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

static inline void
bare_runtime_on_terminate (bare_runtime_t *runtime) {
  int err;

  js_env_t *env = runtime->env;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *exports;
  err = js_get_reference_value(env, runtime->exports, &exports);
  assert(err == 0);

  js_value_t *exit;
  err = js_get_named_property(env, exports, "exit", &exit);
  assert(err == 0);

  js_value_t *global;
  err = js_get_global(env, &global);
  assert(err == 0);

  js_call_function(env, global, exit, 0, NULL, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_runtime_on_terminate_signal (uv_async_t *handle) {
  bare_runtime_t *runtime = (bare_runtime_t *) handle->data;

  bare_runtime_on_terminate(runtime);
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

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

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

  err = js_close_handle_scope(env, scope);
  assert(err == 0);

#if BARE_ANDROID_USE_LOGCAT
  err = __android_log_print(ANDROID_LOG_INFO, "bare", "%s", data);
  assert(err == 1);
#else
  err = fprintf(stdout, "%s", data);
  assert(err >= 0);
#endif

  err = fflush(stdout);
  assert(err == 0);

  free(data);

  return NULL;
}

static js_value_t *
bare_runtime_print_error (js_env_t *env, js_callback_info_t *info) {
  int err;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

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

  err = js_close_handle_scope(env, scope);
  assert(err == 0);

#if BARE_ANDROID_USE_LOGCAT
  err = __android_log_print(ANDROID_LOG_ERROR, "bare", "%s", data);
  assert(err == 1);
#else
  err = fprintf(stderr, "%s", data);
  assert(err >= 0);
#endif

  err = fflush(stderr);
  assert(err == 0);

  free(data);

  return NULL;
}

static js_value_t *
bare_runtime_get_static_addons (js_env_t *env, js_callback_info_t *info) {
  int err;

  bare_runtime_t *runtime;

  err = js_get_callback_info(env, info, NULL, NULL, NULL, (void **) &runtime);
  assert(err == 0);

  return bare_addon_get_static(runtime);
}

static js_value_t *
bare_runtime_get_dynamic_addons (js_env_t *env, js_callback_info_t *info) {
  int err;

  bare_runtime_t *runtime;

  err = js_get_callback_info(env, info, NULL, NULL, NULL, (void **) &runtime);
  assert(err == 0);

  return bare_addon_get_dynamic(runtime);
}

static js_value_t *
bare_runtime_load_static_addon (js_env_t *env, js_callback_info_t *info) {
  int err;

  js_escapable_handle_scope_t *scope;
  err = js_open_escapable_handle_scope(env, &scope);
  assert(err == 0);

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

  if (mod == NULL) goto err;

  js_value_t *handle;
  err = js_create_external(runtime->env, mod, NULL, NULL, &handle);
  assert(err == 0);

  err = js_escape_handle(env, scope, handle, &handle);
  assert(err == 0);

  err = js_close_escapable_handle_scope(env, scope);
  assert(err == 0);

  return handle;

err:
  err = js_close_escapable_handle_scope(env, scope);
  assert(err == 0);

  return NULL;
}

static js_value_t *
bare_runtime_load_dynamic_addon (js_env_t *env, js_callback_info_t *info) {
  int err;

  js_escapable_handle_scope_t *scope;
  err = js_open_escapable_handle_scope(env, &scope);
  assert(err == 0);

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

  if (mod == NULL) goto err;

  js_value_t *handle;
  err = js_create_external(runtime->env, mod, NULL, NULL, &handle);
  assert(err == 0);

  err = js_escape_handle(env, scope, handle, &handle);
  assert(err == 0);

  err = js_close_escapable_handle_scope(env, scope);
  assert(err == 0);

  return handle;

err:
  err = js_close_escapable_handle_scope(env, scope);
  assert(err == 0);

  return NULL;
}

static js_value_t *
bare_runtime_init_addon (js_env_t *env, js_callback_info_t *info) {
  int err;

  js_escapable_handle_scope_t *scope;
  err = js_open_escapable_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *argv[2];
  size_t argc = 2;

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);

  assert(argc == 2);

  bare_module_t *mod;
  err = js_get_value_external(env, argv[0], (void **) &mod);
  assert(err == 0);

  js_value_t *exports = argv[1];

  exports = mod->init(env, exports);

  err = js_escape_handle(env, scope, exports, &exports);
  assert(err == 0);

  err = js_close_escapable_handle_scope(env, scope);
  assert(err == 0);

  return exports;
}

static js_value_t *
bare_runtime_unload_addon (js_env_t *env, js_callback_info_t *info) {
  int err;

  js_escapable_handle_scope_t *scope;
  err = js_open_escapable_handle_scope(env, &scope);
  assert(err == 0);

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

  err = js_escape_handle(env, scope, result, &result);
  assert(err == 0);

  err = js_close_escapable_handle_scope(env, scope);
  assert(err == 0);

  return result;
}

static js_value_t *
bare_runtime_terminate (js_env_t *env, js_callback_info_t *info) {
  int err;

  bare_runtime_t *runtime;

  err = js_get_callback_info(env, info, NULL, NULL, NULL, (void **) &runtime);
  assert(err == 0);

  runtime->terminated = true;

  err = js_terminate_execution(env);
  assert(err == 0);

  if (!runtime->exiting) uv_stop(runtime->loop);

  return NULL;
}

static js_value_t *
bare_runtime_abort (js_env_t *env, js_callback_info_t *info) {
  abort();
}

static js_value_t *
bare_runtime_suspend (js_env_t *env, js_callback_info_t *info) {
  int err;

  bare_runtime_t *runtime;

  js_value_t *argv[1];
  size_t argc = 1;

  err = js_get_callback_info(env, info, &argc, argv, NULL, (void **) &runtime);
  assert(err == 0);

  assert(argc == 1);

  int32_t linger;
  err = js_get_value_int32(env, argv[0], &linger);
  assert(err == 0);

  runtime->linger = linger;

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

  js_escapable_handle_scope_t *scope;
  err = js_open_escapable_handle_scope(env, &scope);
  assert(err == 0);

  bare_runtime_t *runtime;

  size_t argc = 4;
  js_value_t *argv[4];

  err = js_get_callback_info(env, info, &argc, argv, NULL, (void **) &runtime);
  assert(err == 0);

  assert(argc == 4);

  utf8_t filename[4096];
  err = js_get_value_string_utf8(env, argv[0], filename, 4096, NULL);
  assert(err == 0);

  bare_source_t source = {bare_source_none};
  bool has_source;

  err = js_is_typedarray(env, argv[1], &has_source);
  assert(err == 0);

  if (has_source) {
    source.type = bare_source_buffer;

    err = js_get_typedarray_info(env, argv[1], NULL, (void **) &source.buffer.base, (size_t *) &source.buffer.len, NULL, NULL);
    assert(err == 0);
  }

  bare_data_t data = {bare_data_none};
  bool has_data;

  err = js_is_sharedarraybuffer(env, argv[2], &has_data);
  assert(err == 0);

  if (has_data) {
    data.type = bare_data_sharedarraybuffer;

    err = js_get_sharedarraybuffer_backing_store(env, argv[2], &data.backing_store);
    assert(err == 0);
  }

  uint32_t stack_size;
  err = js_get_value_uint32(env, argv[3], &stack_size);
  assert(err == 0);

  bare_thread_t *thread;
  err = bare_thread_create(runtime, (char *) filename, source, data, stack_size, &thread);
  if (err < 0) goto err;

  js_value_t *result;
  err = js_create_external(env, (void *) thread, NULL, NULL, &result);
  assert(err == 0);

  err = js_escape_handle(env, scope, result, &result);
  assert(err == 0);

  err = js_close_escapable_handle_scope(env, scope);
  assert(err == 0);

  return result;

err:
  err = js_close_escapable_handle_scope(env, scope);
  assert(err == 0);

  return NULL;
}

static js_value_t *
bare_runtime_join_thread (js_env_t *env, js_callback_info_t *info) {
  int err;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  bare_runtime_t *runtime;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, (void **) &runtime);
  assert(err == 0);

  assert(argc == 1);

  bare_thread_t *thread;
  err = js_get_value_external(env, argv[0], (void **) &thread);
  assert(err == 0);

  bare_thread_join(runtime, thread);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);

  return NULL;
}

static js_value_t *
bare_runtime_suspend_thread (js_env_t *env, js_callback_info_t *info) {
  int err;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

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

  err = js_close_handle_scope(env, scope);
  assert(err == 0);

  return NULL;
}

static js_value_t *
bare_runtime_resume_thread (js_env_t *env, js_callback_info_t *info) {
  int err;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

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

  err = js_close_handle_scope(env, scope);
  assert(err == 0);

  return NULL;
}

int
bare_runtime_setup (uv_loop_t *loop, bare_process_t *process, bare_runtime_t *runtime) {
  int err;

  runtime->loop = loop;
  runtime->process = process;

  js_env_options_t options = {
    .version = 0,
    .memory_limit = process->options.memory_limit,
  };

  err = js_create_env(runtime->loop, runtime->process->platform, &options, &runtime->env);
  assert(err == 0);

  runtime->suspended = false;
  runtime->exiting = false;
  runtime->terminated = false;
  runtime->linger = 0;

  err = uv_async_init(runtime->loop, &runtime->signals.suspend, bare_runtime_on_suspend_signal);
  assert(err == 0);

  runtime->signals.suspend.data = (void *) runtime;

  uv_unref((uv_handle_t *) &runtime->signals.suspend);

  err = uv_async_init(runtime->loop, &runtime->signals.resume, bare_runtime_on_resume_signal);
  assert(err == 0);

  runtime->signals.resume.data = (void *) runtime;

  uv_unref((uv_handle_t *) &runtime->signals.resume);

  err = uv_async_init(runtime->loop, &runtime->signals.terminate, bare_runtime_on_terminate_signal);
  assert(err == 0);

  runtime->signals.terminate.data = (void *) runtime;

  uv_unref((uv_handle_t *) &runtime->signals.terminate);

  runtime->active_handles = 3;

  js_env_t *env = runtime->env;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *exports;
  err = js_create_object(env, &exports);
  assert(err == 0);

  err = js_create_reference(env, exports, 1, &runtime->exports);
  assert(err == 0);

  err = js_on_uncaught_exception(env, bare_runtime_on_uncaught_exception, (void *) runtime);
  assert(err == 0);

  err = js_on_unhandled_rejection(env, bare_runtime_on_unhandled_rejection, (void *) runtime);
  assert(err == 0);

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

#define V(name, str) \
  { \
    js_value_t *val; \
    err = js_create_string_utf8(env, (utf8_t *) str, -1, &val); \
    assert(err == 0); \
    err = js_set_named_property(env, exports, name, val); \
    assert(err == 0); \
  }
  V("platform", BARE_PLATFORM);
  V("arch", BARE_ARCH);
#undef V

  js_value_t *simulator;
  err = js_get_boolean(env, BARE_SIMULATOR, &simulator);
  assert(err == 0);

  err = js_set_named_property(env, exports, "simulator", simulator);
  assert(err == 0);

  js_value_t *pid;
  err = js_create_int32(env, uv_os_getpid(), &pid);
  assert(err == 0);

  err = js_set_named_property(env, exports, "pid", pid);
  assert(err == 0);

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

  V("getStaticAddons", bare_runtime_get_static_addons);
  V("getDynamicAddons", bare_runtime_get_dynamic_addons);
  V("loadStaticAddon", bare_runtime_load_static_addon);
  V("loadDynamicAddon", bare_runtime_load_dynamic_addon);
  V("initAddon", bare_runtime_init_addon);
  V("unloadAddon", bare_runtime_unload_addon);

  V("terminate", bare_runtime_terminate);
  V("abort", bare_runtime_abort);
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
  err = js_create_string_utf8(env, bare_js, bare_js_len, &script);
  assert(err == 0);

  js_value_t *entry;
  err = js_run_script(env, "bare", -1, 0, script, &entry);
  assert(err == 0);

  js_value_t *args[1] = {exports};

  err = js_call_function(env, global, entry, 1, args, NULL);
  assert(err == 0);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);

  return 0;
}

int
bare_runtime_teardown (bare_runtime_t *runtime, int *exit_code) {
  int err;

  bare_runtime_on_teardown(runtime, exit_code);

  err = js_delete_reference(runtime->env, runtime->exports);
  assert(err == 0);

  err = js_destroy_env(runtime->env);
  assert(err == 0);

  uv_ref((uv_handle_t *) &runtime->signals.suspend);

  uv_ref((uv_handle_t *) &runtime->signals.resume);

  uv_close((uv_handle_t *) &runtime->signals.suspend, bare_runtime_on_handle_close);

  uv_close((uv_handle_t *) &runtime->signals.resume, bare_runtime_on_handle_close);

  uv_close((uv_handle_t *) &runtime->signals.terminate, bare_runtime_on_handle_close);

  err = uv_run(runtime->loop, UV_RUN_DEFAULT);
  assert(err == 0);

  bare_addon_teardown();

  return 0;
}

int
bare_runtime_exit (bare_runtime_t *runtime, int exit_code) {
  int err;

  js_env_t *env = runtime->env;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *exports;
  err = js_get_reference_value(env, runtime->exports, &exports);
  assert(err == 0);

  js_value_t *exit;
  err = js_get_named_property(env, exports, "exit", &exit);
  assert(err == 0);

  js_value_t *global;
  err = js_get_global(env, &global);
  assert(err == 0);

  js_value_t *args[1];

  err = js_create_int32(env, exit_code, &args[0]);
  assert(err == 0);

  js_call_function(env, global, exit, 1, args, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);

  return 0;
}

int
bare_runtime_load (bare_runtime_t *runtime, const char *filename, bare_source_t source, js_value_t **result) {
  int err;

  js_env_t *env = runtime->env;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *exports;
  err = js_get_reference_value(env, runtime->exports, &exports);
  assert(err == 0);

  js_value_t *load;
  err = js_get_named_property(env, exports, "load", &load);
  assert(err == 0);

  js_value_t *args[2];

  err = js_create_string_utf8(env, (utf8_t *) filename, -1, &args[0]);
  if (err < 0) goto err;

  switch (source.type) {
  case bare_source_none:
    err = js_get_null(env, &args[1]);
    assert(err == 0);
    break;

  case bare_source_buffer: {
    err = js_create_external_arraybuffer(env, source.buffer.base, source.buffer.len, NULL, NULL, &args[1]);
    assert(err == 0);
    break;

  case bare_source_arraybuffer:
    err = js_get_reference_value(env, source.arraybuffer, &args[1]);
    assert(err == 0);

    err = js_delete_reference(env, source.arraybuffer);
    assert(err == 0);
    break;
  }
  }

  js_value_t *global;
  err = js_get_global(env, &global);
  assert(err == 0);

  js_call_function(env, global, load, 2, args, result);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);

  return 0;

err:
  err = js_close_handle_scope(env, scope);
  assert(err == 0);

  return -1;
}

int
bare_runtime_run (bare_runtime_t *runtime) {
  int err;

  do {
    err = uv_run(runtime->loop, UV_RUN_DEFAULT);

    // Break immediately if `uv_stop()` was called. In this case, before exit
    // hooks should NOT run.
    if (runtime->terminated) break;

    assert(err == 0);

    if (runtime->suspended) {
      bare_runtime_on_idle(runtime);

      if (uv_loop_alive(runtime->loop)) continue;

      if (!runtime->terminated) {
        uv_ref((uv_handle_t *) &runtime->signals.resume);
      }
    } else {
      bare_runtime_on_before_exit(runtime);
    }

    // Flush the pending `uv_stop()` and short circuit the loop. Any
    // outstanding I/O will be deferred until after the exit hook.
    if (runtime->terminated) {
      uv_run(runtime->loop, UV_RUN_NOWAIT);

      break;
    }
  } while (uv_loop_alive(runtime->loop));

  bare_runtime_on_exit(runtime);

  err = uv_run(runtime->loop, UV_RUN_DEFAULT);
  assert(err == 0);

  return 0;
}
