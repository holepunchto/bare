#include <js.h>
#include <stdio.h>
#include <uv.h>

#include "../include/pear.h"
#include "../src/sync_fs.h"

int
main (int argc, char **argv) {
  uv_loop_t *loop = uv_default_loop();

  argv = uv_setup_args(argc, argv);

  if (argc < 2) {
    fprintf(stderr, "Usage: pear <filename>\n");
    return 1;
  }

  char *entry_point = NULL;
  int err;

  err = pear_sync_fs_realpath(loop, argv[1], NULL, &entry_point);

  if (err < 0) {
    fprintf(stderr, "Could not resolve entry point: %s\n", argv[1]);
    return 1;
  }

  argv[1] = entry_point;

  pear_t pear;
  pear_setup(loop, &pear, argc, argv);

  pear_run_file(&pear, entry_point);

  int exit_code = 0;

  pear_teardown(&pear, &exit_code);

  return exit_code;
}
