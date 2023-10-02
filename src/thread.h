#ifndef BARE_THREAD_H
#define BARE_THREAD_H

#include <stddef.h>
#include <uv.h>

#include "../include/bare.h"

#include "types.h"

int
bare_thread_create (bare_runtime_t *runtime, const char *filename, bare_thread_source_t source, bare_thread_data_t data, size_t stack_size, bare_thread_t **result);

int
bare_thread_join (bare_thread_t *thread);

int
bare_thread_suspend (bare_thread_t *thread);

int
bare_thread_resume (bare_thread_t *thread);

#endif // BARE_THREAD_H