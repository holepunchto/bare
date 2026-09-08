#include <assert.h>
#include <bare.h>
#include <js.h>
#include <utf.h>

static js_value_t *
teardown_addon_exports(js_env_t *env, js_value_t *exports) {
  int err = js_create_string_utf8(env, (utf8_t *) "Hello from teardown addon", -1, &exports);
  assert(err == 0);

  return exports;
}

BARE_MODULE(teardown_addon, teardown_addon_exports)

#ifndef _WIN32
// Count the addon being unloaded from a destructor that runs when the library
// is closed. The counter is defined by the host and resolved at load time, so
// that a test can observe that tearing down the owning process unloads the
// addon. The addon must therefore only be loaded by a host that defines it.
extern int bare_test_addon_unload_count;

__attribute__((destructor)) static void
teardown_addon_unload(void) {
  bare_test_addon_unload_count++;
}
#endif
