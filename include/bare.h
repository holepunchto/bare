#ifndef BARE_H
#define BARE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <js.h>
#include <uv.h>

#include "bare/module.h"
#include "bare/target.h"
#include "bare/version.h"

typedef struct bare_s bare_t;
typedef struct bare_options_s bare_options_t;

typedef void (*bare_before_exit_cb)(bare_t *);
typedef void (*bare_exit_cb)(bare_t *);
typedef void (*bare_teardown_cb)(bare_t *);
typedef void (*bare_suspend_cb)(bare_t *, int linger);
typedef void (*bare_idle_cb)(bare_t *);
typedef void (*bare_resume_cb)(bare_t *);
typedef void (*bare_thread_cb)(bare_t *, js_env_t *);

/** @version 0 */
struct bare_options_s {
  int version;

  /**
   * The memory limit of each JavaScript heap. By default, the limit will be
   * inferred based on the amount of physical memory of the device.
   *
   * Note that the limit applies individually to each thread, including the
   * main thread.
   *
   * @since 0
   */
  size_t memory_limit;
};

/**
 * Set up the Bare process. To get a reference to the JavaScript environment of
 * the process, pass the `env` pointer.
 */
int
bare_setup (uv_loop_t *loop, js_platform_t *platform, js_env_t **env, int argc, char **argv, const bare_options_t *options, bare_t **result);

/**
 * Tear down the Bare process. The exit code will be stored in `exit_code` if
 * provided. The JavaScript environment of the process must not be used after
 * this function returns.
 */
int
bare_teardown (bare_t *bare, int *exit_code);

/**
 * Load the module identified by `filename`, which may be any of the formats
 * supported by the module system. Unless `source` is provided, the contents
 * of `filename` will be read from disk.
 *
 * See https://github.com/holepunchto/bare-module for more information on the
 * supported module formats.
 */
int
bare_load (bare_t *bare, const char *filename, const uv_buf_t *source, js_value_t **result);

/**
 * Run the I/O event loop.
 */
int
bare_run (bare_t *bare);

/**
 * Suspend the process as soon as possible. Once the process has suspended
 * successfully, `bare_run()` will not return until another thread resumes the
 * process. It's safe to call this function from any thread.
 */
int
bare_suspend (bare_t *bare, int linger);

/**
 * Resume the process as soon as possible. If the process is not yet idle after
 * being suspended the suspension will be cancelled. It's safe to call this
 * function from any thread.
 */
int
bare_resume (bare_t *bare);

/**
 * Equivalent to `Bare.on('beforeExit', cb)`.
 */
int
bare_on_before_exit (bare_t *bare, bare_before_exit_cb cb);

/**
 * Equivalent to `Bare.on('exit', cb)`.
 */
int
bare_on_exit (bare_t *bare, bare_exit_cb cb);

/**
 * Equivalent to `Bare.on('teardown', cb)`.
 */
int
bare_on_teardown (bare_t *bare, bare_teardown_cb cb);

/**
 * Equivalent to `Bare.on('suspend', cb)`.
 */
int
bare_on_suspend (bare_t *bare, bare_suspend_cb cb);

/**
 * Equivalent to `Bare.on('idle', cb)`.
 */
int
bare_on_idle (bare_t *bare, bare_idle_cb cb);

/**
 * Equivalent to `Bare.on('resume', cb)`.
 */
int
bare_on_resume (bare_t *bare, bare_resume_cb cb);

/**
 * Attach a thread listener which will invoked with the JavaScript environment
 * of each thread created with the `Thread` constructor. Use this to modify the
 * environment of the thread before it runs any JavaScript.
 */
int
bare_on_thread (bare_t *bare, bare_thread_cb cb);

#ifdef __cplusplus
}
#endif

#endif // BARE_H
