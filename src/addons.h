#ifndef PEAR_ADDONS_H
#define PEAR_ADDONS_H

#include <js.h>
#include <napi.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "../include/pear.h"
#include "addons.h"
#include "fs.h"

#define PEAR_ADDONS_MAX_ENTRIES 256

#define PEAR_ADDONS_DYNAMIC 1
#define PEAR_ADDONS_STATIC  2
#define PEAR_ADDONS_RESOLVE 4

static pear_module_t *pending_dynamic_module = NULL;
static pear_module_t *pending_static_module = NULL;
static pear_module_t **pending_module = &pending_static_module;

static pear_module_t *
shift_pending_dynamic_addon () {
  pear_module_t *mod = pending_dynamic_module;
  pear_module_t *prev = NULL;

  if (mod == NULL) return NULL;

  while (mod->next_module != NULL) {
    prev = mod;
    mod = mod->next_module;
  }

  if (prev == NULL) pending_dynamic_module = NULL;
  else prev->next_module = mod->next_module;

  return mod;
}

static bool
has_extension (const char *s, const char *ext) {
  size_t s_len = strlen(s);
  size_t e_len = strlen(ext);

  return e_len <= s_len && strcmp(s + (s_len - e_len), ext) == 0;
}

static bool
has_dirname (const char *s, const char *dir) {
  size_t s_len = strlen(s);
  size_t dir_len = strlen(dir);

  if (dir_len == s_len) return memcmp(dir, s, dir_len) == 0;

  if (dir_len > s_len) return false;
  if (memcmp(dir, s, dir_len) != 0) return false;

  return *(s + dir_len) == PEAR_FS_SEP[0];
}

static bool
check_addon_dir (pear_t *pear, const char *path, char *out) {
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
    pear_fs_path_join(path, result, out);
    return true;
  }

  return false;
}

static inline void
pear_addons_init () {
  pending_module = &pending_dynamic_module;
}

static inline int
pear_addons_resolve (pear_t *pear, const char *path, char *out) {
  char tmp[PEAR_FS_MAX_PATH];

  if (has_extension(path, ".pear") || has_extension(path, ".node")) {
    if (out != path) strcpy(out, path);
    return 0;
  }

  // TODO: check where cmake likes to build...

  // check for pearjs dev build compat
  pear_fs_path_join(path, "build", tmp);
  if (check_addon_dir(pear, tmp, out)) return 0;

  // node-gyp compat
  pear_fs_path_join(path, "build" PEAR_FS_SEP "Release", tmp);
  if (check_addon_dir(pear, tmp, out)) return 0;

  // check for prebuilds
  pear_fs_path_join(path, "prebuilds" PEAR_FS_SEP PEAR_PLATFORM "-" PEAR_ARCH, tmp);
  if (check_addon_dir(pear, tmp, out)) return 0;

  return UV_ENOENT;
}

static inline js_value_t *
pear_addons_load (pear_t *pear, const char *path, int mode) {
  int err;

  pear_module_t *mod = NULL;

  if (mode & PEAR_ADDONS_STATIC) {
    mod = pending_static_module;
    pear_module_t *prev = NULL;

    while (mod != NULL) {
      bool found = (mode & PEAR_ADDONS_RESOLVE) ? has_dirname(mod->filename, path) : (strcmp(mod->filename, path) == 0);

      if (found) {
        if (prev == NULL) {
          pending_static_module = mod->next_module;
        } else {
          prev->next_module = mod->next_module;
        }
        break;
      }

      prev = mod;
      mod = mod->next_module;
    }
  }

  if (mode & PEAR_ADDONS_DYNAMIC) {
    if (mode & PEAR_ADDONS_RESOLVE) {
      uv_loop_t *loop;
      js_get_env_loop(pear->env, &loop);
      err = pear_addons_resolve(pear, path, (char *) path);
      if (err < 0) {
        js_throw_errorf(pear->env, NULL, "Could not resolve addon %s", path);
        return NULL;
      }
    }

    uv_lib_t *lib = malloc(sizeof(uv_lib_t));
    err = uv_dlopen(path, lib);

    if (err < 0) {
      js_throw_error(pear->env, NULL, uv_dlerror(lib));
      free(lib);
      return NULL;
    }

    mod = shift_pending_dynamic_addon();

    if (mod == NULL) {
      uv_dlclose(lib);
      free(lib);
    }
  }

  if (mod == NULL) {
    js_throw_errorf(pear->env, NULL, "No module registered for %s", path);
    return NULL;
  }

  js_value_t *addon;
  js_create_object(pear->env, &addon);

  js_value_t *exports = mod->register_module(pear->env, addon);

  free(mod);

  if (exports != NULL) {
    addon = exports;
  }

  return addon;
}

void
pear_module_register (pear_module_t *mod) {
  pear_module_t *cpy = malloc(sizeof(pear_module_t));

  *cpy = *mod;

  cpy->next_module = *pending_module;
  *pending_module = cpy;
}

void
napi_module_register (napi_module *napi_mod) {
  pear_module_t mod = {
    .version = PEAR_MODULE_VERSION,
    .filename = napi_mod->nm_filename,
    .register_module = napi_mod->nm_register_func,
  };

  pear_module_register(&mod);
}

#endif // PEAR_ADDONS_H
