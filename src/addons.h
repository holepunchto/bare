#ifndef BARE_ADDONS_H
#define BARE_ADDONS_H

#include <assert.h>
#include <js.h>
#include <napi.h>
#include <path.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "../include/bare.h"
#include "types.h"

#define BARE_ADDONS_MAX_ENTRIES 256

typedef struct bare_module_list_s bare_module_list_t;

struct bare_module_list_s {
  bare_module_t mod;
  char *resolved;
  bool pending;
  uv_lib_t *lib;
  bare_module_list_t *next;
};

static bare_module_list_t *static_module_list = NULL;
static bare_module_list_t *dynamic_module_list = NULL;

static bare_module_list_t **pending_module = &static_module_list;

static uv_once_t module_guard = UV_ONCE_INIT;

static uv_rwlock_t module_lock;

static inline void
on_module_init () {
  int err = uv_rwlock_init(&module_lock);
  assert(err == 0);
}

static bool
bare_addons_has_extension (const char *s, const char *ext) {
  size_t s_len = strlen(s);
  size_t e_len = strlen(ext);

  return e_len <= s_len && strcmp(s + (s_len - e_len), ext) == 0;
}

static inline int
bare_addons_readdir (bare_runtime_t *runtime, const char *dirname, int entries_len, uv_dirent_t *entries) {
  uv_fs_t req;

  int num = 0;

  int err = uv_fs_opendir(runtime->loop, &req, dirname, NULL);
  if (err < 0) {
    uv_fs_req_cleanup(&req);
    return err;
  }

  uv_dir_t *dir = (uv_dir_t *) req.ptr;
  uv_fs_req_cleanup(&req);

  dir->dirents = entries;
  dir->nentries = entries_len;

  num = uv_fs_readdir(runtime->loop, &req, dir, NULL);
  if (num < 0) {
    uv_fs_req_cleanup(&req);
    return num;
  }

  err = uv_fs_closedir(runtime->loop, &req, dir, NULL);
  if (err < 0) {
    uv_fs_req_cleanup(&req);
    return err;
  }

  return num;
}

static bool
bare_addons_check_dir (bare_runtime_t *runtime, const char *path, char *out, size_t *len) {
  uv_dirent_t entries[BARE_ADDONS_MAX_ENTRIES];

  int entries_len = bare_addons_readdir(runtime, path, BARE_ADDONS_MAX_ENTRIES, (uv_dirent_t *) entries);
  if (entries_len <= 0) return false;

  const char *result = NULL;

  for (int i = 0; i < entries_len && result == NULL; i++) {
    const char *name = entries[i].name;
    if (strcmp(name, "node.napi.bare") == 0) {
      result = name;
    }
  }

  for (int i = 0; i < entries_len && result == NULL; i++) {
    const char *name = entries[i].name;
    if (bare_addons_has_extension(name, ".bare")) {
      result = name;
    }
  }

  for (int i = 0; i < entries_len && result == NULL; i++) {
    const char *name = entries[i].name;
    if (strcmp(name, "node.napi.node") == 0) {
      result = name;
    }
  }

  for (int i = 0; i < entries_len && result == NULL; i++) {
    const char *name = entries[i].name;
    if (bare_addons_has_extension(name, ".node")) {
      result = name;
    }
  }

  if (result != NULL) {
    path_join((const char *[]){path, result, NULL}, out, len, path_behavior_system);
    return true;
  }

  return false;
}

static inline int
bare_addons_resolve (bare_runtime_t *runtime, const char *path, char *out, size_t *len) {
  size_t tmp_len = 4096;
  char tmp[4096];

  if (bare_addons_has_extension(path, ".bare") || bare_addons_has_extension(path, ".node")) {
    if (out != path) strcpy(out, path);
    return 0;
  }

  path_join((const char *[]){path, "build", NULL}, tmp, &tmp_len, path_behavior_system);

  if (bare_addons_check_dir(runtime, tmp, out, len)) return 0;

  tmp_len = 4096;

  path_join((const char *[]){path, "build", "Release", NULL}, tmp, &tmp_len, path_behavior_system);

  if (bare_addons_check_dir(runtime, tmp, out, len)) return 0;

  tmp_len = 4096;

  path_join((const char *[]){path, "build", "Debug", NULL}, tmp, &tmp_len, path_behavior_system);

  if (bare_addons_check_dir(runtime, tmp, out, len)) return 0;

  tmp_len = 4096;

  path_join((const char *[]){path, "prebuilds", BARE_TARGET, NULL}, tmp, &tmp_len, path_behavior_system);

  if (bare_addons_check_dir(runtime, tmp, out, len)) return 0;

  return UV_ENOENT;
}

static inline js_value_t *
bare_addons_load (bare_runtime_t *runtime, const char *path) {
  uv_once(&module_guard, on_module_init);

  uv_rwlock_rdlock(&module_lock);

  int err;

  bare_module_t *mod = NULL;
  bare_module_list_t *next = static_module_list;

  while (next) {
    if (bare_addons_has_extension(path, next->mod.filename)) {
      mod = &next->mod;
      break;
    }

    next = next->next;
  }

  if (mod == NULL) {
    size_t resolved_len = 4096;
    char resolved[4096];

    err = bare_addons_resolve(runtime, path, resolved, &resolved_len);
    if (err < 0) {
      js_throw_errorf(runtime->env, NULL, "Could not resolve addon %s", path);
      return NULL;
    }

    bare_module_list_t *next = dynamic_module_list;

    while (next) {
      if (bare_addons_has_extension(resolved, next->resolved)) {
        mod = &next->mod;
        break;
      }

      next = next->next;
    }

    if (mod == NULL) {
      pending_module = &dynamic_module_list;

      uv_lib_t *lib = malloc(sizeof(uv_lib_t));

      uv_rwlock_rdunlock(&module_lock);

      err = uv_dlopen(resolved, lib);

      if (err < 0) {
        js_throw_error(runtime->env, NULL, uv_dlerror(lib));
        free(lib);
        return NULL;
      }

      uv_rwlock_rdlock(&module_lock);

      next = *pending_module;

      if (next && next->pending) {
        next->pending = false;
        next->resolved = strdup(resolved);
        next->lib = lib;

        mod = &next->mod;
      } else {
        uv_dlclose(lib);
        free(lib);
      }
    }
  }

  uv_rwlock_rdunlock(&module_lock);

  if (mod == NULL) {
    js_throw_errorf(runtime->env, NULL, "No module registered for %s", path);
    return NULL;
  }

  if (mod->version != BARE_MODULE_VERSION) {
    js_throw_errorf(runtime->env, NULL, "Unsupported ABI version %d for module %s", mod->version, path);
    return NULL;
  }

  js_value_t *exports;
  err = js_create_object(runtime->env, &exports);
  assert(err == 0);

  return mod->init(runtime->env, exports);
}

void
bare_module_register (bare_module_t *mod) {
  uv_once(&module_guard, on_module_init);

  uv_rwlock_wrlock(&module_lock);

  bool is_dynamic = pending_module == &dynamic_module_list;

  bare_module_list_t *next = malloc(sizeof(bare_module_list_t));

  next->mod.version = mod->version;
  next->mod.filename = mod->filename;
  next->mod.init = mod->init;

  next->resolved = NULL;
  next->pending = is_dynamic;
  next->next = *pending_module;

  *pending_module = next;

  uv_rwlock_wrunlock(&module_lock);
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

#endif // BARE_ADDONS_H
