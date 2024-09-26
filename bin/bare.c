#include <assert.h>
#include <log.h>
#include <uv.h>

#include "../include/bare.h"
#include "bare.bundle.h"

int
main (int argc, char *argv[]) {
  int err;

  err = log_open("bare", 0);
  assert(err == 0);

  argv = uv_setup_args(argc, argv);

  js_platform_t *platform;
  err = js_create_platform(uv_default_loop(), NULL, &platform);
  assert(err == 0);

  bare_t *bare;
  err = bare_setup(uv_default_loop(), platform, NULL, argc, (const char **) argv, NULL, &bare);
  assert(err == 0);

  uv_buf_t source = uv_buf_init((char *) bare_bundle, bare_bundle_len);

  bare_load(bare, "bare:/bare.bundle", &source, NULL);

  err = bare_run(bare);
  assert(err == 0);

  int exit_code;
  err = bare_teardown(bare, &exit_code);
  assert(err == 0);

  err = js_destroy_platform(platform);
  assert(err == 0);

  err = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
  assert(err == 0);

  err = uv_loop_close(uv_default_loop());
  assert(err == 0);

  err = log_close();
  assert(err == 0);

  return exit_code;
}
