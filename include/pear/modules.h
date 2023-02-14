#ifndef PEAR_MODULES_H
#define PEAR_MODULES_H

#include <js.h>

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
      NULL, \
      NULL, \
    }; \
    pear_module_register(&module); \
  }

typedef struct pear_module_s pear_module_t;

typedef js_value_t *(*pear_module_cb)(js_env_t *env, js_value_t *exports);

struct pear_module_s {
  int version;
  const char *filename;
  pear_module_cb init;
  js_value_t *exports;
  uv_lib_t *lib;
};

void
pear_module_register (pear_module_t *mod);

#endif // PEAR_MODULES_H
