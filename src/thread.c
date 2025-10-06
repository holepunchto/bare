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

static void
bare_thread__entry(void *opaque) {
  int err;

  bare_thread_t *thread = (bare_thread_t *) opaque;

  bare_runtime_t *runtime = thread->runtime;

  uv_loop_t loop;
  err = uv_loop_init(&loop);
  assert(err == 0);

  err = bare_runtime_setup(&loop, runtime->process, runtime);
  assert(err == 0);

  js_env_t *env = runtime->env;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  bare_source_t source;

  switch (thread->source.type) {
  case bare_source_none:
  default:
    source.type = bare_source_none;
    break;

  case bare_source_buffer:
    source.type = bare_source_arraybuffer;

    js_value_t *arraybuffer;

    void *data;
    err = js_create_arraybuffer(env, thread->source.buffer.len, &data, &arraybuffer);
    assert(err == 0);

    memcpy(data, thread->source.buffer.base, thread->source.buffer.len);

    err = js_create_reference(env, arraybuffer, 1, &source.arraybuffer);
    assert(err == 0);
    break;

  case bare_source_arraybuffer:
    abort();
  }

  js_value_t *data;

  switch (thread->data.type) {
  case bare_data_none:
  default:
    err = js_get_null(runtime->env, &data);
    assert(err == 0);
    break;

  case bare_data_sharedarraybuffer:
    err = js_create_sharedarraybuffer_with_backing_store(env, thread->data.backing_store, NULL, NULL, &data);
    assert(err == 0);

    err = js_release_arraybuffer_backing_store(env, thread->data.backing_store);
    assert(err == 0);
    break;
  }

  uv_sem_post(&thread->lock);

  js_value_t *exports;
  err = js_get_reference_value(env, runtime->exports, &exports);
  assert(err == 0);

  js_value_t *fn;
  err = js_get_named_property(env, exports, "onthread", &fn);
  assert(err == 0);

  js_value_t *global;
  err = js_get_global(env, &global);
  assert(err == 0);

  err = js_call_function(env, global, fn, 1, (js_value_t *[]) {data}, NULL);
  (void) err;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);

  if (runtime->process->callbacks.thread) {
    runtime->process->callbacks.thread((bare_t *) runtime->process, env, runtime->process->callbacks.thread_data);
  }

  err = bare_runtime_load(runtime, thread->filename, source, NULL);
  (void) err;

  err = bare_runtime_run(runtime, UV_RUN_DEFAULT);
  assert(err == 0);

  free(thread->filename);

  uv_sem_wait(&thread->lock);

  thread->exited = true;

  uv_sem_post(&thread->lock);

  err = bare_runtime_teardown(thread->runtime, UV_RUN_DEFAULT, NULL);
  assert(err == 0);

  err = uv_loop_close(&loop);
  assert(err == 0);
}

int
bare_thread_create(bare_runtime_t *runtime, const char *filename, bare_source_t source, bare_data_t data, size_t stack_size, bare_thread_t **result) {
  int err;

  js_env_t *env = runtime->env;

  bare_thread_t *thread = malloc(sizeof(bare_thread_t));

  thread->filename = strdup(filename);
  thread->source = source;
  thread->data = data;
  thread->exited = false;
  thread->previous = NULL;
  thread->next = NULL;

  thread->runtime = malloc(sizeof(bare_runtime_t));

  thread->runtime->process = runtime->process;

  err = uv_sem_init(&thread->lock, 0);
  assert(err == 0);

  uv_thread_options_t options = {
    .flags = UV_THREAD_HAS_STACK_SIZE,
    .stack_size = stack_size,
  };

  err = uv_thread_create_ex(&thread->id, &options, bare_thread__entry, (void *) thread);

  if (err < 0) {
    err = js_throw_error(env, uv_err_name(err), uv_strerror(err));
    assert(err == 0);

    uv_sem_destroy(&thread->lock);

    free(thread->runtime);
    free(thread);

    return -1;
  }

  thread->next = runtime->threads;

  if (thread->next) thread->next->previous = thread;

  runtime->threads = thread;

  uv_sem_wait(&thread->lock);

  uv_sem_post(&thread->lock);

  *result = thread;

  return 0;
}

int
bare_thread_join(bare_runtime_t *runtime, bare_thread_t *thread) {
  int err;

  js_env_t *env = runtime->env;

  err = uv_thread_join(&thread->id);

  uv_sem_destroy(&thread->lock);

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

  uv_sem_wait(&thread->lock);

  if (thread->exited) goto done;

  thread->runtime->linger = linger;
  thread->runtime->suspending = true;

  err = uv_async_send(&thread->runtime->signals.suspend);
  assert(err == 0);

done:
  uv_sem_post(&thread->lock);

  return 0;
}

int
bare_thread_wakeup(bare_thread_t *thread, int deadline) {
  int err;

  uv_sem_wait(&thread->lock);

  if (thread->exited) goto done;

  thread->runtime->deadline = deadline;

  err = uv_async_send(&thread->runtime->signals.wakeup);
  assert(err == 0);

done:
  uv_sem_post(&thread->lock);

  return 0;
}

int
bare_thread_resume(bare_thread_t *thread) {
  int err;

  uv_sem_wait(&thread->lock);

  if (thread->exited) goto done;

  thread->runtime->suspending = false;

  err = uv_async_send(&thread->runtime->signals.resume);
  assert(err == 0);

done:
  uv_sem_post(&thread->lock);

  return 0;
}

void
bare_thread_teardown(bare_thread_t *thread) {
  int err;

  err = uv_thread_join(&thread->id);
  assert(err == 0);

  uv_sem_destroy(&thread->lock);

  if (thread->previous) thread->previous->next = thread->next;

  if (thread->next) thread->next->previous = thread->previous;

  free(thread);
}

#endif // BARE_THREAD_H
