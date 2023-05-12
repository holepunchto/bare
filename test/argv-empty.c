#include <assert.h>
#include <bare.h>
#include <uv.h>

int
main (int argc, char *argv[]) {
  argc = 0;
  argv = NULL;

  argv = uv_setup_args(argc, argv);

  int e;

  bare_t *bare;
  e = bare_setup(uv_default_loop(), argc, argv, &bare);
  assert(e == 0);

  int exit_code;
  e = bare_teardown(bare, &exit_code);
  assert(e == 0);

  return exit_code;
}
