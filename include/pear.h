#ifndef PEAR_H
#define PEAR_H

#include <js.h>

#ifdef __APPLE__
#define PEAR_PLATFORM "darwin"
#elif __linux__
#define PEAR_PLATFORM "linux"
#else
#define PEAR_PLATFORM "unknown"
#endif

#ifdef __aarch64__
#define PEAR_ARCH "arm64"
#elif __x86_64
#define PEAR_ARCH "x64"
#elif __i386__
#define PEAR_ARCH "ia32"
#else
#define PEAR_ARCH "unknown"
#endif

// https://stackoverflow.com/a/2390626

#if defined(__cplusplus)
#define PEAR_INITIALIZER(f) \
  static void f(void); \
  struct f##_ { \
    f##_(void) { f(); } \
  } f##_; \
  static void f(void)
#elif defined(_MSC_VER)
#pragma section(".CRT$XCU", read)
#define PEAR_INITIALIZER(f) \
  static void f(void); \
  __declspec(dllexport, allocate(".CRT$XCU")) void (*f##_)(void) = f;
#else
#define PEAR_INITIALIZER(f) \
  static void f(void) __attribute__((constructor)); \
  static void f(void)
#endif

#define PEAR_MODULE_VERSION 1

#ifndef PEAR_MODULE_FILENAME
#define PEAR_MODULE_FILENAME ""
#endif

#define PEAR_MODULE(fn) \
  PEAR_INITIALIZER(module_initializer) { \
    pear_module_t module = { \
      PEAR_MODULE_VERSION, \
      PEAR_MODULE_FILENAME, \
      fn, \
      NULL \
    }; \
    pear_module_register(&module); \
  }

typedef js_value_t * (*pear_addon_register)(js_env_t *env, js_value_t *exports);

typedef struct {
  int version;
  const char *filename;
  pear_addon_register register_addon;

  void *next_addon;
} pear_module_t;

extern void
pear_module_register (pear_module_t *mod);

#endif
