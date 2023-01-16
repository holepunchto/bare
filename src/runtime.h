#ifndef PEAR_RUNTIME_H
#define PEAR_RUNTIME_H

#include <uv.h>
#include <js.h>
#include <stdbool.h>

typedef struct {
  js_value_t *exports;
  bool bootstrapped;
  int argc;
  char **argv;
} pear_runtime_t;

int
pear_runtime_setup (js_env_t *env, pear_runtime_t *config);

void
pear_runtime_teardown (js_env_t *env, pear_runtime_t *config);

#endif
