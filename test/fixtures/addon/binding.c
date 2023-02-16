#include <assert.h>
#include <js.h>
#include <pear.h>

static js_value_t *
init (js_env_t *env, js_value_t *exports) {
  int err = js_create_string_utf8(env, "Hello from addon", -1, &exports);
  assert(err == 0);

  return exports;
}

PEAR_MODULE(init)
