#ifndef PEAR_H
#define PEAR_H

#include <js.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <uv.h>

#include "pear/modules.h"
#include "pear/version.h"

#if defined(__APPLE__)
#define PEAR_PLATFORM "darwin"
#elif defined(__linux__)
#define PEAR_PLATFORM "linux"
#else
#error Unsupported platform
#endif

#if defined(__aarch64__)
#define PEAR_ARCH "arm64"
#elif defined(__x86_64)
#define PEAR_ARCH "x64"
#elif defined(__i386__)
#define PEAR_ARCH "ia32"
#else
#error Unsupported architecture
#endif

#define PEAR_TARGET PEAR_PLATFORM "-" PEAR_ARCH

typedef struct pear_s pear_t;

typedef void (*pear_before_exit_cb)(pear_t *);
typedef void (*pear_exit_cb)(pear_t *);

struct pear_s {
  uv_loop_t *loop;
  js_platform_t *platform;
  js_env_t *env;
  uv_idle_t idle;
  bool suspended;

  pear_before_exit_cb on_before_exit;
  pear_exit_cb on_exit;

  struct {
    js_value_t *exports;
    int argc;
    char **argv;
  } runtime;
};

int
pear_setup (uv_loop_t *loop, pear_t *pear, int argc, char **argv);

int
pear_teardown (pear_t *pear, int *exit_code);

int
pear_run (pear_t *pear, const char *filename, const uv_buf_t *source);

int
pear_suspend (pear_t *pear);

int
pear_resume (pear_t *pear);

int
pear_on_before_exit (pear_t *pear, pear_before_exit_cb cb);

int
pear_on_exit (pear_t *pear, pear_exit_cb cb);

int
pear_get_data (pear_t *pear, const char *key, void **result);

int
pear_set_data (pear_t *pear, const char *key, void *value);

#endif // PEAR_H
