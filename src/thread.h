#ifndef BARE_THREAD_H
#define BARE_THREAD_H

#include <stddef.h>
#include <uv.h>

#include "../include/bare.h"

#include "types.h"

int
bare_thread_create (bare_runtime_t *runtime, char *filename, bare_thread_source_t source, bare_thread_data_t data, size_t stack_size, uv_thread_t *result);

#endif // BARE_THREAD_H