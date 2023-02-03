#include <assert.h>
#include <js.h>
#include <stdbool.h>
#include <stddef.h>
#include <uv.h>

#include "../include/pear.h"
#include "addons.h"
#include "runtime.h"

static void
on_suspend (uv_async_t *handle) {
  pear_t *pear = (pear_t *) handle->data;

  if (pear->suspended) return;
  pear->suspended = true;

  uv_ref((uv_handle_t *) &pear->suspend);

  pear_runtime_suspend(pear);
}

static void
on_resume (uv_async_t *handle) {
  pear_t *pear = (pear_t *) handle->data;

  if (!pear->suspended) return;
  pear->suspended = false;

  uv_unref((uv_handle_t *) &pear->suspend);

  pear_runtime_resume(pear);
}

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

  err = uv_async_init(loop, &pear->suspend, on_suspend);
  assert(err == 0);
  pear->suspend.data = (void *) pear;

  err = uv_async_init(loop, &pear->resume, on_resume);
  assert(err == 0);
  pear->resume.data = (void *) pear;

  uv_unref((uv_handle_t *) &pear->suspend);
  uv_unref((uv_handle_t *) &pear->resume);

  return 0;
}

int
pear_teardown (pear_t *pear, int *exit_code) {
  pear_runtime_teardown(pear, exit_code);

  js_destroy_env(pear->env);
  js_destroy_platform(pear->platform);

  uv_close((uv_handle_t *) &pear->suspend, NULL);
  uv_close((uv_handle_t *) &pear->resume, NULL);

  return 0;
}

int
pear_run (pear_t *pear, const char *filename, const char *source, size_t len) {
  int err = pear_runtime_bootstrap(pear, filename, source, len);
  if (err < 0) return err;

  do {
    uv_run(pear->loop, UV_RUN_DEFAULT);

    pear_runtime_before_teardown(pear);
  } while (uv_loop_alive(pear->loop));

  return 0;
}

int
pear_suspend (pear_t *pear) {
  return uv_async_send(&pear->suspend);
}

int
pear_resume (pear_t *pear) {
  return uv_async_send(&pear->resume);
}
