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

// Static addons are registered before any process is set up and remain
// available for as long as the operating system process lives. They're
// therefore neither owned by any single process nor affected by its seal and
// the list needs no locking.
static bare_addon_t *bare_addon__static = NULL;

// Dynamic addons are tracked in a single list spanning every process with each
// addon recording the process that loaded it. The list is shared so that an
// addon can be resolved from any thread of the process that loaded it, but
// ownership is what governs which addons a process may load, unload and find.
static bare_addon_t *bare_addon__dynamic = NULL;

// The lock guards both the dynamic addon list and the seal of every process.
// It is recursive as `bare_module_find()` may be reentered while an addon is
// being loaded on the same thread.
static uv_mutex_t bare_addon__lock;
static uv_once_t bare_addon__guard = UV_ONCE_INIT;

static thread_local bare_addon_t **bare_addon__pending = &bare_addon__static;
static thread_local bare_process_t *bare_addon__pending_owner = NULL;
static thread_local uv_lib_t *bare_addon__pending_lib = NULL;
static thread_local const char *bare_addon__pending_specifier = NULL;

// The process running on this thread, if any. `bare_module_find()` is called
// from the Windows delay load hook when an addon first calls into one of its
// dependencies, which may happen at any point after the addon was loaded, so
// the process asking can't be inferred from the call site and is instead
// tracked for as long as a runtime is set up on the thread.
static thread_local bare_process_t *bare_addon__current = NULL;

static void
bare_addon__on_init(void) {
  int err;

  err = uv_mutex_init_recursive(&bare_addon__lock);
  assert(err == 0);
}

void
bare_addon_attach(bare_runtime_t *runtime) {
  runtime->previous = bare_addon__current;

  bare_addon__current = runtime->process;
}

void
bare_addon_detach(bare_runtime_t *runtime) {
  bare_addon__current = runtime->previous;

  runtime->previous = NULL;
}

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

  uv_once(&bare_addon__guard, bare_addon__on_init);

  js_value_t *result;
  err = js_create_array(runtime->env, &result);
  assert(err == 0);

  uv_mutex_lock(&bare_addon__lock);

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

  uv_mutex_unlock(&bare_addon__lock);

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

  uv_once(&bare_addon__guard, bare_addon__on_init);

  uv_mutex_lock(&bare_addon__lock);

  bare_process_t *process = runtime->process;

  bare_addon_t *next = bare_addon__dynamic;

  while (next) {
    bare_addon_t *addon = next;

    next = addon->next;

    // Only addons loaded by the process itself may be reused. Addons loaded by
    // another process must be loaded again to ensure that a sealed process
    // can't pick up addons that it never loaded.
    if (addon->owner == process && strcmp(specifier, addon->specifier) == 0) {
      uv_mutex_unlock(&bare_addon__lock);

      return addon;
    }
  }

  if (process->sealed) {
    uv_mutex_unlock(&bare_addon__lock);

    err = js_throw_errorf(runtime->env, NULL, "Cannot load addon '%s' because addon loading has been sealed", specifier);
    assert(err == 0);

    return NULL;
  }

  uv_lib_t lib;

  bare_addon__pending = &bare_addon__dynamic;
  bare_addon__pending_owner = process;
  bare_addon__pending_lib = &lib;
  bare_addon__pending_specifier = specifier;

  // Snapshot the head of the list so that any addons registered from a
  // constructor during the load, before the library handle is known, can be
  // updated with the real handle once it is.
  bare_addon_t *loaded = bare_addon__dynamic;

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

  // Addons that register from a constructor do so during `dlopen()`, before the
  // library handle is known, so their handle is stale. Refresh any addons
  // registered during the load with the now-known handle.
  for (bare_addon_t *addon = bare_addon__dynamic; addon != loaded; addon = addon->next) {
    addon->lib = lib;
  }

  if (bare_addon__pending) {
    bare_module_name_cb name;

    err = uv_dlsym(&lib, BARE_STRING(BARE_MODULE_SYMBOL_NAME), (void **) &name);

    if (err < 0) name = NULL;

    bare_module_register_cb exports;

    err = uv_dlsym(&lib, BARE_STRING(BARE_MODULE_SYMBOL_REGISTER), (void **) &exports);

    if (err < 0) {
      err = uv_dlsym(&lib, BARE_STRING(NAPI_MODULE_SYMBOL_REGISTER), (void **) &exports);
    }

    if (err < 0) {
      // The library exposes no registration symbol. This happens for addons
      // that register from a constructor rather than from a known symbol. Reuse
      // the registration captured when the library was first loaded, identified
      // by its shared library handle.
      bare_addon_t *resident = bare_addon__dynamic;

      while (resident && resident->lib.handle != lib.handle) {
        resident = resident->next;
      }

      if (resident == NULL) goto err;

      bare_module_register(&(bare_module_t){
        .version = BARE_MODULE_VERSION,
        .name = resident->name,
        .exports = resident->exports,
      });
    } else {
      bare_module_register(&(bare_module_t){
        .version = BARE_MODULE_VERSION,
        .name = name == NULL ? NULL : name(),
        .exports = exports,
      });
    }
  }

  next = bare_addon__dynamic;

  uv_mutex_unlock(&bare_addon__lock);

  return next;

err:
  uv_mutex_unlock(&bare_addon__lock);

  err = js_throw_error(runtime->env, NULL, uv_dlerror(&lib));
  assert(err == 0);

  uv_dlclose(&lib);

  return NULL;
}

void
bare_addon_seal(bare_process_t *process) {
  uv_once(&bare_addon__guard, bare_addon__on_init);

  uv_mutex_lock(&bare_addon__lock);

  process->sealed = true;

  uv_mutex_unlock(&bare_addon__lock);
}

bool
bare_addon_sealed(bare_process_t *process) {
  uv_once(&bare_addon__guard, bare_addon__on_init);

  uv_mutex_lock(&bare_addon__lock);

  bool sealed = process->sealed;

  uv_mutex_unlock(&bare_addon__lock);

  return sealed;
}

void
bare_addon_teardown(bare_process_t *process) {
  uv_once(&bare_addon__guard, bare_addon__on_init);

  uv_mutex_lock(&bare_addon__lock);

  bare_addon_t **previous = &bare_addon__dynamic;

  bare_addon_t *next = bare_addon__dynamic;

  while (next) {
    bare_addon_t *addon = next;

    next = addon->next;

    if (addon->owner != process) {
      previous = &addon->next;

      continue;
    }

    *previous = next;

    uv_dlclose(&addon->lib);

    free(addon);
  }

  uv_mutex_unlock(&bare_addon__lock);
}

// Addons are named `<name>@<version>` and may be looked up by a truncated
// version, which is how the delay loader on Windows resolves an addon by its
// major version alone. The truncation must fall on a version component boundary
// so that `foo@1` matches `foo@1.2.3` without also matching `foo@10.0.0`.
static inline bool
bare_addon__matches(const char *query, size_t len, const char *name) {
  if (name == NULL) return false;

  // A match over the first `len` characters implies that the name is at least
  // that long, so indexing it by `len` is safe.
  if (strncmp(query, name, len) != 0) return false;

  return name[len] == '\0' || name[len] == '.';
}

uv_lib_t *
bare_module_find(const char *query) {
  uv_once(&bare_addon__guard, bare_addon__on_init);

  size_t len = strlen(query);

  if (len > 5 && strcmp(&query[len - 5], ".bare") == 0) len -= 5;

  bare_addon_t *next;

  // Statically linked addons are compiled into the binary and stay loaded for
  // as long as the operating system process, so they're available to every
  // process and need no ownership check.
  next = bare_addon__static;

  while (next) {
    bare_addon_t *addon = next;

    next = addon->next;

    if (bare_addon__matches(query, len, addon->name)) {
      return &addon->lib;
    }
  }

  // Dynamically loaded addons are only matched for the process that loaded
  // them. The caller is handed a library that it may hold on to indefinitely
  // and only the owning process can say how long the library stays loaded.
  // Matching across processes would also hand a sealed process an addon that it
  // never loaded itself.
  bare_process_t *process = bare_addon__current;

  if (process == NULL) return NULL;

  uv_mutex_lock(&bare_addon__lock);

  next = bare_addon__dynamic;

  while (next) {
    bare_addon_t *addon = next;

    next = addon->next;

    if (addon->owner != process) continue;

    if (bare_addon__matches(query, len, addon->name)) {
      uv_mutex_unlock(&bare_addon__lock);

      return &addon->lib;
    }
  }

  uv_mutex_unlock(&bare_addon__lock);

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
  addon->owner = is_dynamic ? bare_addon__pending_owner : NULL;
  addon->lib = *bare_addon__pending_lib;
  addon->next = *bare_addon__pending;

  *bare_addon__pending = addon;

  if (is_dynamic) bare_addon__pending = NULL;
}

void
napi_module_register(napi_module *module) {
  assert(module->nm_version == NAPI_MODULE_VERSION);

  bare_module_register(&(bare_module_t){
    .version = BARE_MODULE_VERSION,
    .name = module->nm_filename,
    .exports = module->nm_register_func,
  });
}
