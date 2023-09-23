#ifndef BARE_THREAD_H
#define BARE_THREAD_H

#include <assert.h>
#include <js.h>
#include <stdbool.h>
#include <stdlib.h>
#include <uv.h>

#include "../include/bare.h"

#include "runtime.h"
#include "types.h"

static void
bare_thread_entry (void *data) {
  int err;

  bare_thread_t *thread = (bare_thread_t *) data;

  bare_runtime_t *runtime = &thread->runtime;

  err = bare_runtime_setup(runtime->loop, runtime->process, runtime);
  assert(err == 0);

  uv_buf_t *thread_source;

  switch (thread->source.type) {
  case bare_thread_source_none:
    thread_source = NULL;
    break;

  case bare_thread_source_buffer:
    thread_source = &thread->source.buffer;
    break;
  }

  js_value_t *thread_data;

  switch (thread->data.type) {
  case bare_thread_data_none:
  default:
    err = js_get_null(thread->runtime.env, &thread_data);
    assert(err == 0);
    break;

  case bare_thread_data_buffer: {
    js_value_t *arraybuffer;

    void *data;
    err = js_create_arraybuffer(thread->runtime.env, thread->data.buffer.len, &data, &arraybuffer);
    assert(err == 0);

    memcpy(data, thread->data.buffer.base, thread->data.buffer.len);

    err = js_create_typedarray(thread->runtime.env, js_uint8_array, thread->data.buffer.len, arraybuffer, 0, &thread_data);
    assert(err == 0);
    break;
  }

  case bare_thread_data_arraybuffer: {
    void *data;
    err = js_create_arraybuffer(thread->runtime.env, thread->data.buffer.len, &data, &thread_data);
    assert(err == 0);

    memcpy(data, thread->data.buffer.base, thread->data.buffer.len);
    break;
  }

  case bare_thread_data_sharedarraybuffer:
    err = js_create_sharedarraybuffer_with_backing_store(thread->runtime.env, thread->data.backing_store, NULL, NULL, &thread_data);
    assert(err == 0);

    err = js_release_arraybuffer_backing_store(thread->runtime.env, thread->data.backing_store);
    assert(err == 0);
    break;

  case bare_thread_data_external:
    err = js_create_external(thread->runtime.env, thread->data.external, NULL, NULL, &thread_data);
    assert(err == 0);
    break;
  }

  err = js_set_named_property(thread->runtime.env, thread->runtime.exports, "threadData", thread_data);
  assert(err == 0);

  uv_sem_post(&thread->ready);

  err = bare_runtime_run(&thread->runtime, thread->filename, thread_source);
  assert(err == 0);

  bare_process_t *process = thread->runtime.process;

  uv_rwlock_wrlock(&process->locks.threads);

  bare_thread_list_t *node = (bare_thread_list_t *) thread;

  if (node->previous) {
    node->previous->next = node->next;
  } else {
    process->threads = node->next;
  }

  if (node->next) {
    node->next->previous = node->previous;
  }

  uv_rwlock_wrunlock(&process->locks.threads);

  uv_loop_t *loop = thread->runtime.loop;

  err = bare_runtime_teardown(&thread->runtime, NULL);
  assert(err == 0);

  do {
    err = uv_loop_close(loop);

    if (err == UV_EBUSY) {
      int err;

      err = uv_run(loop, UV_RUN_DEFAULT);
      assert(err == 0);
    }
  } while (err == UV_EBUSY);

  free(loop);
}

bare_thread_t *
bare_thread_create (bare_runtime_t *runtime, char *filename, bare_thread_source_t source, bare_thread_data_t data, size_t stack_size) {
  int err;

  js_env_t *env = runtime->env;

  uv_loop_t *loop = malloc(sizeof(uv_loop_t));

  err = uv_loop_init(loop);

  if (err < 0) {
    js_throw_error(env, uv_err_name(err), uv_strerror(err));

    free(loop);

    return NULL;
  }

  bare_thread_list_t *next = malloc(sizeof(bare_thread_list_t));

  next->previous = NULL;
  next->next = NULL;

  bare_thread_t *thread = &next->thread;

  thread->filename = filename;
  thread->source = source;
  thread->data = data;

  thread->runtime.loop = loop;
  thread->runtime.process = runtime->process;
  thread->runtime.env = NULL;

  err = uv_sem_init(&thread->ready, 0);
  assert(err == 0);

  uv_rwlock_wrlock(&runtime->process->locks.threads);

  next->next = runtime->process->threads;

  if (next->next) {
    next->next->previous = next;
  }

  runtime->process->threads = next;

  uv_thread_options_t options = {
    .flags = UV_THREAD_HAS_STACK_SIZE,
    .stack_size = stack_size,
  };

  err = uv_thread_create_ex(&thread->id, &options, bare_thread_entry, (void *) thread);

  if (err < 0) {
    runtime->process->threads = next->next;

    uv_rwlock_wrunlock(&runtime->process->locks.threads);

    js_throw_error(env, uv_err_name(err), uv_strerror(err));

    uv_sem_destroy(&thread->ready);

    err = uv_loop_close(loop);
    assert(err == 0);

    free(next);
    free(loop);

    return NULL;
  }

  uv_sem_wait(&thread->ready);

  uv_sem_destroy(&thread->ready);

  if (runtime->suspended) {
    err = uv_async_send(&thread->runtime.suspend);
    assert(err == 0);
  }

  uv_rwlock_wrunlock(&runtime->process->locks.threads);

  return thread;
}

#endif // BARE_THREAD_H