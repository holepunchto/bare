#ifndef BARE_TYPES_H
#define BARE_TYPES_H

#include <js.h>
#include <stdbool.h>
#include <stddef.h>
#include <uv.h>

#include "../include/bare.h"

typedef struct bare_process_s bare_process_t;
typedef struct bare_runtime_s bare_runtime_t;
typedef struct bare_thread_s bare_thread_t;
typedef struct bare_thread_source_s bare_thread_source_t;
typedef struct bare_thread_data_s bare_thread_data_t;
typedef struct bare_thread_list_s bare_thread_list_t;

typedef void (*bare_thread_setup_cb)(bare_thread_t *);
typedef int (*bare_thread_run_cb)(bare_thread_t *, uv_buf_t *source);
typedef void (*bare_thread_exit_cb)(bare_thread_t *);

struct bare_runtime_s {
  uv_loop_t *loop;

  uv_async_t suspend;
  uv_async_t resume;

  bool suspended;

  bare_process_t *process;

  js_env_t *env;

  js_value_t *exports;
};

struct bare_process_s {
  js_platform_t *platform;

  int argc;
  char **argv;

  bare_before_exit_cb on_before_exit;
  bare_exit_cb on_exit;
  bare_suspend_cb on_suspend;
  bare_idle_cb on_idle;
  bare_resume_cb on_resume;

  bare_thread_list_t *threads;

  bare_runtime_t runtime;

  struct {
    uv_rwlock_t threads;
  } locks;
};

struct bare_thread_source_s {
  enum {
    bare_thread_source_none,
    bare_thread_source_buffer,
  } type;

  union {
    uv_buf_t buffer;
  };
};

struct bare_thread_data_s {
  enum {
    bare_thread_data_none,
    bare_thread_data_buffer,
    bare_thread_data_arraybuffer,
    bare_thread_data_sharedarraybuffer,
    bare_thread_data_external,
  } type;

  union {
    uv_buf_t buffer;
    js_arraybuffer_backing_store_t *backing_store;
    void *external;
  };
};

struct bare_thread_s {
  uv_thread_t id;

  uv_sem_t ready;

  char *filename;

  bare_thread_source_t source;
  bare_thread_data_t data;

  bare_thread_setup_cb on_setup;
  bare_thread_run_cb on_run;
  bare_thread_exit_cb on_exit;

  bare_runtime_t runtime;
};

struct bare_thread_list_s {
  bare_thread_t thread;

  bare_thread_list_t *previous;
  bare_thread_list_t *next;
};

#endif // BARE_TYPES_H
