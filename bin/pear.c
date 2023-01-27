#include <js.h>
#include <stdio.h>
#include <uv.h>

#include "../include/pear.h"
#include "../src/addons.h"
#include "../src/runtime.h"
#include "../src/sync_fs.h"

int
main (int argc, char **argv) {
  argv = uv_setup_args(argc, argv);

  if (argc < 2) {
    fprintf(stderr, "Usage: pear <filename>\n");
    return 1;
  }

  pear_t pear;
  pear_setup(&pear);

  char *entry_point = NULL;
  int err;

  err = pear_sync_fs_realpath(pear.loop, argv[1], NULL, &entry_point);

  if (err < 0) {
    fprintf(stderr, "Could not resolve entry point: %s\n", argv[1]);
    return 1;
  }

  pear_runtime_t config = {0};

  config.main = entry_point;
  config.argc = argc - 2;
  config.argv = argv + 2;

  err = pear_runtime_setup(&pear, &config);

  if (err < 0) {
    fprintf(stderr, "pear_runtime_setup failed with %i\n", err);
    return 1;
  }

  do {
    uv_run(pear.loop, UV_RUN_DEFAULT);
    pear_runtime_before_teardown(&pear, &config);
  } while (uv_loop_alive(pear.loop));

  int exit_code = 0;
  pear_runtime_teardown(&pear, &config, &exit_code);

  pear_teardown(&pear);

  return exit_code;
}
