#ifndef BARE_H
#define BARE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <js.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
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

struct bare_options_s {
  /**
   * The directory containing native addons. If not provided, the addon
   * resolution algorithm will not consider it.
   */
  const char *addons;
};

int
bare_setup (uv_loop_t *loop, js_platform_t *platform, js_env_t **env, int argc, char **argv, const bare_options_t *options, bare_t **result);

int
bare_teardown (bare_t *bare, int *exit_code);

int
bare_run (bare_t *bare, const char *filename, const uv_buf_t *source);

int
bare_exit (bare_t *bare, int exit_code);

int
bare_suspend (bare_t *bare);

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

#ifdef __cplusplus
}
#endif

#endif // BARE_H
