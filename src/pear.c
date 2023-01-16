#include <assert.h>
#include <js.h>
#include <pear.h>
#include <stdlib.h>
#include <uv.h>
#include <string.h>

#include "runtime.h"
#include "addons.h"
#include "sync_fs.h"

int
main (int argc, char **argv) {
  pear_addons_init();

  if (argc < 2) {
    fprintf(stderr, "Usage: pear <filename>\n");
    return 1;
  }

  char *entry_point = NULL;
  int err;

  uv_loop_t *loop = uv_default_loop();

  err = pear_sync_fs_realpath(loop, argv[1], NULL, &entry_point);

  if (err < 0) {
    fprintf(stderr, "Could not resolve entry point: %s\n", argv[1]);
    return 1;
  }

  js_platform_options_t opts = {0};

  js_platform_t *platform;
  js_create_platform(loop, &opts, &platform);

  js_env_t *env;
  js_create_env(loop, platform, &env);

  pear_runtime_t config = {0};

  config.main = entry_point;
  config.argc = argc - 2;
  config.argv = argv + 2;

  err = pear_runtime_setup(env, &config);

  if (err < 0) {
    fprintf(stderr, "pear_runtime_setup failed with %i\n", err);
    return 1;
  }

  uv_run(loop, UV_RUN_DEFAULT);

  pear_runtime_teardown(env, &config);

  js_destroy_env(env);

  js_destroy_platform(platform);

  return 0;
}
