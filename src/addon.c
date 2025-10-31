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

typedef struct bare_addon_ref_s bare_addon_ref_t;

static bare_addon_t *bare_addon__static = NULL;
static bare_addon_t *bare_addon__dynamic = NULL;
static bare_addon_t **bare_addon__pending = &bare_addon__static;

static uv_mutex_t bare_addon__lock;
static uv_lib_t bare_addon__lib;
static uv_once_t bare_addon__guard = UV_ONCE_INIT;

struct bare_addon_ref_s {
  bare_addon_t *addon;
};

static void
bare_addon__on_init(void) {
  int err;

  err = uv_mutex_init_recursive(&bare_addon__lock);
  assert(err == 0);

#if defined(_WIN32)
  bare_addon__lib.handle = GetModuleHandleW(NULL);
#else
  bare_addon__lib.handle = dlopen(NULL, RTLD_LAZY);
#endif

  bare_addon__lib.errmsg = NULL;

  assert(bare_addon__lib.handle);
}

static void
bare_addon__on_teardown(void *data) {
  bare_addon_ref_t *ref = data;

  uv_mutex_lock(&bare_addon__lock);

  ref->addon->refs--;

  uv_mutex_unlock(&bare_addon__lock);

  free(ref);
}

js_value_t *
bare_addon_get_static(bare_runtime_t *runtime) {
  uv_once(&bare_addon__guard, bare_addon__on_init);

  int err;

  uv_mutex_lock(&bare_addon__lock);

  js_value_t *result;
  err = js_create_array(runtime->env, &result);
  assert(err == 0);

  bare_addon_t *next = bare_addon__static;

  uint32_t i = 0;

  while (next) {
    bare_addon_t *addon = next;

    next = addon->next;

    js_value_t *specifier;
    err = js_create_string_utf8(runtime->env, (utf8_t *) addon->resolved, (size_t) -1, &specifier);
    assert(err == 0);

    err = js_set_element(runtime->env, result, i++, specifier);
    assert(err == 0);
  }

  uv_mutex_unlock(&bare_addon__lock);

  return result;
}

js_value_t *
bare_addon_get_dynamic(bare_runtime_t *runtime) {
  uv_once(&bare_addon__guard, bare_addon__on_init);

  int err;

  uv_mutex_lock(&bare_addon__lock);

  js_value_t *result;
  err = js_create_array(runtime->env, &result);
  assert(err == 0);

  bare_addon_t *next = bare_addon__dynamic;

  uint32_t i = 0;

  while (next) {
    bare_addon_t *addon = next;

    next = addon->next;

    js_value_t *specifier;
    err = js_create_string_utf8(runtime->env, (utf8_t *) addon->resolved, (size_t) -1, &specifier);
    assert(err == 0);

    err = js_set_element(runtime->env, result, i++, specifier);
    assert(err == 0);
  }

  uv_mutex_unlock(&bare_addon__lock);

  return result;
}

bare_addon_t *
bare_addon_load_static(bare_runtime_t *runtime, const char *specifier) {
  uv_once(&bare_addon__guard, bare_addon__on_init);

  int err;

  uv_mutex_lock(&bare_addon__lock);

  bare_addon_t *found = NULL;
  bare_addon_t *next = bare_addon__static;

  while (next) {
    bare_addon_t *addon = next;

    next = addon->next;

    if (strcmp(specifier, addon->resolved) == 0) {
      found = addon;
      break;
    }
  }

  uv_mutex_unlock(&bare_addon__lock);

  if (found == NULL) {
    err = js_throw_errorf(runtime->env, NULL, "No addon registered for '%s'", specifier);
    assert(err == 0);

    return NULL;
  }

  return found;
}

bare_addon_t *
bare_addon_load_dynamic(bare_runtime_t *runtime, const char *specifier) {
  uv_once(&bare_addon__guard, bare_addon__on_init);

  int err;

  uv_mutex_lock(&bare_addon__lock);

  bare_addon_ref_t *ref;

  bare_addon_t *found = NULL;
  bare_addon_t *next = bare_addon__dynamic;

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

    goto done;
  }

  bare_addon__pending = &bare_addon__dynamic;

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

  if (bare_addon__pending == NULL) goto registered; // Addon registered itself

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

registered:
  found = bare_addon__dynamic;

  found->resolved = strdup(specifier);
  found->lib = lib;

done:
  ref = malloc(sizeof(bare_addon_ref_t));
  ref->addon = found;

  err = js_add_teardown_callback(runtime->env, bare_addon__on_teardown, ref);
  assert(err == 0);

  uv_mutex_unlock(&bare_addon__lock);

  return found;

err:
  uv_mutex_unlock(&bare_addon__lock);

  err = js_throw_error(runtime->env, NULL, uv_dlerror(&lib));
  assert(err == 0);

  uv_dlclose(&lib);

  return NULL;
}

void
bare_addon_teardown(void) {
  uv_once(&bare_addon__guard, bare_addon__on_init);

  uv_mutex_lock(&bare_addon__lock);

  bare_addon_t *next = bare_addon__dynamic;

  while (next) {
    bare_addon_t *addon = next;

    next = addon->next;

    if (addon->refs) continue;

    uv_dlclose(&addon->lib);

    if (addon->previous) addon->previous->next = addon->next;
    else bare_addon__dynamic = addon->next;

    if (addon->next) addon->next->previous = addon->previous;

    free(addon->name);
    free(addon->resolved);
    free(addon);
  }

  uv_mutex_unlock(&bare_addon__lock);
}

uv_lib_t *
bare_module_find(const char *query) {
  uv_once(&bare_addon__guard, bare_addon__on_init);

  size_t len = strlen(query);

  if (len > 5 && strcmp(&query[len - 5], ".bare") == 0) len -= 5;

  uv_lib_t *result = NULL;

  uv_mutex_lock(&bare_addon__lock);

  bare_addon_t *next = bare_addon__static;

  while (next) {
    bare_addon_t *addon = next;

    next = addon->next;

    const char *name = addon->name;

    if (name && strncmp(query, name, len) == 0) {
      result = &addon->lib;
      break;
    }
  }

  next = bare_addon__dynamic;

  while (next) {
    bare_addon_t *addon = next;

    next = addon->next;

    const char *name = addon->name;

    if (name && strncmp(query, name, len) == 0) {
      result = &addon->lib;
      break;
    }
  }

  uv_mutex_unlock(&bare_addon__lock);

  return result;
}

void
bare_module_register(bare_module_t *module) {
  uv_once(&bare_addon__guard, bare_addon__on_init);

  uv_mutex_lock(&bare_addon__lock);

  bool is_dynamic = bare_addon__pending == &bare_addon__dynamic;

  bare_addon_t *addon = malloc(sizeof(bare_addon_t));

  addon->name = module->name ? strdup(module->name) : NULL;
  addon->exports = module->exports;
  addon->previous = NULL;
  addon->next = NULL;

  if (is_dynamic) {
    addon->resolved = NULL;
    addon->refs = 1;
  } else {
    assert(module->name);

    addon->resolved = strdup(module->name);
    addon->refs = 0;
    addon->lib = bare_addon__lib;
  }

  addon->next = *bare_addon__pending;

  if (addon->next) addon->next->previous = addon;

  *bare_addon__pending = addon;

  if (is_dynamic) bare_addon__pending = NULL;

  uv_mutex_unlock(&bare_addon__lock);
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
