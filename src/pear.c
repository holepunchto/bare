#include <assert.h>
#include <js.h>
#include <pearjs.h>
#include <stdlib.h>
#include <uv.h>
#include <string.h>

#include "runtime.h"
#include "../build/bootstrap.h"

int
main (int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: pear <filename>\n");
    return 1;
  }

  char *entry_point = NULL;
  int err;

  uv_loop_t *loop = uv_default_loop();

  uv_fs_t req;
  uv_fs_realpath(loop, &req, argv[1], NULL);

  if (req.result < 0) {
    fprintf(stderr, "Could not resolve entry point: %s\n", argv[1]);
    return 1;
  }

  entry_point = (char *) malloc(strlen(req.ptr) + 1);
  strcpy(entry_point, req.ptr);
  uv_fs_req_cleanup(&req);

  js_platform_options_t opts = {0};

  js_platform_t *platform;
  js_create_platform(loop, &opts, &platform);

  js_env_t *env;
  js_create_env(loop, platform, &env);

  err = pearjs_runtime_setup(env, entry_point);

  free(entry_point);
  entry_point = NULL;

  if (err < 0) {
    fprintf(stderr, "pearjs_runtime_setup failed with %i\n", err);
    return 1;
  }

  js_value_t *script;
  js_create_string_utf8(env, (const char *) pearjs_bootstrap, pearjs_bootstrap_len, &script);

  js_value_t *result;
  err = js_run_script(env, script, &result);

  if (err < 0) {
    js_value_t *exception;
    js_get_and_clear_last_exception(env, &exception);
    js_fatal_exception(env, exception);
  }

  uv_run(loop, UV_RUN_DEFAULT);

  pearjs_runtime_teardown(env);

  js_destroy_env(env);

  js_destroy_platform(platform);

  return 0;
}
