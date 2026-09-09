#ifndef BARE_CONTEXT_H
#define BARE_CONTEXT_H

#include "types.h"

void
bare_context_init(bare_process_t *process);

void
bare_context_seal(bare_process_t *process);

void
bare_context_teardown(bare_process_t *process);

int
bare_context_set(bare_t *bare, const char *key, void *value, bare_context_destroy_cb destroy);

int
bare_context_delete(bare_t *bare, const char *key);

int
bare_context_get(const char *key, void **result);

#endif // BARE_CONTEXT_H
