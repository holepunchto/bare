#include <assert.h>
#include <bare.h>
#include <js.h>
#include <uv.h>

// A thread attached to no process has nothing to resolve a handle against.
// Attaching gives a call made outside of `bare_run()` a process, and a nested
// attachment restores the outer one rather than clearing it.

static int bare_test__value = 42;

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

  e = bare_context_set(bare, "bare.test.value.v1", &bare_test__value, NULL);
  assert(e == 0);

  void *observed = NULL;

  assert(bare_context_get("bare.test.value.v1", &observed) == -2);
  assert(observed == NULL);

  bare_t *previous;
  e = bare_attach(bare, &previous);
  assert(e == 0);

  e = bare_context_get("bare.test.value.v1", &observed);
  assert(e == 0);
  assert(observed == (void *) &bare_test__value);

  bare_t *nested;
  e = bare_attach(bare, &nested);
  assert(e == 0);

  e = bare_detach(bare, nested);
  assert(e == 0);

  assert(bare_context_get("bare.test.value.v1", NULL) == 0);

  e = bare_detach(bare, previous);
  assert(e == 0);

  observed = NULL;

  assert(bare_context_get("bare.test.value.v1", &observed) == -2);
  assert(observed == NULL);

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
