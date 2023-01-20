#include <uv.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <napi.h>
#include <pear.h>
#include <js.h>

#include "addons.h"
#include "sync_fs.h"

#define PEAR_ADDONS_MAX_ENTRIES 256

static pear_module_t *pending_dynamic_module = NULL;
static pear_module_t *pending_static_module = NULL;
static pear_module_t **pending_module = &pending_static_module;

static pear_module_t *
shift_pending_dynamic_addon () {
  pear_module_t *mod = pending_dynamic_module;
  pear_module_t *prev = NULL;

  if (mod == NULL) return NULL;

  while (mod->next_addon != NULL) {
    prev = mod;
    mod = mod->next_addon;
  }

  if (prev == NULL) pending_dynamic_module = NULL;
  else prev->next_addon = mod->next_addon;

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

  return *(s + dir_len) == PEAR_SYNC_FS_SEP[0];
}

static bool
check_addon_dir (uv_loop_t *loop, const char *path, char *out) {
  uv_dirent_t entries[PEAR_ADDONS_MAX_ENTRIES];
  int entries_len = pear_sync_fs_readdir(loop, path, PEAR_ADDONS_MAX_ENTRIES, (uv_dirent_t *) entries);
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
    pear_sync_fs_path_join(path, result, out);
    return true;
  }

  return false;
}

void
pear_addons_init () {
  pending_module = &pending_dynamic_module;
}

int
pear_addons_resolve (uv_loop_t *loop, const char *path, char *out) {
  char tmp[PEAR_SYNC_FS_MAX_PATH];

  if (has_extension(path, ".pear") || has_extension(path, ".node")) {
    if (out != path) strcpy(out, path);
    return 0;
  }

  // TODO: check where cmake likes to build...

  // check for pearjs dev build compat
  pear_sync_fs_path_join(path, "build", tmp);
  if (check_addon_dir(loop, tmp, out)) return 0;

  // node-gyp compat
  pear_sync_fs_path_join(path, "build" PEAR_SYNC_FS_SEP "Release", tmp);
  if (check_addon_dir(loop, tmp, out)) return 0;

  // check for prebuilds
  pear_sync_fs_path_join(path, "prebuilds" PEAR_SYNC_FS_SEP PEAR_PLATFORM "-" PEAR_ARCH, tmp);
  if (check_addon_dir(loop, tmp, out)) return 0;

  return UV_ENOENT;
}

js_value_t *
pear_addons_load (js_env_t *env, const char *path, int mode) {
  int err;

  pear_module_t *mod = NULL;

  if (mode & PEAR_ADDONS_STATIC) {
    mod = pending_static_module;
    pear_module_t *prev = NULL;

    while (mod != NULL) {
      bool found = (mode & PEAR_ADDONS_RESOLVE) ? has_dirname(mod->filename, path) : (strcmp(mod->filename, path) == 0);

      if (found) {
        if (prev == NULL) {
          pending_static_module = mod->next_addon;
        } else {
          prev->next_addon = mod->next_addon;
        }
        break;
      }

      prev = mod;
      mod = mod->next_addon;
    }
  }

  if (mode & PEAR_ADDONS_DYNAMIC) {
    if (mode & PEAR_ADDONS_RESOLVE) {
      uv_loop_t *loop;
      js_get_env_loop(env, &loop);
      err = pear_addons_resolve(loop, path, (char *) path);
      if (err < 0) {
        js_throw_error(env, NULL, "Could not resolve addon");
        return NULL;
      }
    }

    uv_lib_t *lib = malloc(sizeof(uv_lib_t));
    err = uv_dlopen(path, lib);

    if (err < 0) {
      js_throw_error(env, NULL, uv_dlerror(lib));
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
    js_throw_error(env, NULL, "No module registered");
    return NULL;
  }

  js_value_t *addon;
  js_create_object(env, &addon);

  js_value_t *exports = mod->register_addon(env, addon);

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

  cpy->next_addon = *pending_module;
  *pending_module = cpy;
}

void
napi_module_register (napi_module *napi_mod) {
  pear_module_t mod = {
    .version = PEAR_MODULE_VERSION,
    .filename = napi_mod->nm_filename,
    .register_addon = napi_mod->nm_register_func
  };

  pear_module_register(&mod);
}
