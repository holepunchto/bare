#include <assert.h>
#include <js.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <uv.h>

#include "../include/pear.h"
#include "addons.h"
#include "runtime.h"

static void
on_idle (uv_idle_t *handle) {}

int
pear_setup (uv_loop_t *loop, pear_t *pear, int argc, char **argv) {
  pear_addons_init();

  pear->loop = loop;

  int err;

  err = js_create_platform(pear->loop, NULL, &pear->platform);
  assert(err == 0);

  err = js_create_env(pear->loop, pear->platform, &pear->env);
  assert(err == 0);

  pear->runtime.argc = argc;
  pear->runtime.argv = argv;

  err = pear_runtime_setup(pear);
  assert(err == 0);

  pear->suspended = false;

  err = uv_idle_init(loop, &pear->idle);
  assert(err == 0);
  pear->idle.data = (void *) pear;

  return 0;
}

int
pear_teardown (pear_t *pear, int *exit_code) {
  pear_runtime_teardown(pear, exit_code);

  js_destroy_env(pear->env);
  js_destroy_platform(pear->platform);

  uv_close((uv_handle_t *) &pear->idle, NULL);

  return 0;
}

int
pear_run (pear_t *pear, const char *filename, const uv_buf_t *source) {
  int err = pear_runtime_run(pear, filename, source);
  if (err < 0) return err;

  do {
    uv_run(pear->loop, UV_RUN_DEFAULT);

    pear_runtime_before_teardown(pear);
  } while (uv_loop_alive(pear->loop));

  return 0;
}

int
pear_suspend (pear_t *pear) {
  if (pear->suspended) return -1;
  pear->suspended = true;

  uv_idle_start(&pear->idle, on_idle);

  pear_runtime_suspend(pear);

  return 0;
}

int
pear_resume (pear_t *pear) {
  if (!pear->suspended) return -1;
  pear->suspended = false;

  uv_idle_stop(&pear->idle);

  pear_runtime_resume(pear);

  return 0;
}

int
pear_get_data (pear_t *pear, const char *key, void **result) {
  return pear_runtime_get_data(pear, key, result);
}

int
pear_set_data (pear_t *pear, const char *key, void *value) {
  return pear_runtime_set_data(pear, key, value);
}
