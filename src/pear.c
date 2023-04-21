#include <assert.h>
#include <js.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <uv.h>

#include "../include/pear.h"
#include "addons.h"
#include "runtime.h"
#include "types.h"

static void
on_prepare (uv_prepare_t *handle) {}

int
pear_setup (uv_loop_t *loop, int argc, char **argv, pear_t **result) {
  pear_t *pear = malloc(sizeof(pear_t));

  pear_addons_init();

  pear->runtime.loop = loop;

  int err;

  err = js_create_platform(pear->runtime.loop, NULL, &pear->runtime.platform);
  assert(err == 0);

  err = js_create_env(pear->runtime.loop, pear->runtime.platform, &pear->runtime.env);
  assert(err == 0);

  pear->runtime.process = pear;
  pear->runtime.argc = argc;
  pear->runtime.argv = argv;

  pear_runtime_setup(&pear->runtime);

  pear->suspended = false;
  pear->exited = false;

  err = uv_sem_init(&pear->idle, 0);
  assert(err == 0);

  pear->threads = NULL;

  err = uv_mutex_init(&pear->threads_lock);
  assert(err == 0);

  pear->on_before_exit = NULL;
  pear->on_exit = NULL;
  pear->on_suspend = NULL;
  pear->on_idle = NULL;
  pear->on_resume = NULL;

  *result = pear;

  return 0;
}

int
pear_teardown (pear_t *pear, int *exit_code) {
  int err;

  pear_runtime_on_exit(&pear->runtime, exit_code);

  err = js_destroy_env(pear->runtime.env);
  assert(err == 0);

  err = js_destroy_platform(pear->runtime.platform);
  assert(err == 0);

  uv_sem_destroy(&pear->idle);

  uv_mutex_destroy(&pear->threads_lock);

  free(pear);

  return 0;
}

int
pear_run (pear_t *pear, const char *filename, const uv_buf_t *source) {
  int err = pear_runtime_run(&pear->runtime, filename, source);
  if (err < 0) return err;

  do {
    uv_run(pear->runtime.loop, UV_RUN_DEFAULT);

    if (pear->suspended) {
      pear_runtime_on_idle(&pear->runtime);

      uv_sem_wait(&pear->idle);
    } else {
      pear_runtime_on_before_exit(&pear->runtime);
    }
  } while (uv_loop_alive(pear->runtime.loop));

  return 0;
}

int
pear_exit (pear_t *pear, int exit_code) {
  if (pear->exited) return -1;
  pear->exited = true;

  pear_runtime_on_exit(&pear->runtime, exit_code == -1 ? &exit_code : NULL);

  exit(exit_code);

  return 0;
}

int
pear_suspend (pear_t *pear) {
  if (pear->suspended) return -1;
  pear->suspended = true;

  pear_runtime_on_suspend(&pear->runtime);

  return 0;
}

int
pear_resume (pear_t *pear) {
  if (!pear->suspended) return -1;
  pear->suspended = false;

  uv_sem_post(&pear->idle);

  pear_runtime_on_resume(&pear->runtime);

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
pear_get_platform (pear_t *pear, js_platform_t **result) {
  *result = pear->runtime.platform;

  return 0;
}

int
pear_get_env (pear_t *pear, js_env_t **result) {
  *result = pear->runtime.env;

  return 0;
}

int
pear_get_data (pear_t *pear, const char *key, js_value_t **result) {
  return pear_runtime_get_data(&pear->runtime, key, result);
}

int
pear_get_data_external (pear_t *pear, const char *key, void **result) {
  js_value_t *external;
  int err = pear_get_data(pear, key, &external);
  if (err < 0) return err;

  return js_get_value_external(pear->runtime.env, external, result);
}

int
pear_set_data (pear_t *pear, const char *key, js_value_t *value) {
  return pear_runtime_set_data(&pear->runtime, key, value);
}

int
pear_set_data_external (pear_t *pear, const char *key, void *value) {
  js_value_t *external;
  int err = js_create_external(pear->runtime.env, value, NULL, NULL, &external);

  return pear_set_data(pear, key, external);
}
