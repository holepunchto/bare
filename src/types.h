#ifndef PEAR_TYPES_H
#define PEAR_TYPES_H

#include <js.h>
#include <stdbool.h>
#include <stddef.h>
#include <uv.h>

#include "../include/pear.h"

struct pear_s {
  uv_loop_t *loop;
  js_platform_t *platform;
  js_env_t *env;

  uv_sem_t idle;
  bool suspended;
  bool exited;

  pear_before_exit_cb on_before_exit;
  pear_exit_cb on_exit;
  pear_suspend_cb on_suspend;
  pear_idle_cb on_idle;
  pear_resume_cb on_resume;

  struct {
    js_value_t *exports;
    int argc;
    char **argv;
  } runtime;
};

#endif // PEAR_TYPES_H
