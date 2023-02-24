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

  pear_runtime_setup(pear);

  pear->suspended = false;

  err = uv_idle_init(loop, &pear->idle);
  assert(err == 0);
  pear->idle.data = (void *) pear;

  pear->on_before_exit = NULL;
  pear->on_exit = NULL;
  pear->on_suspend = NULL;
  pear->on_idle = NULL;
  pear->on_resume = NULL;

  return 0;
}

int
pear_teardown (pear_t *pear, int *exit_code) {
  int err;

  pear_runtime_on_exit(pear, exit_code);

  if (pear->on_exit) pear->on_exit(pear);

  err = js_destroy_env(pear->env);
  assert(err == 0);

  err = js_destroy_platform(pear->platform);
  assert(err == 0);

  uv_close((uv_handle_t *) &pear->idle, NULL);

  return 0;
}

int
pear_run (pear_t *pear, const char *filename, const uv_buf_t *source) {
  int err = pear_runtime_run(pear, filename, source);
  if (err < 0) return err;

  do {
    uv_run(pear->loop, UV_RUN_DEFAULT);

    if (pear->suspended) {
      uv_idle_start(&pear->idle, on_idle);

      pear_runtime_on_idle(pear);

      if (pear->on_idle) pear->on_idle(pear);
    } else {
      pear_runtime_on_before_exit(pear);

      if (pear->on_before_exit) pear->on_before_exit(pear);
    }
  } while (uv_loop_alive(pear->loop));

  return 0;
}

int
pear_suspend (pear_t *pear) {
  if (pear->suspended) return -1;
  pear->suspended = true;

  pear_runtime_on_suspend(pear);

  if (pear->on_suspend) pear->on_suspend(pear);

  return 0;
}

int
pear_resume (pear_t *pear) {
  if (!pear->suspended) return -1;
  pear->suspended = false;

  uv_idle_stop(&pear->idle);

  pear_runtime_on_resume(pear);

  if (pear->on_resume) pear->on_resume(pear);

  return 0;
}

int
pear_on_before_exit (pear_t *pear, pear_before_exit_cb cb) {
  pear->on_before_exit = cb;

  return 0;
}

int
pear_on_exit (pear_t *pear, pear_exit_cb cb) {
  pear->on_exit = cb;

  return 0;
}

int
pear_on_suspend (pear_t *pear, pear_suspend_cb cb) {
  pear->on_suspend = cb;

  return 0;
}

int
pear_on_idle (pear_t *pear, pear_idle_cb cb) {
  pear->on_idle = cb;

  return 0;
}

int
pear_on_resume (pear_t *pear, pear_resume_cb cb) {
  pear->on_resume = cb;

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
