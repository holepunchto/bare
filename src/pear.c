#include <assert.h>
#include <js.h>
#include <napi.h>
#include <pearjs.h>
#include <stdlib.h>
#include <uv.h>

#include "bootstrap.h"

static pearjs_module_t *pearjs_pending_module = NULL;

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

static js_value_t *
process_log (js_env_t *env, js_callback_info_t *info) {
  js_value_t *argv[1];
  size_t argc = 1;

  js_get_callback_info(env, info, &argc, argv, NULL, NULL);

  char data[65536];
  size_t data_len;

  js_get_value_string_utf8(env, argv[0], data, 65536, &data_len);

  printf("%s", data);

  return NULL;
}

typedef void (*addon_main)(js_env_t *env, js_value_t *addon);

static js_value_t *
load_addon (js_env_t *env, js_callback_info_t *info) {
  js_value_t *argv[2];
  size_t argc = 2;

  js_get_callback_info(env, info, &argc, argv, NULL, NULL);

  void *handle = NULL;
  addon_main bootstrap = NULL;

  char addon_file[4096];
  char addon_bootstrap[4096];

  js_get_value_string_utf8(env, argv[0], addon_file, 4096, NULL);
  js_get_value_string_utf8(env, argv[1], addon_bootstrap, 4096, NULL);

  uv_lib_t *lib = malloc(sizeof(uv_lib_t));
  int err = uv_dlopen(addon_file, lib);

  if (err < 0) {
    fprintf(stderr, "Unable to open addon: %s, path=%s\n", uv_dlerror(lib), addon_file);
    return NULL;
  }

  pearjs_module_t *mod = shift_pending_addon();

  if (mod == NULL) {
    uv_dlclose(lib);
    free(lib);
    fprintf(stderr, "No module registered, path=%s\n", addon_file);
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

static int
read_file_sync (uv_loop_t *loop, char *name, size_t *size, char **data) {
  uv_fs_t req;
  uv_fs_open(loop, &req, name, UV_FS_O_RDONLY, 0, NULL);

  int fd = req.result;
  uv_fs_req_cleanup(&req);

  if (fd < 0) return fd;

  uv_fs_fstat(loop, &req, fd, NULL);
  uv_stat_t *st = req.ptr;

  size_t len = st->st_size;
  char *base = malloc(len);

  uv_buf_t buf = {
    .base = base,
    .len = len,
  };

  uv_fs_req_cleanup(&req);

  int64_t read = 0;

  while (1) {
    uv_fs_read(loop, &req, fd, &buf, 1, read, NULL);

    int res = req.result;
    uv_fs_req_cleanup(&req);

    if (res < 0) {
      free(base);
      uv_fs_close(loop, &req, fd, NULL);
      uv_fs_req_cleanup(&req);
      return res;
    }

    buf.base += res;
    buf.len -= res;

    read += res;
    if (res == 0 || read == len) break;
  }

  uv_fs_close(loop, &req, fd, NULL);
  uv_fs_req_cleanup(&req);

  *data = base;
  *size = read;

  return 0;
}

static js_value_t *
exists_sync (js_env_t *env, js_callback_info_t *info) {
  js_value_t *argv[1];
  size_t argc = 1;

  js_get_callback_info(env, info, &argc, argv, NULL, NULL);

  char path[4096];

  js_get_value_string_utf8(env, argv[0], path, 4096, NULL);

  uv_loop_t *loop;
  js_get_env_loop(env, &loop);

  uv_fs_t req;
  uv_fs_stat(loop, &req, path, NULL);

  int type = 0;

  if (req.result >= 0) {
    uv_stat_t *st = req.ptr;

    if (st->st_mode & S_IFREG) {
      type = 1;
    } else if (st->st_mode & S_IFDIR) {
      type = 2;
    }
  }

  uv_fs_req_cleanup(&req);

  js_value_t *result;
  js_create_uint32(env, type, &result);

  return result;
}

static js_value_t *
read_source_sync (js_env_t *env, js_callback_info_t *info) {
  js_value_t *argv[1];
  size_t argc = 1;

  js_get_callback_info(env, info, &argc, argv, NULL, NULL);

  char path[4096];

  js_get_value_string_utf8(env, argv[0], path, 4096, NULL);

  uv_loop_t *loop;
  js_get_env_loop(env, &loop);

  size_t size;
  char *data;

  js_value_t *result;

  int err = read_file_sync(loop, path, &size, &data);
  if (err < 0) { // TODO: update when we can create errors
    js_get_null(env, &result);
  } else {
    js_create_string_utf8(env, data, size, &result);
    free(data);
  }

  return result;
}

// struct FastApiArrayBuffer_ {
//   void* data;
//   size_t byte_length;
// };

// void
// process_hrtime_fast (struct FastApiArrayBuffer_ arr, struct FastApiArrayBuffer_ prev) {
//   printf("yo\n");
//   uint32_t *d0 = prev.data;
//   uint32_t *d1 = arr.data;

//   uint64_t p = d0[0] * 1e9 + d0[1];
//   uint64_t now = uv_hrtime() - p;

//   *(d1++) = now / ((uint32_t) 1e9);
//   *d1 = now % ((uint32_t) 1e9);
// }

static js_value_t *
process_hrtime (js_env_t *env, js_callback_info_t *info) {
  js_value_t *argv[2];
  size_t argc = 2;

  js_get_callback_info(env, info, &argc, argv, NULL, NULL);

  size_t arr_len;
  uint32_t *arr;

  size_t prev_len;
  uint32_t *prev;

  js_get_typedarray_info(env, argv[0], NULL, &arr_len, (void **) &arr, NULL, NULL);
  js_get_typedarray_info(env, argv[1], NULL, &prev_len, (void **) &prev, NULL, NULL);

  if (arr_len < 2 || prev_len < 2) return NULL;

  uint64_t p = prev[0] * 1e9 + prev[1];
  uint64_t now = uv_hrtime() - p;

  arr[0] = now / ((uint32_t) 1e9);
  arr[1] = now % ((uint32_t) 1e9);

  return NULL;
}

static js_value_t *
process_exit (js_env_t *env, js_callback_info_t *info) {
  js_value_t *argv[1];
  size_t argc = 1;

  js_get_callback_info(env, info, &argc, argv, NULL, NULL);

  int32_t code;
  js_get_value_int32(env, argv[0], &code);

  exit(code);

  return NULL;
}

int
main (int argc, char **argv) {
  if (argc < 2) {
    printf("Usage: pear <filename>\n");
    return 1;
  }

  int err;
  uv_loop_t *loop = uv_default_loop();

  js_platform_options_t opts = {0};

  js_platform_t *platform;
  js_create_platform(loop, &opts, &platform);

  js_env_t *env;
  js_create_env(loop, platform, &env);

  js_value_t *proc;
  js_create_object(env, &proc);

  {
    js_value_t *val;
    js_create_function(env, "log", -1, process_log, NULL, &val);
    js_set_named_property(env, proc, "_log", val);
  }

  // {
  //   js_ffi_type_info_t *return_info;
  //   js_ffi_create_type_info(js_ffi_void, js_ffi_scalar, &return_info);

  //   js_ffi_type_info_t *arg_info[2];
  //   js_ffi_create_type_info(js_ffi_uint32, js_ffi_typedarray, &arg_info[0]);
  //   js_ffi_create_type_info(js_ffi_uint32, js_ffi_typedarray, &arg_info[1]);

  //   js_ffi_function_info_t *function_info;
  //   js_ffi_create_function_info(return_info, (const js_ffi_type_info_t **) arg_info, 2, &function_info);

  //   js_ffi_function_t *ffi;
  //   js_ffi_create_function(process_hrtime_fast, function_info, &ffi);

  //   js_value_t *val;

  //   js_create_function_with_ffi(env, "hrtime", -1, process_hrtime, NULL, ffi, &val);
  //   js_set_named_property(env, proc, "_hrtime", val);
  // }

  {
    js_value_t *val;
    js_create_function(env, "exit", -1, process_exit, NULL, &val);
    js_set_named_property(env, proc, "_exit", val);
  }

  {
    js_value_t *val;
    js_create_function(env, "loadAddon", -1, load_addon, NULL, &val);
    js_set_named_property(env, proc, "_loadAddon", val);
  }

  {
    js_value_t *val;
    js_create_function(env, "existsSync", -1, exists_sync, NULL, &val);
    js_set_named_property(env, proc, "_existsSync", val);
  }

  {
    js_value_t *val;
    js_create_function(env, "readSourceSync", -1, read_source_sync, NULL, &val);
    js_set_named_property(env, proc, "_readSourceSync", val);
  }

  {
    js_value_t *val;
    uv_fs_t req;
    uv_fs_realpath(loop, &req, argv[1], NULL);
    js_create_string_utf8(env, req.ptr, -1, &val);
    uv_fs_req_cleanup(&req);
    js_set_named_property(env, proc, "_filename", val);
  }

  js_value_t *global;
  js_get_global(env, &global);

  js_set_named_property(env, global, "process", proc);
  js_set_named_property(env, global, "global", global);

  js_value_t *script;
  js_create_string_utf8(env, (const char *) pearjs_bootstrap, pearjs_bootstrap_len, &script);

  js_value_t *result;
  js_run_script(env, script, &result);

  uv_run(loop, UV_RUN_DEFAULT);

  js_destroy_env(env);

  js_destroy_platform(platform);

  return 0;
}
