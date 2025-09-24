#include <assert.h>
#include <js.h>
#include <stdbool.h>
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

#define bare_runtime__invoke_callback(runtime, callback, ...) \
  if (runtime->process->callbacks.callback) { \
    runtime->process->callbacks.callback( \
      (bare_t *) runtime->process, \
      ##__VA_ARGS__, \
      runtime->process->callbacks.callback##_data \
    ); \
  }

#define bare_runtime__invoke_callback_if_main_thread(runtime, callback, ...) \
  if (bare_runtime__is_main_thread(runtime)) { \
    bare_runtime__invoke_callback(runtime, callback, ##__VA_ARGS__); \
  }

static inline bool
bare_runtime__is_main_thread(bare_runtime_t *runtime) {
  return runtime->process->runtime == runtime;
}

static void
bare_runtime__on_uncaught_exception(js_env_t *env, js_value_t *error, void *data) {
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
  assert(err == 0);

  js_value_t *global;
  err = js_get_global(env, &global);
  assert(err == 0);

  js_value_t *args[1] = {error};

  err = js_call_function(env, global, fn, 1, args, NULL);
  (void) err;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_runtime__on_unhandled_rejection(js_env_t *env, js_value_t *reason, js_value_t *promise, void *data) {
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
  assert(err == 0);

  js_value_t *global;
  err = js_get_global(env, &global);
  assert(err == 0);

  js_value_t *args[2] = {reason, promise};

  err = js_call_function(env, global, fn, 2, args, NULL);
  (void) err;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static inline void
bare_runtime__on_before_exit(bare_runtime_t *runtime) {
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

  err = js_call_function(env, global, fn, 0, NULL, NULL);
  (void) err;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);

  bare_runtime__invoke_callback_if_main_thread(runtime, before_exit);
}

static inline void
bare_runtime__on_exit(bare_runtime_t *runtime) {
  int err;

  runtime->state = bare_runtime_state_exiting;

  js_env_t *env = runtime->env;

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

  err = js_call_function(env, global, fn, 0, NULL, NULL);
  (void) err;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);

  bare_runtime__invoke_callback_if_main_thread(runtime, exit);
}

static inline void
bare_runtime__on_teardown(bare_runtime_t *runtime, int *exit_code) {
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

  err = js_call_function(env, global, fn, 0, NULL, NULL);
  (void) err;

  if (exit_code) {
    js_value_t *val;
    err = js_get_named_property(env, exports, "exitCode", &val);
    assert(err == 0);

    err = js_get_value_int32(env, val, exit_code);
    assert(err == 0);
  }

  err = js_close_handle_scope(env, scope);
  assert(err == 0);

  bare_runtime__invoke_callback_if_main_thread(runtime, teardown);
}

static inline void
bare_runtime__on_suspend(bare_runtime_t *runtime) {
  int err;

  if (runtime->state != bare_runtime_state_active) return;

  runtime->state = bare_runtime_state_suspending;

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

  int linger = runtime->linger;

  js_value_t *args[1];

  err = js_create_int32(env, linger, &args[0]);
  assert(err == 0);

  err = js_call_function(env, global, fn, 1, args, NULL);
  (void) err;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);

  bare_thread_t *threads = runtime->threads;

  while (threads) {
    bare_thread_t *thread = threads;

    threads = thread->next;

    err = bare_thread_suspend(thread, linger);
    assert(err == 0);
  }

  bare_runtime__invoke_callback_if_main_thread(runtime, suspend, runtime->linger);
}

static void
bare_runtime__on_suspend_signal(uv_async_t *handle) {
  bare_runtime_t *runtime = (bare_runtime_t *) handle->data;

  uv_unref((uv_handle_t *) handle);

  bare_runtime__on_suspend(runtime);
}

static inline void
bare_runtime__on_wakeup(bare_runtime_t *runtime) {
  int err;

  if (
    runtime->state != bare_runtime_state_suspending &&
    runtime->state != bare_runtime_state_suspended
  ) return;

  runtime->state = bare_runtime_state_awake;

  js_env_t *env = runtime->env;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *exports;
  err = js_get_reference_value(env, runtime->exports, &exports);
  assert(err == 0);

  js_value_t *fn;
  err = js_get_named_property(env, exports, "onwakeup", &fn);
  assert(err == 0);

  js_value_t *global;
  err = js_get_global(env, &global);
  assert(err == 0);

  int deadline = runtime->deadline;

  js_value_t *args[1];

  err = js_create_int32(env, deadline, &args[0]);
  assert(err == 0);

  err = js_call_function(env, global, fn, 1, args, NULL);
  (void) err;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);

  bare_thread_t *threads = runtime->threads;

  while (threads) {
    bare_thread_t *thread = threads;

    threads = thread->next;

    err = bare_thread_wakeup(thread, deadline);
    assert(err == 0);
  }

  bare_runtime__invoke_callback_if_main_thread(runtime, wakeup, runtime->deadline);
}

static void
bare_runtime__on_wakeup_signal(uv_async_t *handle) {
  bare_runtime_t *runtime = (bare_runtime_t *) handle->data;

  uv_unref((uv_handle_t *) handle);

  bare_runtime__on_wakeup(runtime);
}

static inline void
bare_runtime__on_wakeup_timeout(uv_timer_t *handle) {
  bare_runtime_t *runtime = (bare_runtime_t *) handle->data;

  if (
    runtime->state != bare_runtime_state_suspending &&
    runtime->state != bare_runtime_state_awake
  ) return;

  runtime->state = bare_runtime_state_idle;

  uv_stop(runtime->loop);
}

static inline void
bare_runtime__on_idle(bare_runtime_t *runtime) {
  int err;

  if (
    runtime->state != bare_runtime_state_suspending &&
    runtime->state != bare_runtime_state_idle &&
    runtime->state != bare_runtime_state_awake
  ) return;

  runtime->state = bare_runtime_state_idle;

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

  err = js_call_function(env, global, fn, 0, NULL, NULL);
  (void) err;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);

  bare_runtime__invoke_callback_if_main_thread(runtime, idle);
}

static inline void
bare_runtime__on_resume(bare_runtime_t *runtime) {
  int err;

  if (
    runtime->state != bare_runtime_state_suspending &&
    runtime->state != bare_runtime_state_idle &&
    runtime->state != bare_runtime_state_suspended &&
    runtime->state != bare_runtime_state_awake
  ) return;

  runtime->state = bare_runtime_state_active;

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

  err = js_call_function(env, global, fn, 0, NULL, NULL);
  (void) err;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);

  bare_thread_t *threads = runtime->threads;

  while (threads) {
    bare_thread_t *thread = threads;

    threads = thread->next;

    err = bare_thread_resume(thread);
    assert(err == 0);
  }

  bare_runtime__invoke_callback_if_main_thread(runtime, resume);
}

static void
bare_runtime__on_resume_signal(uv_async_t *handle) {
  bare_runtime_t *runtime = (bare_runtime_t *) handle->data;

  uv_unref((uv_handle_t *) handle);

  bare_runtime__on_resume(runtime);

  if (runtime->suspending) bare_runtime__on_suspend(runtime);
}

static inline void
bare_runtime__on_terminate(bare_runtime_t *runtime) {
  int err;

  if (runtime->state == bare_runtime_state_exiting) return;

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

  err = js_call_function(env, global, exit, 0, NULL, NULL);
  (void) err;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_runtime__on_terminate_signal(uv_async_t *handle) {
  bare_runtime_t *runtime = (bare_runtime_t *) handle->data;

  bare_runtime__on_terminate(runtime);
}

static inline void
bare_runtime__on_free(bare_runtime_t *runtime) {
  uv_mutex_destroy(&runtime->lock);

  uv_cond_destroy(&runtime->wake);

  free(runtime);
}

static void
bare_runtime__on_handle_close(uv_handle_t *handle) {
  bare_runtime_t *runtime = (bare_runtime_t *) handle->data;

  if (--runtime->active_handles == 0) {
    bare_runtime__on_free(runtime);
  }
}

static js_value_t *
bare_runtime__get_static_addons(js_env_t *env, js_callback_info_t *info) {
  int err;

  bare_runtime_t *runtime;

  err = js_get_callback_info(env, info, NULL, NULL, NULL, (void **) &runtime);
  assert(err == 0);

  return bare_addon_get_static(runtime);
}

static js_value_t *
bare_runtime__get_dynamic_addons(js_env_t *env, js_callback_info_t *info) {
  int err;

  bare_runtime_t *runtime;

  err = js_get_callback_info(env, info, NULL, NULL, NULL, (void **) &runtime);
  assert(err == 0);

  return bare_addon_get_dynamic(runtime);
}

static js_value_t *
bare_runtime__load_static_addon(js_env_t *env, js_callback_info_t *info) {
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

  bare_addon_t *node = bare_addon_load_static(runtime, (char *) specifier);

  if (node == NULL) goto err;

  js_value_t *handle;
  err = js_create_external(runtime->env, node, NULL, NULL, &handle);
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
bare_runtime__load_dynamic_addon(js_env_t *env, js_callback_info_t *info) {
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

  bare_addon_t *node = bare_addon_load_dynamic(runtime, (char *) specifier);

  if (node == NULL) goto err;

  js_value_t *handle;
  err = js_create_external(runtime->env, node, NULL, NULL, &handle);
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
bare_runtime__init_addon(js_env_t *env, js_callback_info_t *info) {
  int err;

  js_escapable_handle_scope_t *scope;
  err = js_open_escapable_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *argv[2];
  size_t argc = 2;

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);

  assert(argc == 2);

  bare_addon_t *node;
  err = js_get_value_external(env, argv[0], (void **) &node);
  assert(err == 0);

  js_value_t *exports = argv[1];

  exports = node->exports(env, exports);

  err = js_escape_handle(env, scope, exports, &exports);
  assert(err == 0);

  err = js_close_escapable_handle_scope(env, scope);
  assert(err == 0);

  return exports;
}

static js_value_t *
bare_runtime__terminate(js_env_t *env, js_callback_info_t *info) {
  int err;

  bare_runtime_t *runtime;

  err = js_get_callback_info(env, info, NULL, NULL, NULL, (void **) &runtime);
  assert(err == 0);

  err = js_terminate_execution(env);
  assert(err == 0);

  if (runtime->state == bare_runtime_state_exiting) return NULL;

  runtime->state = bare_runtime_state_terminated;

  uv_stop(runtime->loop);

  return NULL;
}

static js_value_t *
bare_runtime__abort(js_env_t *env, js_callback_info_t *info) {
  abort();
}

static js_value_t *
bare_runtime__suspend(js_env_t *env, js_callback_info_t *info) {
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
  runtime->suspending = true;

  uv_ref((uv_handle_t *) &runtime->signals.suspend);

  err = uv_async_send(&runtime->signals.suspend);
  assert(err == 0);

  return NULL;
}

static js_value_t *
bare_runtime__wakeup(js_env_t *env, js_callback_info_t *info) {
  int err;

  bare_runtime_t *runtime;

  js_value_t *argv[1];
  size_t argc = 1;

  err = js_get_callback_info(env, info, &argc, argv, NULL, (void **) &runtime);
  assert(err == 0);

  assert(argc == 1);

  int32_t deadline;
  err = js_get_value_int32(env, argv[0], &deadline);
  assert(err == 0);

  runtime->deadline = deadline;

  uv_ref((uv_handle_t *) &runtime->signals.wakeup);

  err = uv_async_send(&runtime->signals.wakeup);
  assert(err == 0);

  uv_cond_signal(&runtime->wake);

  return NULL;
}

static js_value_t *
bare_runtime__idle(js_env_t *env, js_callback_info_t *info) {
  int err;

  bare_runtime_t *runtime;

  err = js_get_callback_info(env, info, NULL, NULL, NULL, (void **) &runtime);
  assert(err == 0);

  if (
    runtime->state != bare_runtime_state_suspending &&
    runtime->state != bare_runtime_state_awake
  ) return NULL;

  runtime->state = bare_runtime_state_idle;

  uv_stop(runtime->loop);

  return NULL;
}

static js_value_t *
bare_runtime__resume(js_env_t *env, js_callback_info_t *info) {
  int err;

  bare_runtime_t *runtime;

  err = js_get_callback_info(env, info, NULL, NULL, NULL, (void **) &runtime);
  assert(err == 0);

  runtime->suspending = false;

  uv_ref((uv_handle_t *) &runtime->signals.resume);

  err = uv_async_send(&runtime->signals.resume);
  assert(err == 0);

  uv_cond_signal(&runtime->wake);

  return NULL;
}

static js_value_t *
bare_runtime__setup_thread(js_env_t *env, js_callback_info_t *info) {
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
bare_runtime__join_thread(js_env_t *env, js_callback_info_t *info) {
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
bare_runtime__suspend_thread(js_env_t *env, js_callback_info_t *info) {
  int err;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  bare_runtime_t *runtime;

  size_t argc = 2;
  js_value_t *argv[2];

  err = js_get_callback_info(env, info, &argc, argv, NULL, (void **) &runtime);
  assert(err == 0);

  assert(argc == 2);

  bare_thread_t *thread;
  err = js_get_value_external(env, argv[0], (void **) &thread);
  assert(err == 0);

  int32_t linger;
  err = js_get_value_int32(env, argv[1], &linger);
  assert(err == 0);

  err = bare_thread_suspend(thread, linger);
  assert(err == 0);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);

  return NULL;
}

static js_value_t *
bare_runtime__wakeup_thread(js_env_t *env, js_callback_info_t *info) {
  int err;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  bare_runtime_t *runtime;

  size_t argc = 2;
  js_value_t *argv[2];

  err = js_get_callback_info(env, info, &argc, argv, NULL, (void **) &runtime);
  assert(err == 0);

  assert(argc == 2);

  bare_thread_t *thread;
  err = js_get_value_external(env, argv[0], (void **) &thread);
  assert(err == 0);

  int32_t deadline;
  err = js_get_value_int32(env, argv[1], &deadline);
  assert(err == 0);

  err = bare_thread_wakeup(thread, deadline);
  assert(err == 0);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);

  return NULL;
}

static js_value_t *
bare_runtime__resume_thread(js_env_t *env, js_callback_info_t *info) {
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

static js_value_t *
bare_runtime__require(js_env_t *env, js_callback_info_t *info) {
  int err;

  err = js_throw_error(env, NULL, "Cannot require modules from bootstrap script");
  assert(err == 0);

  return NULL;
}

static js_value_t *
bare_runtime__require_addon(js_env_t *env, js_callback_info_t *info) {
  int err;

  js_escapable_handle_scope_t *scope;
  err = js_open_escapable_handle_scope(env, &scope);
  assert(err == 0);

  bare_runtime_t *runtime;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, (void **) &runtime);
  assert(err == 0);

  assert(argc == 1);

  js_value_t *exports;
  err = js_get_reference_value(env, runtime->exports, &exports);
  assert(err == 0);

  js_value_t *addon;
  err = js_get_named_property(env, exports, "addon", &addon);
  assert(err == 0);

  js_value_t *global;
  err = js_get_global(env, &global);
  assert(err == 0);

  js_value_t *result;
  err = js_call_function(env, global, addon, 1, argv, &result);
  assert(err == 0);

  err = js_escape_handle(env, scope, result, &result);
  assert(err == 0);

  err = js_close_escapable_handle_scope(env, scope);
  assert(err == 0);

  return result;
}

int
bare_runtime_setup(uv_loop_t *loop, bare_process_t *process, bare_runtime_t *runtime) {
  int err;

  runtime->loop = loop;
  runtime->process = process;
  runtime->threads = NULL;

  js_env_options_t options = {
    .version = 0,
    .memory_limit = process->options.memory_limit,
  };

  err = js_create_env(runtime->loop, runtime->process->platform, &options, &runtime->env);
  assert(err == 0);

  runtime->state = bare_runtime_state_active;
  runtime->linger = 0;

  err = uv_mutex_init(&runtime->lock);
  assert(err == 0);

  err = uv_cond_init(&runtime->wake);
  assert(err == 0);

  runtime->active_handles = 0;

#define V(signal) \
  { \
    uv_async_t *handle = &runtime->signals.signal; \
\
    handle->data = (void *) runtime; \
\
    err = uv_async_init(runtime->loop, handle, bare_runtime__on_##signal##_signal); \
    assert(err == 0); \
\
    uv_unref((uv_handle_t *) handle); \
\
    runtime->active_handles++; \
  }

  V(suspend)
  V(wakeup)
  V(resume)
  V(terminate)
#undef V

  err = uv_timer_init(runtime->loop, &runtime->timeout);
  assert(err == 0);

  runtime->timeout.data = (void *) runtime;

  uv_unref((uv_handle_t *) &runtime->timeout);

  runtime->active_handles++;

  js_env_t *env = runtime->env;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *exports;
  err = js_create_object(env, &exports);
  assert(err == 0);

  err = js_create_reference(env, exports, 1, &runtime->exports);
  assert(err == 0);

  err = js_on_uncaught_exception(env, bare_runtime__on_uncaught_exception, (void *) runtime);
  assert(err == 0);

  err = js_on_unhandled_rejection(env, bare_runtime__on_unhandled_rejection, (void *) runtime);
  assert(err == 0);

  js_value_t *argv;
  err = js_create_array_with_length(env, (size_t) runtime->process->argc, &argv);
  assert(err == 0);

  err = js_set_named_property(env, exports, "argv", argv);
  assert(err == 0);

  for (uint32_t i = 0, n = (uint32_t) runtime->process->argc; i < n; i++) {
    js_value_t *val;
    err = js_create_string_utf8(env, (utf8_t *) runtime->process->argv[i], (size_t) -1, &val);
    assert(err == 0);

    err = js_set_element(env, argv, i, val);
    assert(err == 0);
  }

#define V(name, str) \
  { \
    js_value_t *val; \
    err = js_create_string_utf8(env, (utf8_t *) str, (size_t) -1, &val); \
    assert(err == 0); \
\
    err = js_set_named_property(env, exports, name, val); \
    assert(err == 0); \
  }

  V("platform", BARE_PLATFORM);
  V("arch", BARE_ARCH);
  V("host", BARE_HOST);
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
    err = js_create_string_utf8(env, (utf8_t *) (version), (size_t) -1, &val); \
    assert(err == 0); \
\
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
    err = js_create_function(env, name, (size_t) -1, fn, (void *) runtime, &val); \
    assert(err == 0); \
\
    err = js_set_named_property(env, exports, name, val); \
    assert(err == 0); \
  }

  V("getStaticAddons", bare_runtime__get_static_addons);
  V("getDynamicAddons", bare_runtime__get_dynamic_addons);
  V("loadStaticAddon", bare_runtime__load_static_addon);
  V("loadDynamicAddon", bare_runtime__load_dynamic_addon);
  V("initAddon", bare_runtime__init_addon);

  V("terminate", bare_runtime__terminate);
  V("abort", bare_runtime__abort);
  V("suspend", bare_runtime__suspend);
  V("wakeup", bare_runtime__wakeup);
  V("idle", bare_runtime__idle);
  V("resume", bare_runtime__resume);

  V("setupThread", bare_runtime__setup_thread);
  V("joinThread", bare_runtime__join_thread);
  V("suspendThread", bare_runtime__suspend_thread);
  V("wakeupThread", bare_runtime__wakeup_thread);
  V("resumeThread", bare_runtime__resume_thread);
#undef V

#define V(name, bool) \
  { \
    js_value_t *val; \
    err = js_get_boolean(env, bool, &val); \
    assert(err == 0); \
\
    err = js_set_named_property(env, exports, name, val); \
    assert(err == 0); \
  }

  V("isMainThread", bare_runtime__is_main_thread(runtime));
#undef V

  js_value_t *global;
  err = js_get_global(env, &global);
  assert(err == 0);

  err = js_set_named_property(env, global, "global", global);
  assert(err == 0);

  js_value_t *require;
  err = js_create_function(env, "require", (size_t) -1, bare_runtime__require, (void *) runtime, &require);
  assert(err == 0);

  js_value_t *addon;
  err = js_create_function(env, "addon", (size_t) -1, bare_runtime__require_addon, (void *) runtime, &addon);
  assert(err == 0);

  err = js_set_named_property(env, require, "addon", addon);
  assert(err == 0);

  js_value_t *source;
  err = js_create_string_utf8(env, bare_js, bare_js_len, &source);
  assert(err == 0);

  js_value_t *args[2];

  err = js_create_string_utf8(env, (utf8_t *) "bare", (size_t) -1, &args[0]);
  assert(err == 0);

  err = js_create_string_utf8(env, (utf8_t *) "require", (size_t) -1, &args[1]);
  assert(err == 0);

  js_value_t *entry;
  err = js_create_function_with_source(env, NULL, 0, "bare:/bare.js", (size_t) -1, args, 2, 0, source, &entry);
  assert(err == 0);

  args[0] = exports;
  args[1] = require;

  err = js_call_function(env, global, entry, 2, args, NULL);
  assert(err == 0);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);

  return 0;
}

int
bare_runtime_teardown(bare_runtime_t *runtime, int *exit_code) {
  int err;

  bare_runtime__on_teardown(runtime, exit_code);

  err = js_delete_reference(runtime->env, runtime->exports);
  assert(err == 0);

  err = js_destroy_env(runtime->env);
  assert(err == 0);

#define V(signal) \
  uv_close((uv_handle_t *) &runtime->signals.signal, bare_runtime__on_handle_close);

  V(suspend)
  V(wakeup)
  V(resume)
  V(terminate)
#undef V

  uv_close((uv_handle_t *) &runtime->timeout, bare_runtime__on_handle_close);

  bare_thread_t *threads = runtime->threads;

  err = uv_run(runtime->loop, UV_RUN_DEFAULT);
  assert(err == 0);

  while (threads) {
    bare_thread_t *thread = threads;

    threads = thread->next;

    bare_thread_teardown(thread);
  }

  bare_addon_teardown();

  return 0;
}

int
bare_runtime_exit(bare_runtime_t *runtime, int exit_code) {
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

  err = js_call_function(env, global, exit, 1, args, NULL);
  (void) err;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);

  return 0;
}

int
bare_runtime_load(bare_runtime_t *runtime, const char *filename, bare_source_t source, js_value_t **result) {
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

  err = js_create_string_utf8(env, (utf8_t *) filename, (size_t) -1, &args[0]);
  if (err < 0) goto err;

  switch (source.type) {
  case bare_source_none:
  default:
    err = js_get_null(env, &args[1]);
    assert(err == 0);
    break;

  case bare_source_buffer:
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

  js_value_t *global;
  err = js_get_global(env, &global);
  assert(err == 0);

  err = js_call_function(env, global, load, 2, args, result);
  (void) err;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);

  return 0;

err:
  err = js_close_handle_scope(env, scope);
  assert(err == 0);

  return -1;
}

int
bare_runtime_run(bare_runtime_t *runtime) {
  int err;

  do {
    err = uv_run(runtime->loop, UV_RUN_DEFAULT);

    if (runtime->state == bare_runtime_state_terminated) goto terminated;

    if (runtime->state == bare_runtime_state_idle) goto idle;

    assert(err == 0);

    if (runtime->state == bare_runtime_state_suspending) {
    idle:
      bare_runtime__on_idle(runtime);

      if (runtime->state == bare_runtime_state_terminated) goto terminated;

      runtime->state = bare_runtime_state_suspended;

      uv_ref((uv_handle_t *) &runtime->signals.resume);

      uv_mutex_lock(&runtime->lock);

      for (;;) {
        uv_run(runtime->loop, UV_RUN_ONCE);

        if (runtime->state == bare_runtime_state_awake) {
          uv_unref((uv_handle_t *) &runtime->signals.resume);

          err = uv_timer_start(&runtime->timeout, bare_runtime__on_wakeup_timeout, runtime->deadline, 0);
          assert(err == 0);

          uv_run(runtime->loop, UV_RUN_DEFAULT);

          err = uv_timer_stop(&runtime->timeout);
          assert(err == 0);

          if (
            runtime->state == bare_runtime_state_idle ||
            runtime->state == bare_runtime_state_awake
          ) {
            uv_mutex_unlock(&runtime->lock);

            goto idle;
          }

          uv_ref((uv_handle_t *) &runtime->signals.resume);
        }

        if (runtime->state == bare_runtime_state_suspended) {
          uv_cond_wait(&runtime->wake, &runtime->lock);
        } else {
          break;
        }
      }

      uv_mutex_unlock(&runtime->lock);

      uv_unref((uv_handle_t *) &runtime->signals.resume);
    } else {
      bare_runtime__on_before_exit(runtime);
    }

    if (runtime->state == bare_runtime_state_terminated) {
    terminated:
      uv_run(runtime->loop, UV_RUN_NOWAIT);

      break;
    }
  } while (uv_loop_alive(runtime->loop));

  bare_runtime__on_exit(runtime);

  return 0;
}
