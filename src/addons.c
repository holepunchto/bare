#include <uv.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <napi.h>
#include <pearjs.h>
#include <js.h>

#include "addons.h"
#include "sync_fs.h"

#define PEARJS_ADDONS_MAX_ENTRIES 256

static pearjs_module_t *pearjs_pending_module = NULL;

static pearjs_module_t *
shift_pending_addon () {
  pearjs_module_t *mod = pearjs_pending_module;
  pearjs_module_t *prev = NULL;

  if (mod == NULL) return NULL;

  while (mod->next_addon != NULL) {
    prev = mod;
    mod = mod->next_addon;
  }

  if (prev == NULL) pearjs_pending_module = NULL;
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
check_addon_dir (uv_loop_t *loop, const char *path, char *out) {
  uv_dirent_t entries[PEARJS_ADDONS_MAX_ENTRIES];
  int entries_len = pearjs_sync_fs_readdir(loop, path, PEARJS_ADDONS_MAX_ENTRIES, (uv_dirent_t *) entries);
  if (entries_len <= 0) return false;

  const char *result = NULL;

  for (int i = 0; i < entries_len && result == NULL; i++) {
    const char *name = entries[i].name;
    if (has_extension(name, ".pear")) {
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
    pearjs_sync_fs_path_join(path, result, out);
    return true;
  }

  return false;
}

int
pearjs_addons_find (uv_loop_t *loop, const char *path, char *out) {
  char tmp[PEARJS_SYNC_FS_MAX_PATH];

  // TODO: check where cmake likes to build...

  // node-gyp compat
  pearjs_sync_fs_path_join(path, "build" PEARJS_SYNC_FS_SEP "Release", tmp);
  if (check_addon_dir(loop, tmp, out)) return 0;

  // check for prebuilds
  pearjs_sync_fs_path_join(path, "prebuilds" PEARJS_SYNC_FS_SEP PEARJS_PLATFORM "-" PEARJS_ARCH, tmp);
  if (check_addon_dir(loop, tmp, out)) return 0;

  return UV_ENOENT;
}

js_value_t *
pearjs_addons_load (js_env_t *env, const char *path) {
  uv_lib_t *lib = malloc(sizeof(uv_lib_t));
  int err = uv_dlopen(path, lib);

  if (err < 0) {
    fprintf(stderr, "Unable to open addon: %s, path=%s\n", uv_dlerror(lib), path);
    return NULL;
  }

  pearjs_module_t *mod = shift_pending_addon();

  if (mod == NULL) {
    uv_dlclose(lib);
    free(lib);
    fprintf(stderr, "No module registered, path=%s\n", path);
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
pearjs_module_register (pearjs_module_t *mod) {
  pearjs_module_t *cpy = malloc(sizeof(pearjs_module_t));

  *cpy = *mod;

  cpy->next_addon = pearjs_pending_module;
  pearjs_pending_module = cpy;
}

void
napi_module_register (napi_module *napi_mod) {
  pearjs_module_t mod = {
    .name = napi_mod->nm_modname,
    .register_addon = napi_mod->nm_register_func,
  };

  pearjs_module_register(&mod);
}
