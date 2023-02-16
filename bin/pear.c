#include <js.h>
#include <stdio.h>
#include <uv.h>

#include "../include/pear.h"
#include "../src/fs.h"

int
main (int argc, char **argv) {
  uv_loop_t *loop = uv_default_loop();

  argv = uv_setup_args(argc, argv);

  if (argc < 2) {
    fprintf(stderr, "Usage: pear <filename>\n");
    return 1;
  }

  pear_t pear;
  pear_setup(loop, &pear, argc, argv);

  char *entry_point = NULL;

  int err = pear_fs_realpath_sync(&pear, argv[1], NULL, &entry_point);

  if (err < 0) {
    fprintf(stderr, "Could not resolve entry point: %s\n", argv[1]);
    return 1;
  }

  argv[1] = entry_point;

  pear_run(&pear, entry_point, NULL);

  int exit_code;
  pear_teardown(&pear, &exit_code);

  return exit_code;
}
