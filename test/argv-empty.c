#include <assert.h>
#include <bare.h>
#include <js.h>
#include <uv.h>

int
main (int argc, char *argv[]) {
  int e;

  argc = 0;
  argv = NULL;

  argv = uv_setup_args(argc, argv);

  js_platform_t *platform;
  e = js_create_platform(uv_default_loop(), NULL, &platform);
  assert(e == 0);

  bare_t *bare;
  e = bare_setup(uv_default_loop(), platform, NULL, argc, argv, NULL, &bare);
  assert(e == 0);

  int exit_code;
  e = bare_teardown(bare, &exit_code);
  assert(e == 0);

  e = js_destroy_platform(platform);
  assert(e == 0);

  e = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
  assert(e == 0);

  return exit_code;
}
