#include <assert.h>
#include <bare.h>
#include <js.h>
#include <stddef.h>
#include <stdlib.h>
#include <utf.h>

// Count the addon being unloaded, so that a test can observe that tearing down
// the owning process unloads the addon. The counter is published by the test
// through the process context rather than as a symbol of the host, which a
// prebuilt addon can't resolve on Windows.
static int *teardown_addon_unload_count = NULL;

static void
teardown_addon_on_unload(void) {
  if (teardown_addon_unload_count) (*teardown_addon_unload_count)++;
}

#if !defined(_WIN32)
__attribute__((destructor)) static void
teardown_addon_unload(void) {
  teardown_addon_on_unload();
}
#endif

static js_value_t *
teardown_addon_exports(js_env_t *env, js_value_t *exports) {
  int err;

  if (teardown_addon_unload_count == NULL) {
    err = bare_context_get("bare.test.unload-count.v1", (void **) &teardown_addon_unload_count);
    assert(err == 0);

#if defined(_WIN32)
    // The delay load hook that every addon links on Windows defines DllMain, so
    // count the unload with a handler that the C runtime of the addon runs when
    // the addon is unloaded instead.
    atexit(teardown_addon_on_unload);
#endif
  }

  err = js_create_string_utf8(env, (utf8_t *) "Hello from teardown addon", -1, &exports);
  assert(err == 0);

  return exports;
}

BARE_MODULE(teardown_addon, teardown_addon_exports)
