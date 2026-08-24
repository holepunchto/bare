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

// Static addons are compiled into the binary and remain available for as long
// as the operating system process lives. They're therefore neither owned by any
// single process nor affected by its seal. The list is still guarded, as an
// addon may register at any point, including while a process is running.
static bare_addon_t *bare_addon__static = NULL;

// Dynamic addons are tracked in a single list spanning every process with each
// addon recording the process that loaded it. The list is shared so that an
// addon can be resolved from any thread of the process that loaded it, but
// ownership is what governs which addons a process may load, unload and find.
static bare_addon_t *bare_addon__dynamic = NULL;

// The lock guards both the dynamic addon list and the seal of every process.
//
// It must never be held across a call into the dynamic loader, which takes a
// lock of its own that on Windows is also taken to run the constructors of a
// library and to bind a delay loaded import. An addon may ask for a library
// from either, so the loader takes this lock underneath its own and holding it
// the other way around would deadlock the two against each other. Loading and
// unloading therefore only take it to read and to publish, never across the
// load itself.
//
// It is recursive to tolerate a lookup reentering from a library constructor.
static uv_mutex_t bare_addon__lock;

// Serializes loading against anything that changes what a load may see; another
// load of the same library, the unloading of one, and the sealing of a process.
//
// It's taken before the addon lock and before the lock of the loader, and
// neither of those is ever taken before it, so it can't take part in a cycle.
static uv_mutex_t bare_addon__loading;

static uv_once_t bare_addon__guard = UV_ONCE_INIT;

// Set for as long as a load is in flight on the thread. Registrations are
// attributed to the load while it is, and to the binary itself when it isn't.
static thread_local bare_process_t *bare_addon__pending_owner = NULL;
static thread_local uv_lib_t *bare_addon__pending_lib = NULL;
static thread_local const char *bare_addon__pending_specifier = NULL;

// Addons registering while a library loads are staged here rather than added to
// the shared list. Their library handle isn't known until the load completes.
static thread_local bare_addon_t *bare_addon__staging = NULL;

// The process whose runtime is currently executing on this thread, if any.
// `bare_module_find()` is called from the Windows delay load hook when an addon
// first calls into one of its dependencies, which may happen at any point after
// the addon was loaded, so the process asking can't be inferred from the call
// site and is instead tracked for as long as a runtime is entered.
static thread_local bare_process_t *bare_addon__current = NULL;

#ifndef _WIN32
// The global symbol scope of the program, which is where the code of a
// statically linked addon is found on platforms that have a runpath.
static uv_lib_t bare_addon__self;

static uv_once_t bare_addon__self_guard = UV_ONCE_INIT;

static void
bare_addon__on_self_init(void) {
  bare_addon__self.handle = dlopen(NULL, RTLD_LAZY);

  assert(bare_addon__self.handle);

  bare_addon__self.errmsg = NULL;
}
#endif

static void
bare_addon__on_init(void) {
  int err;

  err = uv_mutex_init_recursive(&bare_addon__lock);
  assert(err == 0);

  err = uv_mutex_init(&bare_addon__loading);
  assert(err == 0);
}

bare_process_t *
bare_addon_attach(bare_runtime_t *runtime) {
  bare_process_t *previous = bare_addon__current;

  bare_addon__current = runtime->process;

  return previous;
}

void
bare_addon_detach(bare_process_t *previous) {
  bare_addon__current = previous;
}

js_value_t *
bare_addon_get_static(bare_runtime_t *runtime) {
  int err;

  uv_once(&bare_addon__guard, bare_addon__on_init);

  js_value_t *result;
  err = js_create_array(runtime->env, &result);
  assert(err == 0);

  uv_mutex_lock(&bare_addon__lock);

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

  uv_mutex_unlock(&bare_addon__lock);

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

  uv_once(&bare_addon__guard, bare_addon__on_init);

  uv_mutex_lock(&bare_addon__lock);

  bare_addon_t *next = bare_addon__static;

  while (next) {
    bare_addon_t *addon = next;

    next = addon->next;

    if (strcmp(specifier, addon->specifier) == 0) {
      uv_mutex_unlock(&bare_addon__lock);

      return addon;
    }
  }

  uv_mutex_unlock(&bare_addon__lock);

  err = js_throw_errorf(runtime->env, NULL, "No addon registered for '%s'", specifier);
  assert(err == 0);

  return NULL;
}

bare_addon_t *
bare_addon_load_dynamic(bare_runtime_t *runtime, const char *specifier) {
  int err;

  uv_once(&bare_addon__guard, bare_addon__on_init);

  bare_process_t *process = runtime->process;

  uv_mutex_lock(&bare_addon__loading);

  uv_mutex_lock(&bare_addon__lock);

  bare_addon_t *next = bare_addon__dynamic;

  while (next) {
    bare_addon_t *addon = next;

    next = addon->next;

    // Only addons loaded by the process itself may be reused. Addons loaded by
    // another process must be loaded again to ensure that a sealed process
    // can't pick up addons that it never loaded.
    if (addon->owner == process && strcmp(specifier, addon->specifier) == 0) {
      uv_mutex_unlock(&bare_addon__lock);
      uv_mutex_unlock(&bare_addon__loading);

      return addon;
    }
  }

  if (process->sealed) {
    uv_mutex_unlock(&bare_addon__lock);
    uv_mutex_unlock(&bare_addon__loading);

    err = js_throw_errorf(runtime->env, NULL, "Cannot load addon '%s' because addon loading has been sealed", specifier);
    assert(err == 0);

    return NULL;
  }

  uv_mutex_unlock(&bare_addon__lock);

  uv_lib_t lib;

  // Discard anything left staged by a load that failed before it could publish.
  bare_addon__staging = NULL;

  bare_addon__pending_owner = process;
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

  // Addons that register from a constructor do so during `dlopen()`, before the
  // library handle is known, so their handle is stale. Refresh any addons
  // staged during the load with the now-known handle.
  for (bare_addon_t *addon = bare_addon__staging; addon; addon = addon->next) {
    addon->lib = lib;
  }

  // Nothing staged means the library registers from a known symbol rather than
  // from a constructor.
  if (bare_addon__staging == NULL) {
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

      uv_mutex_lock(&bare_addon__lock);

      bare_addon_t *resident = bare_addon__dynamic;

      while (resident && resident->lib.handle != lib.handle) {
        resident = resident->next;
      }

      if (resident == NULL) {
        uv_mutex_unlock(&bare_addon__lock);

        goto err;
      }

      // Registered with the lock held as the registration copies the name of
      // the resident addon, which its owner would otherwise be free to unload.
      // Registration stages the addon and so doesn't take the lock itself.
      bare_module_register(&(bare_module_t){
        .version = BARE_MODULE_VERSION,
        .name = resident->name,
        .exports = resident->exports,
      });

      uv_mutex_unlock(&bare_addon__lock);
    } else {
      bare_module_register(&(bare_module_t){
        .version = BARE_MODULE_VERSION,
        .name = name == NULL ? NULL : name(),
        .exports = exports,
      });
    }
  }

  // Only one of the staged addons may release the reference that the load took,
  // however many of them registered against it.
  assert(bare_addon__staging);

  bare_addon__staging->unloads = true;

  // Publish the staged addons now that their handles are known, keeping the
  // order they registered in.
  next = bare_addon__staging;

  uv_mutex_lock(&bare_addon__lock);

  bare_addon_t **tail = &bare_addon__staging;

  while (*tail) {
    tail = &(*tail)->next;
  }

  *tail = bare_addon__dynamic;

  bare_addon__dynamic = bare_addon__staging;

  uv_mutex_unlock(&bare_addon__lock);

  bare_addon__staging = NULL;

  bare_addon__pending_owner = NULL;
  bare_addon__pending_lib = NULL;
  bare_addon__pending_specifier = NULL;

  uv_mutex_unlock(&bare_addon__loading);

  return next;

err:
  bare_addon__pending_owner = NULL;
  bare_addon__pending_lib = NULL;
  bare_addon__pending_specifier = NULL;

  uv_mutex_unlock(&bare_addon__loading);

  err = js_throw_error(runtime->env, NULL, uv_dlerror(&lib));
  assert(err == 0);

  uv_dlclose(&lib);

  return NULL;
}

void
bare_addon_seal(bare_process_t *process) {
  uv_once(&bare_addon__guard, bare_addon__on_init);

  // Taken so that the seal waits for a load already in flight on another thread
  // rather than being granted while it completes.
  uv_mutex_lock(&bare_addon__loading);

  uv_mutex_lock(&bare_addon__lock);

  process->sealed = true;

  uv_mutex_unlock(&bare_addon__lock);

  uv_mutex_unlock(&bare_addon__loading);
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

  // Unlink every addon owned by the process before unloading any of them, as
  // unloading takes the lock of the dynamic loader and the addon lock must not
  // be held across it. The order they were loaded in is preserved.
  bare_addon_t *unloading = NULL;

  bare_addon_t **tail = &unloading;

  // Taken so that an addon can't be unlinked while another thread is loading
  // the same library and is about to look it up to recover its registration.
  uv_mutex_lock(&bare_addon__loading);

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

    addon->next = NULL;

    *tail = addon;
    tail = &addon->next;
  }

  uv_mutex_unlock(&bare_addon__lock);

  uv_mutex_unlock(&bare_addon__loading);

  while (unloading) {
    bare_addon_t *addon = unloading;

    unloading = addon->next;

    if (addon->unloads) uv_dlclose(&addon->lib);

    free(addon);
  }
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

  uv_mutex_lock(&bare_addon__lock);

  // Statically linked addons are compiled into the binary and stay loaded for
  // as long as the operating system process, so they're available to every
  // process and need no ownership check.
  next = bare_addon__static;

  while (next) {
    bare_addon_t *addon = next;

    next = addon->next;

    if (bare_addon__matches(query, len, addon->name)) {
      uv_mutex_unlock(&bare_addon__lock);

      return &addon->lib;
    }
  }

  // Dynamically loaded addons are only matched for the process that loaded
  // them. The caller is handed a library that it may hold on to indefinitely
  // and only the owning process can say how long the library stays loaded.
  // Matching across processes would also hand a sealed process an addon that it
  // never loaded itself.
  bare_process_t *process = bare_addon__current;

  if (process == NULL) {
    uv_mutex_unlock(&bare_addon__lock);

    return NULL;
  }

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

// The module that carries an addon's code. For a statically linked addon that's
// whichever binary it was linked into, which is the executable when Bare is
// linked statically but the shared library that Bare lives in when an embedder
// bundles its addons alongside it. It's resolved from the address of the
// callback the addon registers rather than assumed to be the executable, as
// looking an addon up has to yield the module that actually exports its
// symbols.
static inline uv_lib_t
bare_addon__module(bare_module_register_cb exports) {
  uv_lib_t lib = {.handle = NULL, .errmsg = NULL};

#ifdef _WIN32
  HMODULE module;

  // The reference count is left alone as the handle is never closed again.
  BOOL ok = GetModuleHandleExW(
    GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
    (LPCWSTR) exports,
    &module
  );

  lib.handle = ok ? module : GetModuleHandleW(NULL);
#else
  // Nothing resolves symbols through this handle on platforms that have a
  // runpath, so the global scope of the program is handle enough.
  (void) exports;

  uv_once(&bare_addon__self_guard, bare_addon__on_self_init);

  lib = bare_addon__self;
#endif

  assert(lib.handle);

  return lib;
}

void
bare_module_register(bare_module_t *module) {
  uv_once(&bare_addon__guard, bare_addon__on_init);

  // A registration belongs to the load in flight on the thread, if there is
  // one. Anything registering outside a load is statically linked.
  bool is_dynamic = bare_addon__pending_owner != NULL;

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
  }

  addon->exports = module->exports;
  addon->owner = is_dynamic ? bare_addon__pending_owner : NULL;

  // A dynamic addon is registered while its library loads and so takes the
  // handle of that library. A statically linked addon has no load of its own
  // and is resolved to the module it was linked into instead.
  addon->lib = is_dynamic
                 ? *bare_addon__pending_lib
                 : bare_addon__module(module->exports);

  // The load takes the reference and hands it to whichever addon it publishes
  // first, so a registration never holds one of its own.
  addon->unloads = false;

  if (is_dynamic) {
    // Staged on the thread, so no lock; the load publishes them once their
    // handles are known.
    addon->next = bare_addon__staging;

    bare_addon__staging = addon;
  } else {
    uv_mutex_lock(&bare_addon__lock);

    addon->next = bare_addon__static;

    bare_addon__static = addon;

    uv_mutex_unlock(&bare_addon__lock);
  }
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
