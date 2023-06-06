#ifndef BARE_TYPES_H
#define BARE_TYPES_H

#include <js.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <uv.h>

#include "../include/bare.h"

typedef struct bare_runtime_s bare_runtime_t;
typedef struct bare_thread_s bare_thread_t;
typedef struct bare_thread_source_s bare_thread_source_t;
typedef struct bare_thread_data_s bare_thread_data_t;
typedef struct bare_thread_list_s bare_thread_list_t;

struct bare_runtime_s {
  uv_loop_t *loop;

  bare_t *process;

  js_platform_t *platform;
  js_env_t *env;

  js_value_t *exports;

  int argc;
  char **argv;
};

struct bare_s {
  uv_async_t suspend;
  uv_sem_t resume;

  atomic_bool suspended;
  atomic_bool resumed;
  atomic_bool exited;

  bare_before_exit_cb on_before_exit;
  bare_exit_cb on_exit;
  bare_suspend_cb on_suspend;
  bare_idle_cb on_idle;
  bare_resume_cb on_resume;

  bare_thread_list_t *threads;

  bare_runtime_t runtime;

  struct {
    uv_rwlock_t threads;
    uv_rwlock_t env;
  } locks;
};

struct bare_thread_source_s {
  enum {
    /**
     * No source, read from file system.
     */
    bare_thread_source_none,

    /**
     * Copy of a typed array or an array buffer.
     */
    bare_thread_source_buffer,
  } type;

  union {
    uv_buf_t buffer;
  };
};

struct bare_thread_data_s {
  enum {
    /**
     * No data.
     */
    bare_thread_data_none,

    /**
     * Copy of a typed array or an array buffer.
     */
    bare_thread_data_buffer,

    /**
     * Backing store of a shared array buffer.
     */
    bare_thread_data_backing_store,
  } type;

  union {
    uv_buf_t buffer;
    js_arraybuffer_backing_store_t *backing_store;
  };
};

struct bare_thread_s {
  uv_thread_t id;

  uv_sem_t ready;

  char *filename;

  bare_thread_source_t source;
  bare_thread_data_t data;

  bare_runtime_t runtime;
};

struct bare_thread_list_s {
  bare_thread_t thread;

  bare_thread_list_t *previous;
  bare_thread_list_t *next;
};

#endif // BARE_TYPES_H
