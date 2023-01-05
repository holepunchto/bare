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

// https://stackoverflow.com/a/2390626

#if defined(__cplusplus)
#define PEARJS_INITIALIZER(f) \
  static void f(void); \
  struct f##_ { \
    f##_(void) { f(); } \
  } f##_; \
  static void f(void)
#elif defined(_MSC_VER)
#pragma section(".CRT$XCU", read)
#define PEARJS_INITIALIZER(f) \
  static void f(void); \
  __declspec(dllexport, allocate(".CRT$XCU")) void (*f##_)(void) = f;
#else
#define PEARJS_INITIALIZER(f) \
  static void f(void) __attribute__((constructor)); \
  static void f(void)
#endif

#define PEARJS_MODULE_VERSION 1

#define PEARJS_MODULE_NAME(name) #name

#define PEARJS_MODULE(name, fn) \
  PEARJS_INITIALIZER(module_initializer) { \
    pearjs_module_t module = { \
      PEARJS_MODULE_VERSION, \
      __FILE__, \
      PEARJS_MODULE_NAME(name), \
      fn, \
      NULL \
    }; \
    pearjs_module_register(&module); \
  }

typedef js_value_t * (*pearjs_addon_register)(js_env_t *env, js_value_t *exports);

typedef struct {
  int version;
  const char *filename;
  const char *modname;
  pearjs_addon_register register_addon;

  void *next_addon;
} pearjs_module_t;

extern void
pearjs_module_register (pearjs_module_t *mod);

#endif
