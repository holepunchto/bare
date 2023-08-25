#ifndef BARE_ADDON_H
#define BARE_ADDON_H

#include <assert.h>
#include <js.h>
#include <napi.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "../include/bare.h"

typedef struct bare_module_list_s bare_module_list_t;

struct bare_module_list_s {
  bare_module_t mod;
  char *resolved;
  bool pending;
  uv_lib_t *lib;
  bare_module_list_t *next;
};

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

static inline bare_module_t *
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

  if (mod->version != BARE_MODULE_VERSION) {
    js_throw_errorf(env, NULL, "Unsupported ABI version %d for module %s", mod->version, specifier);
    return NULL;
  }

  return mod;
}

static inline bare_module_t *
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
    uv_mutex_unlock(&bare_addon_lock);

    if (mod->version != BARE_MODULE_VERSION) {
      js_throw_errorf(env, NULL, "Unsupported ABI version %d for module %s", mod->version, specifier);
      return NULL;
    }

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

  err = uv_dlsym(lib, "bare_register_module_v1", (void **) &init);

  if (err < 0) {
    err = uv_dlsym(lib, "napi_register_module_v1", (void **) &init);

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

  if (mod->version != BARE_MODULE_VERSION) {
    js_throw_errorf(env, NULL, "Unsupported ABI version %d for module %s", mod->version, specifier);
    return NULL;
  }

  return mod;

err:
  uv_mutex_unlock(&bare_addon_lock);

  js_throw_error(env, NULL, uv_dlerror(lib));

  if (opened) uv_dlclose(lib);

  free(lib);

  return NULL;
}

void
bare_module_register (bare_module_t *mod) {
  uv_once(&bare_addon_lock_guard, bare_addon_on_lock_init);

  uv_mutex_lock(&bare_addon_lock);

  bool is_dynamic = bare_addon_pending == &bare_addon_dynamic;

  bare_module_list_t *next = malloc(sizeof(bare_module_list_t));

  next->mod.version = mod->version;
  next->mod.filename = mod->filename ? strdup(mod->filename) : NULL;
  next->mod.init = mod->init;

  next->resolved = is_dynamic ? NULL : strdup(mod->filename);
  next->pending = is_dynamic;
  next->next = *bare_addon_pending;

  *bare_addon_pending = next;

  uv_mutex_unlock(&bare_addon_lock);
}

void
napi_module_register (napi_module *mod) {
  bare_module_register(&(bare_module_t){
    // As Node-API modules rely on an already stable ABI that does not change
    // with the runtime ABI, we should be able to safely assume that Node-API
    // modules will always be compatible with the current module version.
    .version = BARE_MODULE_VERSION,

    .filename = mod->nm_filename,
    .init = mod->nm_register_func,
  });
}

#endif // BARE_ADDON_H
