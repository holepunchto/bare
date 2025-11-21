#ifndef BARE_THREAD_H
#define BARE_THREAD_H

#include <assert.h>
#include <js.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "../include/bare.h"

#include "runtime.h"
#include "types.h"

#define bare_thread__invoke_callback(runtime, callback, ...) \
  if (runtime.process->callbacks.callback) { \
    runtime.process->callbacks.callback( \
      (bare_t *) runtime.process, \
      ##__VA_ARGS__, \
      runtime.process->callbacks.callback##_data \
    ); \
  }

static void
bare_thread__entry(void *opaque) {
  int err;

  bare_thread_t *thread = opaque;

  char *filename = strdup(thread->filename);
  bare_source_t source = *thread->source;
  bare_data_t data = *thread->data;

  uv_loop_t loop;
  err = uv_loop_init(&loop);
  assert(err == 0);

  bare_runtime_t runtime;
  err = bare_runtime_setup(&loop, thread->process, &runtime);
  assert(err == 0);

  thread->runtime = &runtime;

  js_env_t *env = runtime.env;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *args[1];

  switch (data.type) {
  case bare_data_none:
  default:
    err = js_get_null(runtime.env, &args[0]);
    assert(err == 0);
    break;

  case bare_data_sharedarraybuffer:
    err = js_create_sharedarraybuffer_with_backing_store(env, data.backing_store, NULL, NULL, &args[0]);
    assert(err == 0);

    err = js_release_arraybuffer_backing_store(env, data.backing_store);
    assert(err == 0);
    break;
  }

  uv_barrier_wait(thread->ready);

  js_value_t *exports;
  err = js_get_reference_value(env, runtime.exports, &exports);
  assert(err == 0);

  js_value_t *fn;
  err = js_get_named_property(env, exports, "onthread", &fn);
  assert(err == 0);

  err = js_call_function(env, exports, fn, 1, args, NULL);
  (void) err;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);

  bare_thread__invoke_callback(runtime, thread, env);

  err = bare_runtime_load(&runtime, filename, source, NULL);
  (void) err;

  free(filename);

  err = bare_runtime_run(&runtime, UV_RUN_DEFAULT);
  assert(err == 0);

  uv_mutex_lock(&thread->lock);

  thread->exited = true;

  uv_mutex_unlock(&thread->lock);

  err = bare_runtime_teardown(&runtime, UV_RUN_DEFAULT, NULL);
  assert(err == 0);

  err = uv_loop_close(&loop);
  assert(err == 0);
}

int
bare_thread_create(bare_runtime_t *runtime, const char *filename, bare_source_t source, bare_data_t data, size_t stack_size, bare_thread_t **result) {
  int err;

  js_env_t *env = runtime->env;

  bare_thread_t *thread = malloc(sizeof(bare_thread_t));

  thread->process = runtime->process;
  thread->filename = filename;
  thread->source = &source;
  thread->data = &data;
  thread->exited = false;
  thread->previous = NULL;
  thread->next = NULL;

  uv_barrier_t ready;
  err = uv_barrier_init(&ready, 2);
  assert(err == 0);

  thread->ready = &ready;

  uv_thread_options_t options = {
    .flags = UV_THREAD_HAS_STACK_SIZE,
    .stack_size = stack_size,
  };

  err = uv_thread_create_ex(&thread->id, &options, bare_thread__entry, (void *) thread);

  if (err < 0) {
    err = js_throw_error(env, uv_err_name(err), uv_strerror(err));
    assert(err == 0);

    uv_barrier_destroy(&ready);

    free(thread);

    return -1;
  }

  thread->next = runtime->threads;

  if (thread->next) thread->next->previous = thread;

  runtime->threads = thread;

  err = uv_mutex_init(&thread->lock);
  assert(err == 0);

  uv_barrier_wait(&ready);

  *result = thread;

  return 0;
}

int
bare_thread_join(bare_runtime_t *runtime, bare_thread_t *thread) {
  int err;

  js_env_t *env = runtime->env;

  err = uv_thread_join(&thread->id);

  uv_mutex_destroy(&thread->lock);

  if (thread->previous) thread->previous->next = thread->next;
  else runtime->threads = thread->next;

  if (thread->next) thread->next->previous = thread->previous;

  free(thread);

  if (err < 0) {
    err = js_throw_error(env, uv_err_name(err), uv_strerror(err));
    assert(err == 0);

    return -1;
  }

  return 0;
}

int
bare_thread_suspend(bare_thread_t *thread, int linger) {
  int err;

  uv_mutex_lock(&thread->lock);

  if (thread->exited) goto done;

  err = bare_runtime_suspend(thread->runtime, linger);
  assert(err == 0);

done:
  uv_mutex_unlock(&thread->lock);

  return 0;
}

int
bare_thread_wakeup(bare_thread_t *thread, int deadline) {
  int err;

  uv_mutex_lock(&thread->lock);

  if (thread->exited) goto done;

  err = bare_runtime_wakeup(thread->runtime, deadline);
  assert(err == 0);

done:
  uv_mutex_unlock(&thread->lock);

  return 0;
}

int
bare_thread_resume(bare_thread_t *thread) {
  int err;

  uv_mutex_lock(&thread->lock);

  if (thread->exited) goto done;

  err = bare_runtime_resume(thread->runtime);
  assert(err == 0);

done:
  uv_mutex_unlock(&thread->lock);

  return 0;
}

int
bare_thread_terminate(bare_thread_t *thread) {
  int err;

  uv_mutex_lock(&thread->lock);

  if (thread->exited) goto done;

  err = bare_runtime_terminate(thread->runtime);
  assert(err == 0);

done:
  uv_mutex_unlock(&thread->lock);

  return 0;
}

void
bare_thread_teardown(bare_thread_t *thread) {
  int err;

  err = uv_thread_join(&thread->id);
  assert(err == 0);

  uv_mutex_destroy(&thread->lock);

  if (thread->previous) thread->previous->next = thread->next;

  if (thread->next) thread->next->previous = thread->previous;

  free(thread);
}

#endif // BARE_THREAD_H
