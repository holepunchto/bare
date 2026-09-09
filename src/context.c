#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "../include/bare.h"

#include "addon.h"
#include "context.h"
#include "types.h"

// Context is scoped to the process rather than to the operating system process,
// which several may share, and so lives on `bare_process_t` alongside the seal
// that freezes it. It's looked up roughly once per addon per thread, so the
// entries are held in a plain list rather than interned or hashed.
//
// The lock is per process, as there is no cross-process lookup to serialise,
// and non-recursive, as nothing reenters: a destructor runs once its entry has
// been unlinked and with the lock released.

static bare_context_t **
bare_context__find(bare_process_t *process, const char *key) {
  bare_context_t **entry = &process->context.entries;

  while (*entry && strcmp((*entry)->key, key) != 0) {
    entry = &(*entry)->next;
  }

  return entry;
}

static void
bare_context__destroy(bare_context_t *entry) {
  if (entry->destroy) entry->destroy(entry->key, entry->value);

  free(entry);
}

void
bare_context_init(bare_process_t *process) {
  int err;

  err = uv_mutex_init(&process->context.lock);
  assert(err == 0);

  process->context.entries = NULL;
  process->context.sealed = false;
}

void
bare_context_seal(bare_process_t *process) {
  uv_mutex_lock(&process->context.lock);

  process->context.sealed = true;

  uv_mutex_unlock(&process->context.lock);
}

void
bare_context_teardown(bare_process_t *process) {
  uv_mutex_lock(&process->context.lock);

  bare_context_t *entries = process->context.entries;

  process->context.entries = NULL;

  uv_mutex_unlock(&process->context.lock);

  while (entries) {
    bare_context_t *entry = entries;

    entries = entry->next;

    bare_context__destroy(entry);
  }

  uv_mutex_destroy(&process->context.lock);
}

int
bare_context_set(bare_t *bare, const char *key, void *value, bare_context_destroy_cb destroy) {
  if (key == NULL) return -1;

  bare_process_t *process = &bare->process;

  uv_mutex_lock(&process->context.lock);

  if (process->context.sealed || *bare_context__find(process, key)) {
    uv_mutex_unlock(&process->context.lock);

    return -1;
  }

  size_t len = strlen(key) + 1 /* NULL */;

  bare_context_t *entry = malloc(sizeof(bare_context_t) + len);

  if (entry == NULL) {
    uv_mutex_unlock(&process->context.lock);

    return -1;
  }

  entry->key = (char *) entry + sizeof(bare_context_t);

  memcpy(entry->key, key, len);

  entry->value = value;
  entry->destroy = destroy;

  entry->next = process->context.entries;

  process->context.entries = entry;

  uv_mutex_unlock(&process->context.lock);

  return 0;
}

int
bare_context_delete(bare_t *bare, const char *key) {
  if (key == NULL) return -1;

  bare_process_t *process = &bare->process;

  uv_mutex_lock(&process->context.lock);

  bare_context_t *entry = NULL;

  if (!process->context.sealed) {
    bare_context_t **previous = bare_context__find(process, key);

    entry = *previous;

    if (entry) *previous = entry->next;
  }

  uv_mutex_unlock(&process->context.lock);

  if (entry == NULL) return -1;

  bare_context__destroy(entry);

  return 0;
}

int
bare_context_get(const char *key, void **result) {
  if (key == NULL) return -1;

  // An addon has no handle on the process that loaded it, so the lookup is
  // resolved against the process whose runtime the calling thread has entered.
  bare_process_t *process = bare_addon_current();

  if (process == NULL) return -1;

  uv_mutex_lock(&process->context.lock);

  bare_context_t *entry = *bare_context__find(process, key);

  if (entry && result) *result = entry->value;

  uv_mutex_unlock(&process->context.lock);

  return entry ? 0 : -1;
}
