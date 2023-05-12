#ifndef BARE_TYPES_H
#define BARE_TYPES_H

#include <js.h>
#include <stdbool.h>
#include <stddef.h>
#include <uv.h>

#include "../include/bare.h"

typedef struct bare_runtime_s bare_runtime_t;
typedef struct bare_thread_s bare_thread_t;
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
  uv_sem_t idle;
  bool suspended;
  bool exited;

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

struct bare_thread_s {
  uv_thread_t id;

  uv_sem_t ready;

  char *filename;

  uv_buf_t source;
  bool has_source;

  uv_buf_t data;
  bool has_data;

  bare_runtime_t runtime;
};

struct bare_thread_list_s {
  bare_thread_t thread;

  bare_thread_list_t *previous;
  bare_thread_list_t *next;
};

#endif // BARE_TYPES_H
