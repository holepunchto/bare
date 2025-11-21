#include <assert.h>
#include <js.h>
#include <napi.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <utf.h>
#include <uv.h>

#ifndef _WIN32
#include <dlfcn.h>
#endif

#ifndef thread_local
#ifdef _WIN32
#define thread_local __declspec(thread)
#else
#define thread_local _Thread_local
#endif
#endif

#include "types.h"

static bare_addon_t *bare_addon__static = NULL;
static thread_local bare_addon_t *bare_addon__dynamic = NULL;
static thread_local bare_addon_t **bare_addon__pending = &bare_addon__static;
static thread_local uv_lib_t *bare_addon__pending_lib = NULL;
static thread_local const char *bare_addon__pending_specifier = NULL;

js_value_t *
bare_addon_get_static(bare_runtime_t *runtime) {
  int err;

  js_value_t *result;
  err = js_create_array(runtime->env, &result);
  assert(err == 0);

  bare_addon_t *next = bare_addon__static;

  uint32_t i = 0;

  while (next) {
    bare_addon_t *addon = next;

    next = addon->next;

    js_value_t *specifier;
    err = js_create_string_utf8(runtime->env, (utf8_t *) addon->specifier, (size_t) -1, &specifier);
    assert(err == 0);

    err = js_set_element(runtime->env, result, i++, specifier);
    assert(err == 0);
  }

  return result;
}

js_value_t *
bare_addon_get_dynamic(bare_runtime_t *runtime) {
  int err;

  js_value_t *result;
  err = js_create_array(runtime->env, &result);
  assert(err == 0);

  bare_addon_t *next = bare_addon__dynamic;

  uint32_t i = 0;

  while (next) {
    bare_addon_t *addon = next;

    next = addon->next;

    js_value_t *specifier;
    err = js_create_string_utf8(runtime->env, (utf8_t *) addon->specifier, (size_t) -1, &specifier);
    assert(err == 0);

    err = js_set_element(runtime->env, result, i++, specifier);
    assert(err == 0);
  }

  return result;
}

bare_addon_t *
bare_addon_load_static(bare_runtime_t *runtime, const char *specifier) {
  int err;

  bare_addon_t *next = bare_addon__static;

  while (next) {
    bare_addon_t *addon = next;

    next = addon->next;

    if (strcmp(specifier, addon->specifier) == 0) {
      return addon;
    }
  }

  err = js_throw_errorf(runtime->env, NULL, "No addon registered for '%s'", specifier);
  assert(err == 0);

  return NULL;
}

bare_addon_t *
bare_addon_load_dynamic(bare_runtime_t *runtime, const char *specifier) {
  int err;

  bare_addon_t *next = bare_addon__dynamic;

  while (next) {
    bare_addon_t *addon = next;

    next = addon->next;

    if (strcmp(specifier, addon->specifier) == 0) {
      return addon;
    }
  }

  uv_lib_t lib;

  bare_addon__pending = &bare_addon__dynamic;
  bare_addon__pending_lib = &lib;
  bare_addon__pending_specifier = specifier;

#ifdef _WIN32
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

  if (bare_addon__pending) {
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
  }

  return bare_addon__dynamic;

err:
  err = js_throw_error(runtime->env, NULL, uv_dlerror(&lib));
  assert(err == 0);

  uv_dlclose(&lib);

  return NULL;
}

void
bare_addon_teardown(void) {
  bare_addon_t *next = bare_addon__dynamic;

  bare_addon__dynamic = NULL;

  while (next) {
    bare_addon_t *addon = next;

    next = addon->next;

    uv_dlclose(&addon->lib);

    free(addon);
  }
}

uv_lib_t *
bare_module_find(const char *query) {
  size_t len = strlen(query);

  if (len > 5 && strcmp(&query[len - 5], ".bare") == 0) len -= 5;

  bare_addon_t *next;

  next = bare_addon__static;

  while (next) {
    bare_addon_t *addon = next;

    next = addon->next;

    const char *name = addon->name;

    if (name && strncmp(query, name, len) == 0) {
      return &addon->lib;
    }
  }

  next = bare_addon__dynamic;

  while (next) {
    bare_addon_t *addon = next;

    next = addon->next;

    const char *name = addon->name;

    if (name && strncmp(query, name, len) == 0) {
      return &addon->lib;
    }
  }

  return NULL;
}

void
bare_module_register(bare_module_t *module) {
  bool is_dynamic = bare_addon__pending == &bare_addon__dynamic;

  bare_addon_t *addon;

  size_t len = sizeof(bare_addon_t);

  if (is_dynamic) {
    size_t offset = 0;

    if (module->name) len += offset = strlen(module->name) + 1 /* NULL */;

    len += strlen(bare_addon__pending_specifier) + 1 /* NULL */;

    addon = malloc(len);

    if (module->name) {
      addon->name = (char *) addon + sizeof(bare_addon_t);

      strcpy(addon->name, module->name);
    } else {
      addon->name = NULL;
    }

    addon->specifier = (char *) addon + sizeof(bare_addon_t) + offset;

    strcpy(addon->specifier, bare_addon__pending_specifier);
  } else {
    assert(module->name);

    len += strlen(module->name) + 1 /* NULL */;

    addon = malloc(len);

    addon->name = addon->specifier = (char *) addon + sizeof(bare_addon_t);

    strcpy(addon->name, module->name);

    if (bare_addon__pending_lib == NULL) {
      static uv_lib_t lib;

#ifdef _WIN32
      lib.handle = GetModuleHandleW(NULL);
#else
      lib.handle = dlopen(NULL, RTLD_LAZY);
#endif
      assert(lib.handle);

      lib.errmsg = NULL;

      bare_addon__pending_lib = &lib;
    }
  }

  addon->exports = module->exports;
  addon->lib = *bare_addon__pending_lib;
  addon->next = *bare_addon__pending;

  *bare_addon__pending = addon;

  if (is_dynamic) bare_addon__pending = NULL;
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
