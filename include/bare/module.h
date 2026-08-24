#ifndef BARE_MODULE_H
#define BARE_MODULE_H

#include <js.h>

#include "helper.h"

#define BARE_MODULE_VERSION 0

#ifndef BARE_MODULE_NAME
#define BARE_MODULE_NAME NULL
#endif

#define BARE_MODULE_SYMBOL_HELPER(base, version) BARE_CONCAT(base, version)

#define BARE_MODULE_SYMBOL_NAME_BASE bare_get_module_name_v

#define BARE_MODULE_SYMBOL_NAME \
  BARE_MODULE_SYMBOL_HELPER(BARE_MODULE_SYMBOL_NAME_BASE, BARE_MODULE_VERSION)

#define BARE_MODULE_SYMBOL_REGISTER_BASE bare_register_module_v

#define BARE_MODULE_SYMBOL_REGISTER \
  BARE_MODULE_SYMBOL_HELPER(BARE_MODULE_SYMBOL_REGISTER_BASE, BARE_MODULE_VERSION)

// https://stackoverflow.com/a/2390626

#if defined(_MSC_VER)
#pragma section(".CRT$XCU", read)
#define BARE_MODULE_CONSTRUCTOR_BASE(id, version) \
  __pragma(comment(linker, "/include:bare_register_module_" #id "_" #version "_")); \
  static void bare_register_module_##id(void); \
  __declspec(dllexport, allocate(".CRT$XCU")) void (*bare_register_module_##id##_##version##_)(void) = bare_register_module_##id; \
  static void bare_register_module_##id(void)
#else
#define BARE_MODULE_CONSTRUCTOR_BASE(id, version) \
  static void bare_register_module_##id(void) __attribute__((constructor)); \
  static void bare_register_module_##id(void)
#endif

#define BARE_MODULE_CONSTRUCTOR(id, version) BARE_MODULE_CONSTRUCTOR_BASE(id, version)

#ifdef BARE_MODULE_REGISTER_CONSTRUCTOR

// Constructor based module registration: This method registers modules by using
// a compiler-dependant constructor function that will run before the `main()`
// function. This method is suited for both dynamic and static modules.

#define BARE_MODULE(id, fn) \
  BARE_EXTERN_C_START \
  BARE_MODULE_CONSTRUCTOR(id, BARE_MODULE_CONSTRUCTOR_VERSION) { \
    bare_module_t module = { \
      BARE_MODULE_VERSION, \
      BARE_MODULE_NAME, \
      fn, \
    }; \
    bare_module_register(&module); \
  } \
  BARE_EXTERN_C_END

#else

// Symbol based module registration: This method registers modules by exposing
// a known symbol that can be loaded from a shared library. It is NOT suited
// for registering static modules as this will cause symbol duplication.

#define BARE_MODULE(id, fn) \
  BARE_EXTERN_C_START \
  const char *BARE_MODULE_SYMBOL_NAME(void) { \
    return BARE_MODULE_NAME; \
  } \
  js_value_t *BARE_MODULE_SYMBOL_REGISTER(js_env_t *env, js_value_t *exports) { \
    return fn(env, exports); \
  } \
  BARE_EXTERN_C_END

#endif

typedef struct bare_module_s bare_module_t;

typedef const char *(*bare_module_name_cb)(void);

typedef js_value_t *(*bare_module_register_cb)(js_env_t *env, js_value_t *exports);

/** @version 0 */
struct bare_module_s {
  int version;

  /** @since 0 */
  const char *name;

  /** @since 0 */
  bare_module_register_cb exports;
};

/**
 * Find a loaded addon by name, optionally suffixed with `.bare`. Addons are
 * named `<name>@<version>` and the version may be truncated at a component
 * boundary, which makes it possible to look up an addon by its major version
 * alone: `foo@1` matches `foo@1.2.3` but not `foo@10.0.0`. A query that carries
 * no version matches only an addon registered without one. If several addons
 * match, the most recently loaded of them is returned.
 *
 * Only addons loaded by the process running on the calling thread are matched,
 * along with the statically linked addons, which are compiled into the binary
 * and available to every process. `NULL` is returned if no addon matches or if
 * no process is running on the thread.
 *
 * The returned library is owned by Bare and must not be closed. It stays loaded
 * for as long as the process that loaded the addon, which may be shorter than
 * the operating system process. A caller that holds on to the library beyond
 * the call, such as a delay load hook binding an import address table, must
 * take a reference of its own.
 */
uv_lib_t *
bare_module_find(const char *query);

void
bare_module_register(bare_module_t *module);

#endif // BARE_MODULE_H
