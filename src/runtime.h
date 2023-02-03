#ifndef PEAR_RUNTIME_H
#define PEAR_RUNTIME_H

#include <js.h>
#include <stdbool.h>
#include <stddef.h>
#include <uv.h>

#include "../include/pear.h"

int
pear_runtime_setup (pear_t *pear);

void
pear_runtime_before_teardown (pear_t *pear);

void
pear_runtime_teardown (pear_t *pear, int *exit_code);

int
pear_runtime_bootstrap (pear_t *pear, const char *filename, const char *source, size_t len);

void
pear_runtime_suspend (pear_t *pear);

void
pear_runtime_resume (pear_t *pear);

#endif // PEAR_RUNTIME_H
