#include <assert.h>
#include <bare.h>
#include <js.h>
#include <uv.h>

// Several Bare processes may share one operating system process. Attaching
// restores what was attached before rather than clearing it, and detaching
// out of order is refused.

static int bare_test__values[2] = {1, 2};

int
main(int argc, char *argv[]) {
  int e;

  argc = 0;
  argv = NULL;

  argv = uv_setup_args(argc, argv);

  js_platform_t *platform;
  e = js_create_platform(uv_default_loop(), NULL, &platform);
  assert(e == 0);

  uv_loop_t loop;
  e = uv_loop_init(&loop);
  assert(e == 0);

  bare_t *x;
  e = bare_setup(uv_default_loop(), platform, NULL, argc, (const char **) argv, NULL, &x);
  assert(e == 0);

  bare_t *y;
  e = bare_setup(&loop, platform, NULL, argc, (const char **) argv, NULL, &y);
  assert(e == 0);

  e = bare_context_set(x, "bare.test.value.v1", &bare_test__values[0], NULL);
  assert(e == 0);

  e = bare_context_set(y, "bare.test.value.v1", &bare_test__values[1], NULL);
  assert(e == 0);

  void *observed = NULL;

  bare_t *outer;
  e = bare_attach(x, &outer);
  assert(e == 0);

  e = bare_context_get("bare.test.value.v1", &observed);
  assert(e == 0);
  assert(observed == (void *) &bare_test__values[0]);

  bare_t *inner;
  e = bare_attach(y, &inner);
  assert(e == 0);

  e = bare_context_get("bare.test.value.v1", &observed);
  assert(e == 0);
  assert(observed == (void *) &bare_test__values[1]);

  // The outer process is not the one the thread is attached to.
  assert(bare_detach(x, outer) == -1);

  e = bare_context_get("bare.test.value.v1", &observed);
  assert(e == 0);
  assert(observed == (void *) &bare_test__values[1]);

  e = bare_detach(y, inner);
  assert(e == 0);

  e = bare_context_get("bare.test.value.v1", &observed);
  assert(e == 0);
  assert(observed == (void *) &bare_test__values[0]);

  e = bare_detach(x, outer);
  assert(e == 0);

  assert(bare_context_get("bare.test.value.v1", NULL) == -2);

  int exit_code = 0;

  e = bare_teardown(x, UV_RUN_DEFAULT, &exit_code);
  assert(e == 0);
  assert(exit_code == 0);

  e = bare_teardown(y, UV_RUN_DEFAULT, &exit_code);
  assert(e == 0);
  assert(exit_code == 0);

  e = js_destroy_platform(platform);
  assert(e == 0);

  e = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
  assert(e == 0);

  e = uv_loop_close(&loop);
  assert(e == 0);

  return 0;
}
