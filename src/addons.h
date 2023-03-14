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

#define PEAR_ADDONS_MAX_ENTRIES 256

typedef struct pear_module_list_s pear_module_list_t;

struct pear_module_list_s {
  pear_module_t mod;
  pear_module_list_t *next;
};

static pear_module_list_t *static_modules = NULL;
static pear_module_list_t *dynamic_modules = NULL;

static pear_module_list_t **pending_module = &static_modules;

static inline void
pear_addons_init () {
  pending_module = &dynamic_modules;
}

static bool
pear_addons_has_extension (const char *s, const char *ext) {
  size_t s_len = strlen(s);
  size_t e_len = strlen(ext);

  return e_len <= s_len && strcmp(s + (s_len - e_len), ext) == 0;
}

static inline int
pear_addons_readdir (pear_t *pear, const char *dirname, int entries_len, uv_dirent_t *entries) {
  uv_fs_t req;

  int num = 0;

  int err = uv_fs_opendir(pear->loop, &req, dirname, NULL);
  if (err < 0) {
    uv_fs_req_cleanup(&req);
    return err;
  }

  uv_dir_t *dir = (uv_dir_t *) req.ptr;
  uv_fs_req_cleanup(&req);

  dir->dirents = entries;
  dir->nentries = entries_len;

  num = uv_fs_readdir(pear->loop, &req, dir, NULL);
  if (num < 0) {
    uv_fs_req_cleanup(&req);
    return num;
  }

  err = uv_fs_closedir(pear->loop, &req, dir, NULL);
  if (err < 0) {
    uv_fs_req_cleanup(&req);
    return err;
  }

  return num;
}

static bool
pear_addons_check_dir (pear_t *pear, const char *path, char *out, size_t *len) {
  uv_dirent_t entries[PEAR_ADDONS_MAX_ENTRIES];

  int entries_len = pear_addons_readdir(pear, path, PEAR_ADDONS_MAX_ENTRIES, (uv_dirent_t *) entries);
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
    if (pear_addons_has_extension(name, ".pear")) {
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
    if (pear_addons_has_extension(name, ".node")) {
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
pear_addons_resolve (pear_t *pear, const char *path, char *out, size_t *len) {
  size_t tmp_len = PATH_MAX;
  char tmp[PATH_MAX];

  if (pear_addons_has_extension(path, ".pear") || pear_addons_has_extension(path, ".node")) {
    if (out != path) strcpy(out, path);
    return 0;
  }

  path_join((const char *[]){path, "build", NULL}, tmp, &tmp_len, path_behavior_system);

  if (pear_addons_check_dir(pear, tmp, out, len)) return 0;

  tmp_len = PATH_MAX;

  path_join((const char *[]){path, "build", "Release", NULL}, tmp, &tmp_len, path_behavior_system);

  if (pear_addons_check_dir(pear, tmp, out, len)) return 0;

  tmp_len = PATH_MAX;

  path_join((const char *[]){path, "build", "Debug", NULL}, tmp, &tmp_len, path_behavior_system);

  if (pear_addons_check_dir(pear, tmp, out, len)) return 0;

  tmp_len = PATH_MAX;

  path_join((const char *[]){path, "prebuilds", PEAR_TARGET, NULL}, tmp, &tmp_len, path_behavior_system);

  if (pear_addons_check_dir(pear, tmp, out, len)) return 0;

  return UV_ENOENT;
}

static inline js_value_t *
pear_addons_load (pear_t *pear, const char *path) {
  int err;

  pear_module_t *mod = NULL;
  pear_module_list_t *next = static_modules;

  while (next) {
    if (pear_addons_has_extension(path, next->mod.filename)) {
      mod = &next->mod;
      break;
    }

    next = next->next;
  }

  if (mod == NULL) {
    size_t resolved_len = PATH_MAX;
    char resolved[PATH_MAX];

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

  if (mod->version != PEAR_MODULE_VERSION) {
    js_throw_errorf(pear->env, NULL, "Unsupported ABI version %d for module %s", mod->version, path);
    return NULL;
  }

  js_value_t *exports;
  err = js_create_object(pear->env, &exports);
  assert(err == 0);

  return mod->init(pear->env, exports);
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
