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

  char data[65536];
  size_t data_len;

  js_get_value_string_utf8(env, argv[0], data, 65536, &data_len);

  printf("%s", data);

  return NULL;
}

typedef void (*addon_main)(js_env_t *env, js_value_t *addon);

static js_value_t *
load_addon (js_env_t *env, const js_callback_info_t *info) {
  js_value_t *argv[2];
  size_t argc = 2;

  js_get_callback_info(env, info, &argc, argv, NULL, NULL);

  void *handle = NULL;
  addon_main bootstrap = NULL;

  char addon_file[4096];
  char addon_bootstrap[4096];

  js_get_value_string_utf8(env, argv[0], addon_file, 4096, NULL);
  js_get_value_string_utf8(env, argv[1], addon_bootstrap, 4096, NULL);

  handle = dlopen(addon_file, RTLD_NOW | RTLD_GLOBAL);

  if (handle == NULL) {
    fprintf(stderr, "Unable to open lib: %s\n", dlerror());
    return NULL;
  }

  bootstrap = dlsym(handle, addon_bootstrap);

  if (bootstrap == NULL) {
    fprintf(stderr, "Unable to get symbol\n");
    return NULL;
  }

  js_handle_scope_t *scope;
  js_open_handle_scope(env, &scope);

  js_value_t *addon;
  js_create_object(env, &addon);

  bootstrap(env, addon);

  js_close_handle_scope(env, scope);

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
    .len = len
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
exists_sync (js_env_t *env, const js_callback_info_t *info) {
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
read_source_sync (js_env_t *env, const js_callback_info_t *info) {
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

int
main (int argc, char **argv) {
  if (argc < 2) {
    printf("Usage: pear <filename>\n");
    return 1;
  }

  int err;
  uv_loop_t *loop = uv_default_loop();

  js_platform_t *platform;
  js_create_platform(loop, &platform);

  js_env_t *env;
  js_create_env(loop, platform, &env);

  js_handle_scope_t *scope;
  js_open_handle_scope(env, &scope);

  js_value_t *proc;
  js_create_object(env, &proc);

  {
    js_value_t *val;
    js_create_function(env, "log", -1, log, NULL, &val);
    js_set_named_property(env, proc, "_log", val);
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

  size_t size;
  char *data;

  err = read_file_sync(loop, "./bootstrap.js", &size, &data);
  if (err < 0) {
    printf("Could not load bootstrap.js\n");
    return err;
  }

  js_value_t *script;
  js_create_string_utf8(env, data, size, &script);
  free(data);

  js_value_t *result;
  js_run_script(env, script, &result);

  js_close_handle_scope(env, scope);

  uv_run(loop, UV_RUN_DEFAULT);

  js_destroy_env(env);

  js_destroy_platform(platform);

  return 0;
}
