#include <assert.h>
#include <uv.h>

#include "../include/bare.h"
#include "bare.bundle.h"

int
main (int argc, char *argv[]) {
  int err;

  argv = uv_setup_args(argc, argv);

  bare_t *bare;
  err = bare_setup(uv_default_loop(), argc, argv, NULL, &bare);
  assert(err == 0);

  uv_buf_t source = uv_buf_init((char *) bare_bin, bare_bin_len);

  err = bare_run(bare, "/bare.bundle", &source);
  assert(err == 0);

  int exit_code;
  err = bare_teardown(bare, &exit_code);
  assert(err == 0);

  return exit_code;
}
