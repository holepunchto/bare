#include <assert.h>
#include <uv.h>

#include "../include/pear.h"
#include "pear.bundle.h"

void print_help () {
  printf("\U0001F350 Pear.js\n"
         "Small and modular JavaScript runtime for desktop and mobile.\n"
         "Usage:\n"
         "    pear [-m, --import-map <path>] <filename>\n");
}

int
main (int argc, char *argv[]) {
  int err;

  if (argc == 1) {
    print_help();
    return 1;
  }

  argv = uv_setup_args(argc, argv);

  pear_t pear;
  err = pear_setup(uv_default_loop(), &pear, argc, argv);
  assert(err == 0);

  uv_buf_t source = uv_buf_init((char *) bundle, bundle_len);

  err = pear_run(&pear, "/pear.bundle", &source);
  assert(err == 0);

  int exit_code;
  err = pear_teardown(&pear, &exit_code);
  assert(err == 0);

  return exit_code;
}
