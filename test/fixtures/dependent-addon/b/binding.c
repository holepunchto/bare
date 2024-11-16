#include <assert.h>
#include <bare.h>
#include <js.h>

int
addon_a_fn (void);

static js_value_t *
addon_b_exports (js_env_t *env, js_value_t *exports) {
  int err = js_create_int32(env, addon_a_fn(), &exports);
  assert(err == 0);

  return exports;
}

BARE_MODULE(addon_b, addon_b_exports)
