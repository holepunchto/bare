#include <js.h>
#include <stddef.h>
#include <stdlib.h>
#include <uv.h>

#include "../include/bare.h"

#include "runtime.h"
#include "types.h"

#define bare_option(options, min_version, field) \
  (options && options->version >= min_version ? options->field : bare_default_options.field)

struct bare_s {
  bare_process_t process;
};

static const bare_options_t bare_default_options = {
  .version = 0,
  .memory_limit = 0,
};

int
bare_version(int *major, int *minor, int *patch) {
  if (major) *major = BARE_VERSION_MAJOR;
  if (minor) *minor = BARE_VERSION_MINOR;
  if (patch) *patch = BARE_VERSION_PATCH;

  return 0;
}

int
bare_setup(uv_loop_t *loop, js_platform_t *platform, js_env_t **env, int argc, const char *argv[], const bare_options_t *options, bare_t **result) {
  int err;

  bare_t *bare = malloc(sizeof(bare_t));

  bare_process_t *process = &bare->process;

  process->runtime = malloc(sizeof(bare_runtime_t));

  process->options = bare_default_options;

  process->options.memory_limit = bare_option(options, 0, memory_limit);

  process->platform = platform;
  process->argc = argc;
  process->argv = argv;

  process->callbacks.before_exit = NULL;
  process->callbacks.exit = NULL;
  process->callbacks.teardown = NULL;
  process->callbacks.suspend = NULL;
  process->callbacks.wakeup = NULL;
  process->callbacks.idle = NULL;
  process->callbacks.resume = NULL;
  process->callbacks.thread = NULL;

  bare_runtime_t *runtime = process->runtime;

  err = bare_runtime_setup(loop, process, runtime);
  if (err < 0) {
    free(process->runtime);
    free(process);

    return err;
  }

  if (env) *env = runtime->env;

  *result = bare;

  return 0;
}

int
bare_teardown(bare_t *bare, int *exit_code) {
  int err = bare_runtime_teardown(bare->process.runtime, exit_code);
  if (err < 0) return err;

  free(bare);

  return 0;
}

int
bare_exit(bare_t *bare, int exit_code) {
  return bare_runtime_exit(bare->process.runtime, exit_code);
}

int
bare_load(bare_t *bare, const char *filename, const uv_buf_t *source, js_value_t **result) {
  return bare_runtime_load(
    bare->process.runtime,
    filename,
    (bare_source_t) {
      .type = source ? bare_source_buffer : bare_source_none,
      .buffer = uv_buf_init(
        source ? source->base : NULL,
        source ? (unsigned int) source->len : 0
      ),
    },
    result
  );
}

int
bare_run(bare_t *bare) {
  return bare_runtime_run(bare->process.runtime);
}

int
bare_suspend(bare_t *bare, int linger) {
  bare->process.runtime->linger = linger;
  bare->process.runtime->suspending = true;

  return uv_async_send(&bare->process.runtime->signals.suspend);
}

int
bare_wakeup(bare_t *bare, int deadline) {
  bare->process.runtime->deadline = deadline;

  int err = uv_async_send(&bare->process.runtime->signals.wakeup);
  if (err < 0) return err;

  uv_cond_signal(&bare->process.runtime->wake);

  return 0;
}

int
bare_resume(bare_t *bare) {
  bare->process.runtime->suspending = false;

  int err = uv_async_send(&bare->process.runtime->signals.resume);
  if (err < 0) return err;

  uv_cond_signal(&bare->process.runtime->wake);

  return 0;
}

int
bare_terminate(bare_t *bare) {
  int err = uv_async_send(&bare->process.runtime->signals.terminate);
  if (err < 0) return err;

  uv_cond_signal(&bare->process.runtime->wake);

  return 0;
}

int
bare_on_before_exit(bare_t *bare, bare_before_exit_cb cb, void *data) {
  bare->process.callbacks.before_exit = cb;
  bare->process.callbacks.before_exit_data = data;

  return 0;
}

int
bare_on_exit(bare_t *bare, bare_exit_cb cb, void *data) {
  bare->process.callbacks.exit = cb;
  bare->process.callbacks.exit_data = data;

  return 0;
}

int
bare_on_teardown(bare_t *bare, bare_teardown_cb cb, void *data) {
  bare->process.callbacks.teardown = cb;
  bare->process.callbacks.teardown_data = data;

  return 0;
}

int
bare_on_suspend(bare_t *bare, bare_suspend_cb cb, void *data) {
  bare->process.callbacks.suspend = cb;
  bare->process.callbacks.suspend_data = data;

  return 0;
}

int
bare_on_wakeup(bare_t *bare, bare_wakeup_cb cb, void *data) {
  bare->process.callbacks.wakeup = cb;
  bare->process.callbacks.wakeup_data = data;

  return 0;
}

int
bare_on_idle(bare_t *bare, bare_idle_cb cb, void *data) {
  bare->process.callbacks.idle = cb;
  bare->process.callbacks.idle_data = data;

  return 0;
}

int
bare_on_resume(bare_t *bare, bare_resume_cb cb, void *data) {
  bare->process.callbacks.resume = cb;
  bare->process.callbacks.resume_data = data;

  return 0;
}

int
bare_on_thread(bare_t *bare, bare_thread_cb cb, void *data) {
  bare->process.callbacks.thread = cb;
  bare->process.callbacks.thread_data = data;

  return 0;
}
