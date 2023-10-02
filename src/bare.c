#include <assert.h>
#include <js.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <uv.h>

#include "../include/bare.h"

#include "runtime.h"
#include "types.h"

struct bare_s {
  bare_process_t process;

  bool exited;
};

int
bare_setup (uv_loop_t *loop, js_platform_t *platform, js_env_t **env, int argc, char **argv, const bare_options_t *options, bare_t **result) {
  int err;

  bare_t *bare = malloc(sizeof(bare_t));

  bare->exited = false;

  bare_process_t *process = &bare->process;

  process->runtime = malloc(sizeof(bare_runtime_t));

  process->platform = platform;
  process->argc = argc;
  process->argv = argv;

  process->on_before_exit = NULL;
  process->on_exit = NULL;
  process->on_suspend = NULL;
  process->on_idle = NULL;
  process->on_resume = NULL;
  process->on_thread = NULL;

  bare_runtime_t *runtime = process->runtime;

  err = bare_runtime_setup(loop, process, runtime);
  assert(err == 0);

  if (env) *env = runtime->env;

  *result = bare;

  return 0;
}

int
bare_teardown (bare_t *bare, int *exit_code) {
  int err;

  bare_process_t *process = &bare->process;

  err = bare_runtime_teardown(process->runtime, exit_code);
  assert(err == 0);

  free(bare);

  return 0;
}

int
bare_run (bare_t *bare, const char *filename, const uv_buf_t *source) {
  int err;

  bare_runtime_t *runtime = bare->process.runtime;

  err = bare_runtime_run(runtime, filename, source);
  if (err < 0) return err;

  bare->exited = true;

  return 0;
}

int
bare_exit (bare_t *bare, int exit_code) {
  if (bare->exited) return -1;

  int err;

  bare->exited = true;

  err = bare_teardown(bare, exit_code == -1 ? &exit_code : NULL);
  assert(err == 0);

  exit(exit_code);
}

int
bare_suspend (bare_t *bare) {
  return uv_async_send(&bare->process.runtime->signals.suspend);
}

int
bare_resume (bare_t *bare) {
  return uv_async_send(&bare->process.runtime->signals.resume);
}

int
bare_on_before_exit (bare_t *bare, bare_before_exit_cb cb) {
  bare->process.on_before_exit = cb;

  return 0;
}

int
bare_on_exit (bare_t *bare, bare_exit_cb cb) {
  bare->process.on_exit = cb;

  return 0;
}

int
bare_on_suspend (bare_t *bare, bare_suspend_cb cb) {
  bare->process.on_suspend = cb;

  return 0;
}

int
bare_on_idle (bare_t *bare, bare_idle_cb cb) {
  bare->process.on_idle = cb;

  return 0;
}

int
bare_on_resume (bare_t *bare, bare_resume_cb cb) {
  bare->process.on_resume = cb;

  return 0;
}

int
bare_on_thread (bare_t *bare, bare_thread_cb cb) {
  bare->process.on_thread = cb;

  return 0;
}
