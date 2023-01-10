#ifndef PEARJS_RUNTIME_H
#define PEARJS_RUNTIME_H

#include <uv.h>
#include <js.h>
#include <stdbool.h>

typedef struct {
  js_value_t *exports;
  bool bootstrapped;
  int argc;
  char **argv;
} pearjs_runtime_t;

int
pearjs_runtime_setup (js_env_t *env, pearjs_runtime_t *config);

void
pearjs_runtime_teardown (js_env_t *env, pearjs_runtime_t *config);

#endif
