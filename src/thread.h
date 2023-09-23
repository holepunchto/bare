#ifndef BARE_THREAD_H
#define BARE_THREAD_H

#include <stddef.h>

#include "../include/bare.h"

#include "types.h"

bare_thread_t *
bare_thread_create (bare_runtime_t *runtime, char *filename, bare_thread_source_t source, bare_thread_data_t data, size_t stack_size);

#endif // BARE_THREAD_H