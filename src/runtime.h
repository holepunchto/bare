#ifndef PEAR_RUNTIME_H
#define PEAR_RUNTIME_H

#include <js.h>
#include <stdbool.h>
#include <uv.h>

#include "../include/pear.h"

typedef struct {
  js_value_t *exports;
  bool bootstrapped;
  char *main;
  int argc;
  char **argv;
} pear_runtime_t;

int
pear_runtime_setup (pear_t *pear, pear_runtime_t *config);

void
pear_runtime_before_teardown (pear_t *pear, pear_runtime_t *config);

void
pear_runtime_teardown (pear_t *pear, pear_runtime_t *config, int *exit_code);

#endif
