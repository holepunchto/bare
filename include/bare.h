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
typedef void (*bare_suspend_cb)(bare_t *);
typedef void (*bare_idle_cb)(bare_t *);
typedef void (*bare_resume_cb)(bare_t *);
typedef void (*bare_thread_cb)(bare_t *, js_env_t *);

/** @version 0 */
struct bare_options_s {
  int version;

  /**
   * The directory containing native addons. If not provided, the addon
   * resolution algorithm will not consider it.
   *
   * @since 0
   */
  const char *addons;
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
 * Run the module identified by `filename`, which may be any of the formats
 * supported by the module system. Unless `source` is provided, the contents
 * of `filename` will be read from disk.
 */
int
bare_run (bare_t *bare, const char *filename, const uv_buf_t *source);

/**
 * Suspend the process as soon as possible. Once the process has suspended
 * successfully, `bare_run()` will not return until another thread resumes the
 * process. It's safe to call this function from any thread.
 */
int
bare_suspend (bare_t *bare);

/**
 * Resume the process as soon as possible. If the process is not yet idle after
 * being suspended the suspension will be cancelled. It's safe to call this
 * function from any thread.
 */
int
bare_resume (bare_t *bare);

int
bare_on_before_exit (bare_t *bare, bare_before_exit_cb cb);

int
bare_on_exit (bare_t *bare, bare_exit_cb cb);

int
bare_on_suspend (bare_t *bare, bare_suspend_cb cb);

int
bare_on_idle (bare_t *bare, bare_idle_cb cb);

int
bare_on_resume (bare_t *bare, bare_resume_cb cb);

int
bare_on_thread (bare_t *bare, bare_thread_cb cb);

#ifdef __cplusplus
}
#endif

#endif // BARE_H
