#include <assert.h>
#include <js.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <uv.h>

#include "../include/bare.h"
#include "addons.h"
#include "runtime.h"
#include "types.h"

static void
on_suspend (uv_async_t *handle) {
  bare_t *bare = (bare_t *) handle->data;

  bare_runtime_on_suspend(&bare->runtime);

  // The process got suspended, but the suspension was cancelled; flush the
  // semaphore without blocking and invoke the resume callback.
  if (bare->resumed) {
    uv_sem_trywait(&bare->resume);

    bare_runtime_on_resume(&bare->runtime);

    bare->resumed = false;
  }
}

int
bare_setup (uv_loop_t *loop, int argc, char **argv, const bare_options_t *options, bare_t **result) {
  bare_t *bare = malloc(sizeof(bare_t));

  bare->runtime.loop = loop;

  int err;

  err = js_create_platform(bare->runtime.loop, NULL, &bare->runtime.platform);
  assert(err == 0);

  err = js_create_env(bare->runtime.loop, bare->runtime.platform, NULL, &bare->runtime.env);
  assert(err == 0);

  bare->runtime.process = bare;
  bare->runtime.argc = argc;
  bare->runtime.argv = argv;

  bare_runtime_setup(&bare->runtime);

  bare->suspended = false;
  bare->resumed = false;
  bare->exited = false;

  err = uv_async_init(loop, &bare->suspend, on_suspend);
  assert(err == 0);

  bare->suspend.data = (void *) bare;

  uv_unref((uv_handle_t *) &bare->suspend);

  err = uv_sem_init(&bare->resume, 0);
  assert(err == 0);

  bare->threads = NULL;

  bare->on_before_exit = NULL;
  bare->on_exit = NULL;
  bare->on_suspend = NULL;
  bare->on_idle = NULL;
  bare->on_resume = NULL;

  err = uv_rwlock_init(&bare->locks.threads);
  assert(err == 0);

  err = uv_rwlock_init(&bare->locks.env);
  assert(err == 0);

  *result = bare;

  return 0;
}

int
bare_teardown (bare_t *bare, int *exit_code) {
  int err;

  bare_runtime_on_exit(&bare->runtime, exit_code);

  err = js_destroy_env(bare->runtime.env);
  assert(err == 0);

  err = js_destroy_platform(bare->runtime.platform);
  assert(err == 0);

  uv_close((uv_handle_t *) &bare->suspend, NULL);

  uv_sem_destroy(&bare->resume);

  uv_rwlock_destroy(&bare->locks.threads);

  uv_rwlock_destroy(&bare->locks.env);

  free(bare);

  return 0;
}

int
bare_run (bare_t *bare, const char *filename, const uv_buf_t *source) {
  int err;

  err = bare_runtime_run(&bare->runtime, filename, source);
  if (err < 0) return err;

  do {
    uv_run(bare->runtime.loop, UV_RUN_DEFAULT);

    // The process got suspended; invoke the idle callback, wait for the
    // process to resume, and then invoke the resume callback.
    if (bare->suspended) {
      bare_runtime_on_idle(&bare->runtime);

      uv_sem_wait(&bare->resume);

      bare_runtime_on_resume(&bare->runtime);
    }

    // The process finished; invoke the before exit callback to give it a
    // chance to queue additional work.
    else {
      bare_runtime_on_before_exit(&bare->runtime);
    }

    bare->suspended = bare->resumed = false;
  } while (uv_loop_alive(bare->runtime.loop));

  bare->exited = true;

  return 0;
}

int
bare_exit (bare_t *bare, int exit_code) {
  if (bare->exited) return -1;
  bare->exited = true;

  bare_runtime_on_exit(&bare->runtime, exit_code == -1 ? &exit_code : NULL);

  exit(exit_code);

  return 0;
}

int
bare_suspend (bare_t *bare) {
  bool suspended = false;

  if (!atomic_compare_exchange_strong(&bare->suspended, &suspended, true)) {
    return -1;
  }

  bare->resumed = false;

  return uv_async_send(&bare->suspend);
}

int
bare_resume (bare_t *bare) {
  bool suspended = true;

  if (!atomic_compare_exchange_strong(&bare->suspended, &suspended, false)) {
    return -1;
  }

  bare->resumed = true;

  uv_sem_post(&bare->resume);

  return 0;
}

int
bare_on_before_exit (bare_t *bare, bare_before_exit_cb cb) {
  bare->on_before_exit = cb;

  return 0;
}

int
bare_on_exit (bare_t *bare, bare_exit_cb cb) {
  bare->on_exit = cb;

  return 0;
}

int
bare_on_suspend (bare_t *bare, bare_suspend_cb cb) {
  bare->on_suspend = cb;

  return 0;
}

int
bare_on_idle (bare_t *bare, bare_idle_cb cb) {
  bare->on_idle = cb;

  return 0;
}

int
bare_on_resume (bare_t *bare, bare_resume_cb cb) {
  bare->on_resume = cb;

  return 0;
}

int
bare_get_platform (bare_t *bare, js_platform_t **result) {
  *result = bare->runtime.platform;

  return 0;
}

int
bare_get_env (bare_t *bare, js_env_t **result) {
  *result = bare->runtime.env;

  return 0;
}
