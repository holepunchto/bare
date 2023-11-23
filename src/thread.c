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
bare_thread_entry (void *data) {
  int err;

  bare_thread_t *thread = (bare_thread_t *) data;

  bare_runtime_t *runtime = thread->runtime;

  uv_loop_t loop;
  err = uv_loop_init(&loop);
  assert(err == 0);

  err = bare_runtime_setup(&loop, runtime->process, runtime);
  assert(err == 0);

  if (runtime->process->on_thread) {
    runtime->process->on_thread((bare_t *) runtime->process, runtime->env);
  }

  bare_source_t thread_source;

  switch (thread->source.type) {
  case bare_thread_source_none:
    thread_source.type = bare_source_none;
    break;

  case bare_thread_source_buffer:
    thread_source.type = bare_source_arraybuffer;

    void *data;
    err = js_create_arraybuffer(runtime->env, thread->source.buffer.len, &data, &thread_source.arraybuffer);
    assert(err == 0);

    memcpy(data, thread->source.buffer.base, thread->source.buffer.len);
    break;
  }

  js_value_t *thread_data;

  switch (thread->data.type) {
  case bare_thread_data_none:
  default:
    err = js_get_null(runtime->env, &thread_data);
    assert(err == 0);
    break;

  case bare_thread_data_buffer: {
    js_value_t *arraybuffer;

    void *data;
    err = js_create_arraybuffer(runtime->env, thread->data.buffer.len, &data, &arraybuffer);
    assert(err == 0);

    memcpy(data, thread->data.buffer.base, thread->data.buffer.len);

    err = js_create_typedarray(runtime->env, js_uint8_array, thread->data.buffer.len, arraybuffer, 0, &thread_data);
    assert(err == 0);
    break;
  }

  case bare_thread_data_arraybuffer: {
    void *data;
    err = js_create_arraybuffer(runtime->env, thread->data.buffer.len, &data, &thread_data);
    assert(err == 0);

    memcpy(data, thread->data.buffer.base, thread->data.buffer.len);
    break;
  }

  case bare_thread_data_sharedarraybuffer:
    err = js_create_sharedarraybuffer_with_backing_store(runtime->env, thread->data.backing_store, NULL, NULL, &thread_data);
    assert(err == 0);

    err = js_release_arraybuffer_backing_store(runtime->env, thread->data.backing_store);
    assert(err == 0);
    break;

  case bare_thread_data_external:
    err = js_create_external(runtime->env, thread->data.external, NULL, NULL, &thread_data);
    assert(err == 0);
    break;
  }

  err = js_set_named_property(runtime->env, runtime->exports, "threadData", thread_data);
  assert(err == 0);

  uv_sem_post(&thread->lock);

  bare_runtime_run(runtime, thread->filename, thread_source);

  free(thread->filename);

  uv_sem_wait(&thread->lock);

  thread->exited = true;

  uv_sem_post(&thread->lock);

  err = bare_runtime_teardown(thread->runtime, NULL);
  assert(err == 0);

  err = uv_loop_close(&loop);
  assert(err == 0);
}

int
bare_thread_create (bare_runtime_t *runtime, const char *filename, bare_thread_source_t source, bare_thread_data_t data, size_t stack_size, bare_thread_t **result) {
  int err;

  js_env_t *env = runtime->env;

  bare_thread_t *thread = malloc(sizeof(bare_thread_t));

  thread->filename = strdup(filename);
  thread->source = source;
  thread->data = data;
  thread->exited = false;

  thread->runtime = malloc(sizeof(bare_runtime_t));

  thread->runtime->process = runtime->process;

  err = uv_sem_init(&thread->lock, 0);
  assert(err == 0);

  uv_thread_options_t options = {
    .flags = UV_THREAD_HAS_STACK_SIZE,
    .stack_size = stack_size,
  };

  err = uv_thread_create_ex(&thread->id, &options, bare_thread_entry, (void *) thread);

  if (err < 0) {
    js_throw_error(env, uv_err_name(err), uv_strerror(err));

    uv_sem_destroy(&thread->lock);

    free(thread->runtime);
    free(thread);

    return -1;
  }

  uv_sem_wait(&thread->lock);
  uv_sem_post(&thread->lock);

  *result = thread;

  return 0;
}

int
bare_thread_join (bare_runtime_t *runtime, bare_thread_t *thread) {
  int err;

  js_env_t *env = runtime->env;

  err = uv_thread_join(&thread->id);

  uv_sem_destroy(&thread->lock);

  free(thread);

  if (err < 0) {
    js_throw_error(env, uv_err_name(err), uv_strerror(err));

    return -1;
  }

  return 0;
}

int
bare_thread_suspend (bare_thread_t *thread) {
  int err;

  uv_sem_wait(&thread->lock);

  if (thread->exited) goto done;

  err = uv_async_send(&thread->runtime->signals.suspend);
  assert(err == 0);

done:
  uv_sem_post(&thread->lock);

  return 0;
}

int
bare_thread_resume (bare_thread_t *thread) {
  int err;

  uv_sem_wait(&thread->lock);

  if (thread->exited) goto done;

  err = uv_async_send(&thread->runtime->signals.resume);
  assert(err == 0);

done:
  uv_sem_post(&thread->lock);

  return 0;
}

#endif // BARE_THREAD_H
