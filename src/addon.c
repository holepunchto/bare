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

static bare_addon_t *bare_addon_static = NULL;

static bare_addon_t *bare_addon_dynamic = NULL;

static bare_addon_t **bare_addon_pending = &bare_addon_static;

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

  bare_addon_t *next = bare_addon_static;

  uint32_t i = 0;

  while (next) {
    bare_addon_t *addon = next;

    next = addon->next;

    js_value_t *specifier;
    err = js_create_string_utf8(runtime->env, (utf8_t *) addon->resolved, -1, &specifier);
    assert(err == 0);

    err = js_set_element(runtime->env, result, i++, specifier);
    assert(err == 0);
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

  bare_addon_t *next = bare_addon_dynamic;

  uint32_t i = 0;

  while (next) {
    bare_addon_t *addon = next;

    next = addon->next;

    js_value_t *specifier;
    err = js_create_string_utf8(runtime->env, (utf8_t *) addon->resolved, -1, &specifier);
    assert(err == 0);

    err = js_set_element(runtime->env, result, i++, specifier);
    assert(err == 0);
  }

  uv_mutex_unlock(&bare_addon_lock);

  return result;
}

bare_addon_t *
bare_addon_load_static(bare_runtime_t *runtime, const char *specifier) {
  uv_once(&bare_addon_guard, bare_addon_on_init);

  int err;

  uv_mutex_lock(&bare_addon_lock);

  bare_addon_t *found = NULL;
  bare_addon_t *next = bare_addon_static;

  while (next) {
    bare_addon_t *addon = next;

    next = addon->next;

    if (strcmp(specifier, addon->resolved) == 0) {
      found = addon;
      break;
    }
  }

  uv_mutex_unlock(&bare_addon_lock);

  if (found == NULL) {
    err = js_throw_errorf(runtime->env, NULL, "No addon registered for '%s'", specifier);
    assert(err == 0);

    return NULL;
  }

  return found;
}

bare_addon_t *
bare_addon_load_dynamic(bare_runtime_t *runtime, const char *specifier) {
  uv_once(&bare_addon_guard, bare_addon_on_init);

  int err;

  uv_mutex_lock(&bare_addon_lock);

  bare_addon_t *found = NULL;
  bare_addon_t *next = bare_addon_dynamic;

  while (next) {
    bare_addon_t *addon = next;

    next = addon->next;

    if (strcmp(specifier, addon->resolved) == 0) {
      found = addon;
      break;
    }
  }

  if (found) {
    found->refs++;

    uv_mutex_unlock(&bare_addon_lock);

    return found;
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
  found = next;

  next->resolved = strdup(specifier);
  next->lib = lib;

  uv_mutex_unlock(&bare_addon_lock);

  return found;

err:
  uv_mutex_unlock(&bare_addon_lock);

  err = js_throw_error(runtime->env, NULL, uv_dlerror(&lib));
  assert(err == 0);

  uv_dlclose(&lib);

  return NULL;
}

bool
bare_addon_unload(bare_runtime_t *runtime, bare_addon_t *addon) {
  uv_once(&bare_addon_guard, bare_addon_on_init);

  uv_mutex_lock(&bare_addon_lock);

  if (addon->refs == 0) {
    uv_mutex_unlock(&bare_addon_lock);

    return false;
  }

  bool unloaded = --addon->refs == 0;

  uv_mutex_unlock(&bare_addon_lock);

  return unloaded;
}

void
bare_addon_teardown(void) {
  uv_once(&bare_addon_guard, bare_addon_on_init);

  uv_mutex_lock(&bare_addon_lock);

  bare_addon_t *next = bare_addon_dynamic;

  while (next) {
    bare_addon_t *addon = next;

    next = addon->next;

    if (addon->refs) continue;

    uv_dlclose(&addon->lib);

    if (addon->previous) addon->previous->next = addon->next;
    else bare_addon_dynamic = addon->next;

    if (addon->next) addon->next->previous = addon->previous;

    free(addon->name);
    free(addon->resolved);
    free(addon);
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

  bare_addon_t *next = bare_addon_static;

  while (next) {
    bare_addon_t *addon = next;

    next = addon->next;

    const char *name = addon->name;

    if (name && strncmp(query, name, len) == 0) {
      result = &addon->lib;
      break;
    }
  }

  next = bare_addon_dynamic;

  while (next) {
    bare_addon_t *addon = next;

    next = addon->next;

    const char *name = addon->name;

    if (name && strncmp(query, name, len) == 0) {
      result = &addon->lib;
      break;
    }
  }

  uv_mutex_unlock(&bare_addon_lock);

  return result;
}

void
bare_module_register(bare_module_t *module) {
  uv_once(&bare_addon_guard, bare_addon_on_init);

  uv_mutex_lock(&bare_addon_lock);

  bool is_dynamic = bare_addon_pending == &bare_addon_dynamic;

  bare_addon_t *addon = malloc(sizeof(bare_addon_t));

  addon->next = NULL;
  addon->previous = NULL;

  addon->name = module->name ? strdup(module->name) : NULL;
  addon->exports = module->exports;

  if (is_dynamic) {
    addon->resolved = NULL;
    addon->refs = 1;
  } else {
    assert(module->name);

    addon->resolved = strdup(module->name);
    addon->refs = 0;
    addon->lib = bare_addon_lib;
  }

  addon->next = *bare_addon_pending;

  if (addon->next) addon->next->previous = addon;

  *bare_addon_pending = addon;

  if (is_dynamic) bare_addon_pending = NULL;

  uv_mutex_unlock(&bare_addon_lock);
}

void
napi_module_register(napi_module *module) {
  assert(module->nm_version == NAPI_MODULE_VERSION);

  bare_module_register(&(bare_module_t) {
    .version = BARE_MODULE_VERSION,
    .name = module->nm_filename,
    .exports = module->nm_register_func,
  });
}
