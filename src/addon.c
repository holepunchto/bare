#include <assert.h>
#include <js.h>
#include <napi.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <utf.h>
#include <uv.h>

#if !defined(_WIN32)
#include <dlfcn.h>
#endif

#include "types.h"

static bare_module_list_t *bare_addon_static = NULL;

static bare_module_list_t *bare_addon_dynamic = NULL;

static bare_module_list_t **bare_addon_pending = &bare_addon_static;

static uv_mutex_t bare_addon_lock;

static uv_once_t bare_addon_lock_guard = UV_ONCE_INIT;

static void
bare_addon_on_lock_init (void) {
  int err = uv_mutex_init_recursive(&bare_addon_lock);
  assert(err == 0);
}

js_value_t *
bare_addon_get_static (bare_runtime_t *runtime) {
  uv_once(&bare_addon_lock_guard, bare_addon_on_lock_init);

  int err;

  uv_mutex_lock(&bare_addon_lock);

  js_value_t *result;
  err = js_create_array(runtime->env, &result);
  assert(err == 0);

  bare_module_list_t *next = bare_addon_static;

  uint32_t i = 0;

  while (next) {
    js_value_t *specifier;
    err = js_create_string_utf8(runtime->env, (utf8_t *) next->resolved, -1, &specifier);
    assert(err == 0);

    err = js_set_element(runtime->env, result, i++, specifier);
    assert(err == 0);

    next = next->next;
  }

  uv_mutex_unlock(&bare_addon_lock);

  return result;
}

js_value_t *
bare_addon_get_dynamic (bare_runtime_t *runtime) {
  uv_once(&bare_addon_lock_guard, bare_addon_on_lock_init);

  int err;

  uv_mutex_lock(&bare_addon_lock);

  js_value_t *result;
  err = js_create_array(runtime->env, &result);
  assert(err == 0);

  bare_module_list_t *next = bare_addon_dynamic;

  uint32_t i = 0;

  while (next) {
    js_value_t *specifier;
    err = js_create_string_utf8(runtime->env, (utf8_t *) next->resolved, -1, &specifier);
    assert(err == 0);

    err = js_set_element(runtime->env, result, i++, specifier);
    assert(err == 0);

    next = next->next;
  }

  uv_mutex_unlock(&bare_addon_lock);

  return result;
}

bare_module_t *
bare_addon_load_static (bare_runtime_t *runtime, const char *specifier) {
  uv_once(&bare_addon_lock_guard, bare_addon_on_lock_init);

  uv_mutex_lock(&bare_addon_lock);

  bare_module_t *mod = NULL;
  bare_module_list_t *next = bare_addon_static;

  while (next) {
    if (strcmp(specifier, next->resolved) == 0) {
      mod = &next->mod;
      break;
    }

    next = next->next;
  }

  uv_mutex_unlock(&bare_addon_lock);

  if (mod == NULL) {
    js_throw_errorf(runtime->env, NULL, "No addon registered for '%s'", specifier);

    return NULL;
  }

  return mod;
}

bare_module_t *
bare_addon_load_dynamic (bare_runtime_t *runtime, const char *specifier) {
  uv_once(&bare_addon_lock_guard, bare_addon_on_lock_init);

  int err;

  uv_mutex_lock(&bare_addon_lock);

  bare_module_t *mod = NULL;
  bare_module_list_t *next = bare_addon_dynamic;

  while (next) {
    if (strcmp(specifier, next->resolved) == 0) {
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

  bare_addon_pending = &bare_addon_dynamic;

  uv_lib_t *lib = malloc(sizeof(uv_lib_t));

#if defined(_WIN32)
  err = uv_dlopen(specifier, lib);
#else
  dlerror(); // Reset any previous error

  lib->handle = dlopen(specifier, RTLD_LAZY | RTLD_GLOBAL);

  if (lib->handle) {
    lib->errmsg = NULL;
    err = 0;
  } else {
    lib->errmsg = strdup(dlerror());
    err = -1;
  }
#endif

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

  bare_module_register(&(bare_module_t) {
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

  js_throw_error(runtime->env, NULL, uv_dlerror(lib));

  if (opened) uv_dlclose(lib);

  free(lib);

  return NULL;
}

bool
bare_addon_unload (bare_runtime_t *runtime, bare_module_t *mod) {
  uv_once(&bare_addon_lock_guard, bare_addon_on_lock_init);

  bare_module_list_t *node = (bare_module_list_t *) mod;

  if (node->lib == NULL) return false;

  uv_mutex_lock(&bare_addon_lock);

  bool unloaded = --node->refs == 0;

  uv_mutex_unlock(&bare_addon_lock);

  return unloaded;
}

void
bare_addon_teardown (void) {
  uv_once(&bare_addon_lock_guard, bare_addon_on_lock_init);

  uv_mutex_lock(&bare_addon_lock);

  bare_module_list_t *next = bare_addon_dynamic;

  while (next) {
    bare_module_list_t *node = next;

    next = next->next;

    if (node->refs) continue;

    uv_dlclose(node->lib);

    if (node->previous) {
      node->previous->next = node->next;
    } else {
      bare_addon_dynamic = node->next;
    }

    if (node->next) {
      node->next->previous = node->previous;
    }

    free(node->lib);
    free(node->resolved);
    free(node);
  }

  uv_mutex_unlock(&bare_addon_lock);
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

  bare_module_register(&(bare_module_t) {
    .version = BARE_MODULE_VERSION,
    .filename = mod->nm_filename,
    .init = mod->nm_register_func,
  });
}
