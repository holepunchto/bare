#ifndef BARE_MODULE_H
#define BARE_MODULE_H

#include <js.h>

#include "helper.h"

#define BARE_MODULE_VERSION 0

#ifndef BARE_MODULE_FILENAME
#define BARE_MODULE_FILENAME ""
#endif

#define BARE_MODULE_SYMBOL_HELPER(base, version) BARE_CONCAT(base, version)

#define BARE_MODULE_SYMBOL_REGISTER_BASE bare_register_module_v

#define BARE_MODULE_SYMBOL_REGISTER \
  BARE_MODULE_SYMBOL_HELPER(BARE_MODULE_SYMBOL_REGISTER_BASE, BARE_MODULE_VERSION)

// https://stackoverflow.com/a/2390626

#if defined(__cplusplus)
#define BARE_MODULE_CONSTRUCTOR(f) \
  static void f(void); \
  struct f##_ { \
    f##_(void) { f(); } \
  } f##_; \
  static void f(void)
#elif defined(_MSC_VER)
#pragma section(".CRT$XCU", read)
#define BARE_MODULE_CONSTRUCTOR(f) \
  static void f(void); \
  __declspec(dllexport, allocate(".CRT$XCU")) void (*f##_)(void) = f; \
  __pragma(comment(linker, "/include:" #f "_")) static void f(void)
#else
#define BARE_MODULE_CONSTRUCTOR(f) \
  static void f(void) __attribute__((constructor)); \
  static void f(void)
#endif

#ifdef BARE_MODULE_REGISTER_CONSTRUCTOR

// Constructor based module registration: This method registers modules by using
// a compiler-dependant constructor function that will run before the `main()`
// function. This method is suited for both dynamic and static modules.

#define BARE_MODULE(id, fn) \
  BARE_MODULE_CONSTRUCTOR(bare_register_module_##id) { \
    bare_module_t module = { \
      BARE_MODULE_VERSION, \
      BARE_MODULE_FILENAME, \
      fn, \
    }; \
    bare_module_register(&module); \
  }

#else

// Symbol based module registration: This method registers modules by exposing
// a known symbol that can be loaded from a shared library. It is NOT suited
// for registering static modules as this will cause symbol duplication.

#define BARE_MODULE(id, fn) \
  js_value_t *BARE_MODULE_SYMBOL_REGISTER(js_env_t *env, js_value_t *exports) { \
    return fn(env, exports); \
  }

#endif

typedef struct bare_module_s bare_module_t;

typedef js_value_t *(*bare_module_cb)(js_env_t *env, js_value_t *exports);

/** @version 0 */
struct bare_module_s {
  int version;

  /** @since 0 */
  const char *filename;

  /** @since 0 */
  bare_module_cb init;
};

void
bare_module_register (bare_module_t *mod);

#endif // BARE_MODULE_H
