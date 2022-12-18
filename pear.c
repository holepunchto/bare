#include <js.h>
#include <uv.h>
#include <stdlib.h>
#include <assert.h>

#include <dlfcn.h>

static js_value_t *
log (js_env_t *env, const js_callback_info_t *info) {
  js_value_t *argv[1];
  size_t argc = 1;

  js_get_callback_info(env, info, &argc, argv, NULL, NULL);

  char *data;
  size_t data_len;

  js_get_typedarray_info(env, argv[0], NULL, &data_len, (void **) &data, NULL, NULL);

  printf("%s\n", data);

  return NULL;
}

typedef void (*addon_main)(js_env_t *env, js_value_t *addon);

static js_value_t *
load_file (js_env_t *env, const js_callback_info_t *info) {
  return NULL;
}

static js_value_t *
load_addon (js_env_t *env, const js_callback_info_t *info) {
  js_value_t *argv[2];
  size_t argc = 2;

  js_get_callback_info(env, info, &argc, argv, NULL, NULL);

  void *handle = NULL;
  addon_main func = NULL;

  char *addon_file;
  size_t addon_file_len;

  char *addon_bootstrap;
  size_t addon_bootstrap_len;

  js_get_typedarray_info(env, argv[0], NULL, &addon_file_len, (void **) &addon_file, NULL, NULL);
  js_get_typedarray_info(env, argv[1], NULL, &addon_bootstrap_len, (void **) &addon_bootstrap, NULL, NULL);

  handle = dlopen(addon_file, RTLD_NOW | RTLD_GLOBAL);

  if (handle == NULL) {
    fprintf(stderr, "Unable to open lib: %s\n", dlerror());
    return NULL;
  }

  func = dlsym(handle, addon_bootstrap);

  if (func == NULL) {
    fprintf(stderr, "Unable to get symbol\n");
    return NULL;
  }

  js_handle_scope_t *scope;
  js_open_handle_scope(env, &scope);

  js_value_t *addon;
  js_create_object(env, &addon);

  func(env, addon);

  js_close_handle_scope(env, scope);

  return addon;
}

int
main () {
  int e;

  uv_loop_t *loop = uv_default_loop();

  js_platform_t *platform;
  js_create_platform(loop, &platform);

  js_env_t *env;
  js_create_env(loop, platform, &env);

  js_handle_scope_t *scope;
  js_open_handle_scope(env, &scope);

  js_value_t *proc;
  js_create_object(env, &proc);

  js_value_t *fn_print;
  js_create_function(env, "print", -1, log, NULL, &fn_print);

  js_value_t *fn_load_addon;
  js_create_function(env, "loadAddon", -1, load_addon, NULL, &fn_load_addon);

  js_value_t *global;
  js_get_global(env, &global);

  js_set_named_property(env, proc, "_print", fn_print);
  js_set_named_property(env, proc, "_loadAddon", fn_load_addon);

  js_set_named_property(env, global, "process", proc);

  uv_fs_t req;
  uv_fs_open(loop, &req, "./index.js", UV_FS_O_RDONLY, 0, NULL);

  int fd = req.result;
  uv_fs_req_cleanup(&req);

  char data[65536];

  uv_buf_t buf = {
    .base = (char *) &data,
    .len = 65536
  };

  uv_fs_read(loop, &req, fd, &buf, 1, 0, NULL);

  int read = req.result;
  uv_fs_req_cleanup(&req);

  js_value_t *script;
  js_create_string_utf8(env, buf.base, read, &script);

  js_value_t *result;
  js_run_script(env, script, &result);

  js_close_handle_scope(env, scope);

  uv_run(loop, UV_RUN_DEFAULT);

  e = js_destroy_env(env);
  assert(e == 0);

  e = js_destroy_platform(platform);
  assert(e == 0);
}
