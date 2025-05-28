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

static uv_lib_t bare_addon_lib;

static uv_once_t bare_addon_guard = UV_ONCE_INIT;

static void
bare_addon_on_init(void) {
  int err;

  err = uv_mutex_init_recursive(&bare_addon_lock);
  assert(err == 0);

#if defined(_WIN32)
  bare_addon_lib.handle = GetModuleHandleW(NULL);
#else
  bare_addon_lib.handle = dlopen(NULL, RTLD_LAZY);
#endif

  bare_addon_lib.errmsg = NULL;

  assert(bare_addon_lib.handle);
}

js_value_t *
bare_addon_get_static(bare_runtime_t *runtime) {
  uv_once(&bare_addon_guard, bare_addon_on_init);

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
bare_addon_get_dynamic(bare_runtime_t *runtime) {
  uv_once(&bare_addon_guard, bare_addon_on_init);

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
bare_addon_load_static(bare_runtime_t *runtime, const char *specifier) {
  uv_once(&bare_addon_guard, bare_addon_on_init);

  int err;

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
    err = js_throw_errorf(runtime->env, NULL, "No addon registered for '%s'", specifier);
    assert(err == 0);

    return NULL;
  }

  return mod;
}

bare_module_t *
bare_addon_load_dynamic(bare_runtime_t *runtime, const char *specifier) {
  uv_once(&bare_addon_guard, bare_addon_on_init);

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

  uv_lib_t lib;

#if defined(_WIN32)
  err = uv_dlopen(specifier, &lib);
#else
  dlerror(); // Reset any previous error

  lib.handle = dlopen(specifier, RTLD_LAZY | RTLD_LOCAL);

  if (lib.handle) {
    lib.errmsg = NULL;
    err = 0;
  } else {
    lib.errmsg = strdup(dlerror());
    err = -1;
  }
#endif

  if (err < 0) goto err;

  if (bare_addon_pending == NULL) goto done; // Addon registered itself

  bare_module_name_cb name;

  err = uv_dlsym(&lib, BARE_STRING(BARE_MODULE_SYMBOL_NAME), (void **) &name);

  if (err < 0) name = NULL;

  bare_module_register_cb exports;

  err = uv_dlsym(&lib, BARE_STRING(BARE_MODULE_SYMBOL_REGISTER), (void **) &exports);

  if (err < 0) {
    err = uv_dlsym(&lib, BARE_STRING(NAPI_MODULE_SYMBOL_REGISTER), (void **) &exports);

    if (err < 0) goto err;
  }

  bare_module_register(&(bare_module_t) {
    .version = BARE_MODULE_VERSION,
    .name = name == NULL ? NULL : name(),
    .exports = exports,
  });

  next = bare_addon_dynamic;

done:
  mod = &next->mod;

  next->resolved = strdup(specifier);
  next->lib = lib;

  uv_mutex_unlock(&bare_addon_lock);

  return mod;

err:
  uv_mutex_unlock(&bare_addon_lock);

  uv_dlclose(&lib);

  err = js_throw_error(runtime->env, NULL, uv_dlerror(&lib));
  assert(err == 0);

  return NULL;
}

bool
bare_addon_unload(bare_runtime_t *runtime, bare_module_t *mod) {
  uv_once(&bare_addon_guard, bare_addon_on_init);

  bare_module_list_t *node = (bare_module_list_t *) mod;

  if (node->refs == 0) return false;

  uv_mutex_lock(&bare_addon_lock);

  bool unloaded = --node->refs == 0;

  uv_mutex_unlock(&bare_addon_lock);

  return unloaded;
}

void
bare_addon_teardown(void) {
  uv_once(&bare_addon_guard, bare_addon_on_init);

  uv_mutex_lock(&bare_addon_lock);

  bare_module_list_t *next = bare_addon_dynamic;

  while (next) {
    bare_module_list_t *node = next;

    next = next->next;

    if (node->refs) continue;

    uv_dlclose(&node->lib);

    if (node->previous) node->previous->next = node->next;
    else bare_addon_dynamic = node->next;

    if (node->next) node->next->previous = node->previous;

    free(node->resolved);
    free(node);
  }

  uv_mutex_unlock(&bare_addon_lock);
}

uv_lib_t *
bare_module_find(const char *query) {
  uv_once(&bare_addon_guard, bare_addon_on_init);

  size_t len = strlen(query);

  if (len > 5 && strcmp(&query[len - 5], ".bare") == 0) len -= 5;

  uv_lib_t *result = NULL;

  uv_mutex_lock(&bare_addon_lock);

  bare_module_list_t *next = bare_addon_static;

  while (next) {
    const char *name = next->mod.name;

    if (name && strncmp(query, name, len) == 0) {
      result = &next->lib;
      break;
    }

    next = next->next;
  }

  next = bare_addon_dynamic;

  while (next) {
    const char *name = next->mod.name;

    if (name && strncmp(query, name, len) == 0) {
      result = &next->lib;
      break;
    }

    next = next->next;
  }

  uv_mutex_unlock(&bare_addon_lock);

  return result;
}

void
bare_module_register(bare_module_t *mod) {
  uv_once(&bare_addon_guard, bare_addon_on_init);

  uv_mutex_lock(&bare_addon_lock);

  bool is_dynamic = bare_addon_pending == &bare_addon_dynamic;

  bare_module_list_t *next = malloc(sizeof(bare_module_list_t));

  next->next = NULL;
  next->previous = NULL;

  next->mod.version = mod->version;
  next->mod.name = mod->name;
  next->mod.exports = mod->exports;

  if (is_dynamic) {
    next->resolved = NULL;
    next->refs = 1;
  } else {
    next->resolved = strdup(mod->name);
    next->refs = 0;
    next->lib = bare_addon_lib;
  }

  next->next = *bare_addon_pending;

  if (next->next) next->next->previous = next;

  *bare_addon_pending = next;

  if (is_dynamic) bare_addon_pending = NULL;

  uv_mutex_unlock(&bare_addon_lock);
}

void
napi_module_register(napi_module *mod) {
  assert(mod->nm_version == NAPI_MODULE_VERSION);

  bare_module_register(&(bare_module_t) {
    .version = BARE_MODULE_VERSION,
    .name = mod->nm_filename,
    .exports = mod->nm_register_func,
  });
}
