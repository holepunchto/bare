#ifndef BARE_MODULE_H
#define BARE_MODULE_H

#include <js.h>

// https://stackoverflow.com/a/2390626

#if defined(__cplusplus)
#define BARE_INITIALIZER(f) \
  static void f(void); \
  struct f##_ { \
    f##_(void) { f(); } \
  } f##_; \
  static void f(void)
#elif defined(_MSC_VER)
#pragma section(".CRT$XCU", read)
#define BARE_INITIALIZER(f) \
  static void f(void); \
  __declspec(dllexport, allocate(".CRT$XCU")) void (*f##_)(void) = f; \
  static void f(void)
#else
#define BARE_INITIALIZER(f) \
  static void f(void) __attribute__((constructor)); \
  static void f(void)
#endif

#define BARE_MODULE_VERSION 1

#ifndef BARE_MODULE_FILENAME
#define BARE_MODULE_FILENAME ""
#endif

#define BARE_MODULE(id, fn) \
  BARE_INITIALIZER(module_initializer_##id) { \
    bare_module_t module = { \
      BARE_MODULE_VERSION, \
      BARE_MODULE_FILENAME, \
      fn, \
    }; \
    bare_module_register(&module); \
  }

typedef struct bare_module_s bare_module_t;

typedef js_value_t *(*bare_module_cb)(js_env_t *env, js_value_t *exports);

struct bare_module_s {
  int version;
  const char *filename;
  bare_module_cb init;
};

void
bare_module_register (bare_module_t *mod);

#endif // BARE_MODULE_H
