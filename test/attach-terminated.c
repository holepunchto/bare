#include <assert.h>
#include <bare.h>
#include <js.h>
#include <string.h>
#include <uv.h>

// A process that has terminated may no longer be called into, so attaching
// it is refused.

static const char *code = "";

int
main(int argc, char *argv[]) {
  int e;

  argc = 0;
  argv = NULL;

  argv = uv_setup_args(argc, argv);

  js_platform_t *platform;
  e = js_create_platform(uv_default_loop(), NULL, &platform);
  assert(e == 0);

  bare_t *bare;
  e = bare_setup(uv_default_loop(), platform, NULL, argc, (const char **) argv, NULL, &bare);
  assert(e == 0);

  uv_buf_t source = uv_buf_init((char *) code, strlen(code));

  e = bare_load(bare, "/test.js", &source, NULL);
  assert(e == 0);

  bare_t *previous;
  e = bare_attach(bare, &previous);
  assert(e == 0);

  e = bare_detach(bare, previous);
  assert(e == 0);

  e = bare_terminate(bare);
  assert(e == 0);

  e = bare_run(bare, UV_RUN_DEFAULT);
  assert(e == 0);

  assert(bare_attach(bare, &previous) == -1);

  int exit_code = 0;

  e = bare_teardown(bare, UV_RUN_DEFAULT, &exit_code);
  assert(e == 0);

  e = js_destroy_platform(platform);
  assert(e == 0);

  e = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
  assert(e == 0);

  return 0;
}
