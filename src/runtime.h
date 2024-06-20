#ifndef BARE_RUNTIME_H
#define BARE_RUNTIME_H

#include <uv.h>

#include "types.h"

int
bare_runtime_setup (uv_loop_t *loop, bare_process_t *process, bare_runtime_t *runtime);

int
bare_runtime_teardown (bare_runtime_t *runtime, int *exit_code);

int
bare_runtime_load (bare_runtime_t *runtime, const char *filename, bare_source_t source);

int
bare_runtime_run (bare_runtime_t *runtime);

#endif // BARE_RUNTIME_H
