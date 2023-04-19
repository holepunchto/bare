#ifndef PEAR_TYPES_H
#define PEAR_TYPES_H

#include <js.h>
#include <stdbool.h>
#include <stddef.h>
#include <uv.h>

#include "../include/pear.h"

typedef struct pear_runtime_s pear_runtime_t;
typedef struct pear_thread_s pear_thread_t;

struct pear_runtime_s {
  uv_loop_t *loop;

  pear_t *process;

  js_platform_t *platform;
  js_env_t *env;

  js_value_t *exports;

  int argc;
  char **argv;
};

struct pear_s {
  uv_sem_t idle;
  bool suspended;
  bool exited;

  pear_before_exit_cb on_before_exit;
  pear_exit_cb on_exit;
  pear_suspend_cb on_suspend;
  pear_idle_cb on_idle;
  pear_resume_cb on_resume;

  pear_runtime_t runtime;
};

struct pear_thread_s {
  uv_thread_t id;

  uv_sem_t ready;

  char *filename;
  uv_buf_t source;
  uv_buf_t data;

  pear_runtime_t runtime;
};

#endif // PEAR_TYPES_H
