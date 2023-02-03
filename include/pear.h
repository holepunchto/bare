#ifndef PEAR_H
#define PEAR_H

#include <js.h>
#include <stdbool.h>
#include <stddef.h>
#include <uv.h>

#include "pear/modules.h"

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

typedef struct pear_s pear_t;

struct pear_s {
  uv_loop_t *loop;
  js_platform_t *platform;
  js_env_t *env;
  uv_async_t suspend;
  uv_async_t resume;
  bool suspended;

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
pear_run (pear_t *pear, const char *filename, const char *source, size_t len);

int
pear_suspend (pear_t *pear);

int
pear_resume (pear_t *pear);

#endif // PEAR_H
