#include <assert.h>
#include <bare.h>
#include <js.h>
#include <string.h>
#include <uv.h>

// An embedder may stop the loop to regain control of the thread while the
// process still has work left, which must make the run return early rather
// than carry on until the process exits.

static const char *code =
  "setTimeout(() => {}, 100)\n";

static void
on_timer(uv_timer_t *handle) {
  uv_stop(handle->loop);
}

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

  uv_timer_t timer;
  e = uv_timer_init(uv_default_loop(), &timer);
  assert(e == 0);

  e = uv_timer_start(&timer, on_timer, 10, 0);
  assert(e == 0);

  e = bare_run(bare, UV_RUN_DEFAULT);
  assert(e > 0);

  uv_close((uv_handle_t *) &timer, NULL);

  e = bare_run(bare, UV_RUN_DEFAULT);
  assert(e == 0);

  int exit_code = 0;

  e = bare_teardown(bare, UV_RUN_DEFAULT, &exit_code);
  assert(e == 0);
  assert(exit_code == 0);

  e = js_destroy_platform(platform);
  assert(e == 0);

  e = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
  assert(e == 0);

  return 0;
}
