#include <assert.h>
#include <pear.h>
#include <uv.h>

int
main (int argc, char *argv[]) {
  argc = 0;
  argv = NULL;

  argv = uv_setup_args(argc, argv);

  int e;

  pear_t *pear;
  e = pear_setup(uv_default_loop(), argc, argv, &pear);
  assert(e == 0);

  int exit_code;
  e = pear_teardown(pear, &exit_code);
  assert(e == 0);

  return exit_code;
}
