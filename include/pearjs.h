#ifndef PEARJS_H
#define PEARJS_H

#include <js.h>

#ifdef __APPLE__
#define PEARJS_PLATFORM "darwin"
#elif __linux__
#define PEARJS_PLATFORM "linux"
#else
#define PEARJS_PLATFORM "unknown"
#endif

#ifdef __aarch64__
#define PEARJS_ARCH "arm64"
#elif __x86_64
#define PEARJS_ARCH "x64"
#elif __i386__
#define PEARJS_ARCH "ia32"
#else
#define PEARJS_ARCH "unknown"
#endif

typedef js_value_t * (*pearjs_addon_register)(js_env_t *env, js_value_t *exports);

typedef struct {
  const char *name;
  pearjs_addon_register register_addon;

  void *next_addon;
} pearjs_module_t;

extern void
pearjs_module_register (pearjs_module_t *mod);

#endif
