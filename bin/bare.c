#include <assert.h>
#include <js.h>
#include <rlimit.h>
#include <signal.h>
#include <string.h>
#include <uv.h>

#include "../include/bare.h"

#include "bare.bundle.h"

static uv_barrier_t bare__platform_ready;
static uv_async_t bare__platform_shutdown;
static js_platform_t *bare__platform;

static void
bare__on_platform_shutdown(uv_async_t *handle) {
  uv_close((uv_handle_t *) handle, NULL);
}

static void
bare__on_platform_thread(void *data) {
  int err;

  uv_loop_t loop;
  err = uv_loop_init(&loop);
  assert(err == 0);

  err = uv_async_init(&loop, &bare__platform_shutdown, bare__on_platform_shutdown);
  assert(err == 0);

  js_platform_options_t *options = (js_platform_options_t *) data;

  err = js_create_platform(&loop, options, &bare__platform);
  assert(err == 0);

  uv_barrier_wait(&bare__platform_ready);

  err = uv_run(&loop, UV_RUN_DEFAULT);
  assert(err == 0);

  err = js_destroy_platform(bare__platform);
  assert(err == 0);

  err = uv_run(&loop, UV_RUN_DEFAULT);
  assert(err == 0);

  err = uv_loop_close(&loop);
  assert(err == 0);
}

int
main(int argc, char *argv[]) {
  int err;

#ifdef SIGPIPE
  signal(SIGPIPE, SIG_IGN);
#endif

  err = rlimit_set(rlimit_open_files, rlimit_infer);
  assert(err == 0);

  uv_disable_stdio_inheritance();

  argv = uv_setup_args(argc, argv);

  err = uv_barrier_init(&bare__platform_ready, 2);
  assert(err == 0);

  js_platform_options_t options = {};

  for (int i = 1; i < argc; i++) {
    if (memcmp(argv[i], "--expose-gc", 11) == 0) {
      options.expose_garbage_collection = true;
    }
  }

  uv_thread_t thread;
  err = uv_thread_create(&thread, bare__on_platform_thread, (void *) &options);
  assert(err == 0);

  uv_barrier_wait(&bare__platform_ready);

  uv_barrier_destroy(&bare__platform_ready);

  uv_loop_t *loop = uv_default_loop();

  bare_t *bare;
  err = bare_setup(loop, bare__platform, NULL, argc, (const char **) argv, NULL, &bare);
  assert(err == 0);

  uv_buf_t source = uv_buf_init((char *) bare_bundle, bare_bundle_len);

  err = bare_load(bare, "bare:/bare.bundle", &source, NULL);
  (void) err;

  err = bare_run(bare, UV_RUN_DEFAULT);
  assert(err == 0);

  int exit_code;
  err = bare_teardown(bare, UV_RUN_DEFAULT, &exit_code);
  assert(err == 0);

  err = uv_loop_close(loop);
  assert(err == 0);

  err = uv_async_send(&bare__platform_shutdown);
  assert(err == 0);

  uv_thread_join(&thread);

  return exit_code;
}
