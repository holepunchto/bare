#ifndef PEAR_H
#define PEAR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <js.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <uv.h>

#include "pear/modules.h"
#include "pear/target.h"
#include "pear/version.h"

typedef struct pear_s pear_t;

typedef void (*pear_before_exit_cb)(pear_t *);
typedef void (*pear_exit_cb)(pear_t *);
typedef void (*pear_suspend_cb)(pear_t *);
typedef void (*pear_idle_cb)(pear_t *);
typedef void (*pear_resume_cb)(pear_t *);

int
pear_setup (uv_loop_t *loop, int argc, char **argv, pear_t **result);

int
pear_teardown (pear_t *pear, int *exit_code);

int
pear_run (pear_t *pear, const char *filename, const uv_buf_t *source);

int
pear_run_begin (pear_t *pear, const char *filename, const uv_buf_t *source);

int
pear_run_tick (pear_t *pear);

int
pear_exit (pear_t *pear, int exit_code);

int
pear_suspend (pear_t *pear);

int
pear_resume (pear_t *pear);

int
pear_on_before_exit (pear_t *pear, pear_before_exit_cb cb);

int
pear_on_exit (pear_t *pear, pear_exit_cb cb);

int
pear_on_suspend (pear_t *pear, pear_suspend_cb cb);

int
pear_on_idle (pear_t *pear, pear_idle_cb cb);

int
pear_on_resume (pear_t *pear, pear_resume_cb cb);

int
pear_get_platform (pear_t *pear, js_platform_t **result);

int
pear_get_env (pear_t *pear, js_env_t **result);

#ifdef __cplusplus
}
#endif

#endif // PEAR_H
