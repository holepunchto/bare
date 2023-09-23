#include <assert.h>
#include <js.h>
#include <napi.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "../include/bare.h"

#include "types.h"

static bare_module_list_t *bare_addon_static = NULL;

static bare_module_list_t *bare_addon_dynamic = NULL;

static bare_module_list_t **bare_addon_pending = &bare_addon_static;

static uv_mutex_t bare_addon_lock;

static uv_once_t bare_addon_lock_guard = UV_ONCE_INIT;

static void
bare_addon_on_lock_init () {
  int err = uv_mutex_init_recursive(&bare_addon_lock);
  assert(err == 0);
}

static inline bool
bare_addon_ends_with (const char *string, const char *substring) {
  size_t s_len = strlen(string);
  size_t e_len = strlen(substring);

  return e_len <= s_len && strcmp(string + (s_len - e_len), substring) == 0;
}

bare_module_t *
bare_addon_load_static (js_env_t *env, const char *specifier) {
  uv_mutex_lock(&bare_addon_lock);

  bare_module_t *mod = NULL;
  bare_module_list_t *next = bare_addon_static;

  while (next) {
    if (bare_addon_ends_with(specifier, next->resolved)) {
      mod = &next->mod;
      break;
    }

    next = next->next;
  }

  uv_mutex_unlock(&bare_addon_lock);

  if (mod == NULL) {
    js_throw_errorf(env, NULL, "No module registered for %s", specifier);
    return NULL;
  }

  return mod;
}

bare_module_t *
bare_addon_load_dynamic (js_env_t *env, const char *specifier) {
  uv_mutex_lock(&bare_addon_lock);

  bare_module_t *mod = NULL;
  bare_module_list_t *next = bare_addon_dynamic;

  while (next) {
    if (bare_addon_ends_with(specifier, next->resolved)) {
      mod = &next->mod;
      break;
    }

    next = next->next;
  }

  if (mod) {
    next->refs++;

    uv_mutex_unlock(&bare_addon_lock);

    return mod;
  }

  int err;

  bare_addon_pending = &bare_addon_dynamic;

  uv_lib_t *lib = malloc(sizeof(uv_lib_t));

  err = uv_dlopen(specifier, lib);

  bool opened = err >= 0;

  if (err < 0) goto err;

  next = *bare_addon_pending;

  if (next && next->pending) goto done;

  bare_module_cb init;

  err = uv_dlsym(lib, BARE_STRING(BARE_MODULE_SYMBOL_REGISTER), (void **) &init);

  if (err < 0) {
    err = uv_dlsym(lib, BARE_STRING(NAPI_MODULE_SYMBOL_REGISTER), (void **) &init);

    if (err < 0) goto err;
  }

  bare_module_register(&(bare_module_t){
    .version = BARE_MODULE_VERSION,
    .filename = strdup(specifier),
    .init = init,
  });

  next = *bare_addon_pending;

done:
  mod = &next->mod;

  next->pending = false;
  next->resolved = strdup(specifier);
  next->lib = lib;

  uv_mutex_unlock(&bare_addon_lock);

  return mod;

err:
  uv_mutex_unlock(&bare_addon_lock);

  js_throw_error(env, NULL, uv_dlerror(lib));

  if (opened) uv_dlclose(lib);

  free(lib);

  return NULL;
}

bool
bare_addon_unload (js_env_t *env, bare_module_t *mod) {
  bare_module_list_t *node = (bare_module_list_t *) mod;

  if (node->lib == NULL) return false;

  uv_mutex_lock(&bare_addon_lock);

  bool unloaded = --node->refs == 0;

  if (unloaded) {
    uv_dlclose(node->lib);

    if (node->previous) {
      node->previous->next = node->next;
    } else {
      bare_addon_dynamic = node->next;
    }

    if (node->next) {
      node->next->previous = node->previous;
    }

    free(node->resolved);
    free(node);
  }

  uv_mutex_unlock(&bare_addon_lock);

  return unloaded;
}

void
bare_module_register (bare_module_t *mod) {
  uv_once(&bare_addon_lock_guard, bare_addon_on_lock_init);

  uv_mutex_lock(&bare_addon_lock);

  bool is_dynamic = bare_addon_pending == &bare_addon_dynamic;

  bare_module_list_t *next = malloc(sizeof(bare_module_list_t));

  next->next = NULL;
  next->previous = NULL;

  next->mod.version = mod->version;
  next->mod.filename = NULL;
  next->mod.init = mod->init;

  next->resolved = is_dynamic ? NULL : strdup(mod->filename);
  next->pending = is_dynamic;
  next->refs = 1;
  next->lib = NULL;

  next->next = *bare_addon_pending;

  if (next->next) {
    next->next->previous = next;
  }

  *bare_addon_pending = next;

  uv_mutex_unlock(&bare_addon_lock);
}

void
napi_module_register (napi_module *mod) {
  assert(mod->nm_version == NAPI_MODULE_VERSION);

  bare_module_register(&(bare_module_t){
    .version = BARE_MODULE_VERSION,
    .filename = mod->nm_filename,
    .init = mod->nm_register_func,
  });
}
