#ifndef PEAR_ADDONS_H
#define PEAR_ADDONS_H

#include <assert.h>
#include <js.h>
#include <napi.h>
#include <path.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "../include/pear.h"
#include "addons.h"
#include "fs.h"

#define PEAR_ADDONS_MAX_ENTRIES 256

typedef struct pear_module_list_s pear_module_list_t;

struct pear_module_list_s {
  pear_module_t mod;
  pear_module_list_t *next;
};

static pear_module_list_t *static_modules = NULL;
static pear_module_list_t *dynamic_modules = NULL;

static pear_module_list_t **pending_module = &static_modules;

static bool
has_extension (const char *s, const char *ext) {
  size_t s_len = strlen(s);
  size_t e_len = strlen(ext);

  return e_len <= s_len && strcmp(s + (s_len - e_len), ext) == 0;
}

static bool
check_addon_dir (pear_t *pear, const char *path, char *out, size_t *len) {
  uv_dirent_t entries[PEAR_ADDONS_MAX_ENTRIES];

  int entries_len = pear_fs_readdir_sync(pear, path, PEAR_ADDONS_MAX_ENTRIES, (uv_dirent_t *) entries);
  if (entries_len <= 0) return false;

  const char *result = NULL;

  for (int i = 0; i < entries_len && result == NULL; i++) {
    const char *name = entries[i].name;
    if (strcmp(name, "node.napi.pear") == 0) {
      result = name;
    }
  }

  for (int i = 0; i < entries_len && result == NULL; i++) {
    const char *name = entries[i].name;
    if (has_extension(name, ".pear")) {
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
    if (has_extension(name, ".node")) {
      result = name;
    }
  }

  if (result != NULL) {
    path_join((const char *[]){path, result, NULL}, out, len, path_behavior_system);
    return true;
  }

  return false;
}

static inline void
pear_addons_init () {
  pending_module = &dynamic_modules;
}

static inline int
pear_addons_resolve (pear_t *pear, const char *path, char *out, size_t *len) {
  size_t tmp_len = PEAR_FS_MAX_PATH;
  char tmp[PEAR_FS_MAX_PATH];

  if (has_extension(path, ".pear") || has_extension(path, ".node")) {
    if (out != path) strcpy(out, path);
    return 0;
  }

  // TODO: check where cmake likes to build...

  // check for pearjs dev build compat
  path_join((const char *[]){path, "build", NULL}, tmp, &tmp_len, path_behavior_system);
  if (check_addon_dir(pear, tmp, out, len)) return 0;

  tmp_len = PEAR_FS_MAX_PATH;

  // node-gyp compat
  path_join((const char *[]){path, "build", "Release", NULL}, tmp, &tmp_len, path_behavior_system);
  if (check_addon_dir(pear, tmp, out, len)) return 0;

  tmp_len = PEAR_FS_MAX_PATH;

  // check for prebuilds
  path_join((const char *[]){path, "prebuilds", PEAR_PLATFORM "-" PEAR_ARCH, NULL}, tmp, &tmp_len, path_behavior_system);
  if (check_addon_dir(pear, tmp, out, len)) return 0;

  return UV_ENOENT;
}

static inline js_value_t *
pear_addons_load (pear_t *pear, const char *path) {
  int err;

  pear_module_t *mod = NULL;
  pear_module_list_t *next = static_modules;

  while (next) {
    if (has_extension(path, next->mod.filename)) {
      mod = &next->mod;
      break;
    }

    next = next->next;
  }

  if (mod == NULL) {
    size_t resolved_len = PEAR_FS_MAX_PATH;
    char resolved[PEAR_FS_MAX_PATH];

    err = pear_addons_resolve(pear, path, resolved, &resolved_len);
    if (err < 0) {
      js_throw_errorf(pear->env, NULL, "Could not resolve addon %s", path);
      return NULL;
    }

    uv_lib_t *lib = malloc(sizeof(uv_lib_t));

    err = uv_dlopen(resolved, lib);
    if (err < 0) {
      js_throw_error(pear->env, NULL, uv_dlerror(lib));
      free(lib);
      return NULL;
    }

    if (dynamic_modules) {
      mod = &dynamic_modules->mod;
      mod->lib = lib;
    } else {
      uv_dlclose(lib);
      free(lib);
    }
  }

  if (mod == NULL) {
    js_throw_errorf(pear->env, NULL, "No module registered for %s", path);
    return NULL;
  }

  js_value_t *exports = mod->exports;

  if (exports == NULL) {
    err = js_create_object(pear->env, &exports);
    assert(err == 0);

    exports = mod->exports = mod->init(pear->env, exports);
  }

  return exports;
}

void
pear_module_register (pear_module_t *mod) {
  pear_module_list_t *next = malloc(sizeof(pear_module_list_t));

  next->mod = *mod;
  next->next = *pending_module;

  *pending_module = next;
}

void
napi_module_register (napi_module *mod) {
  pear_module_register(&(pear_module_t){
    .version = PEAR_MODULE_VERSION,
    .filename = mod->nm_filename,
    .init = mod->nm_register_func,
  });
}

#endif // PEAR_ADDONS_H
