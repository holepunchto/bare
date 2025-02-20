#include <assert.h>
#include <log.h>
#include <uv.h>

#include "../include/bare.h"
#include "bare.bundle.h"

static uv_sem_t bare__platform_ready;
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

  err = js_create_platform(&loop, NULL, &bare__platform);
  assert(err == 0);

  uv_sem_post(&bare__platform_ready);

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

  err = log_open("bare", 0);
  assert(err == 0);

  uv_loop_t *loop = uv_default_loop();

  argv = uv_setup_args(argc, argv);

  err = uv_sem_init(&bare__platform_ready, 0);
  assert(err == 0);

  uv_thread_t thread;
  err = uv_thread_create(&thread, bare__on_platform_thread, NULL);
  assert(err == 0);

  uv_sem_wait(&bare__platform_ready);

  uv_sem_destroy(&bare__platform_ready);

  bare_t *bare;
  err = bare_setup(loop, bare__platform, NULL, argc, (const char **) argv, NULL, &bare);
  assert(err == 0);

  uv_buf_t source = uv_buf_init((char *) bare_bundle, bare_bundle_len);

  bare_load(bare, "bare:/bare.bundle", &source, NULL);

  err = bare_run(bare);
  assert(err == 0);

  int exit_code;
  err = bare_teardown(bare, &exit_code);
  assert(err == 0);

  err = uv_loop_close(loop);
  assert(err == 0);

  err = uv_async_send(&bare__platform_shutdown);
  assert(err == 0);

  uv_thread_join(&thread);

  err = log_close();
  assert(err == 0);

  return exit_code;
}
