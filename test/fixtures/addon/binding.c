#include <assert.h>
#include <bare.h>
#include <js.h>
#include <utf.h>

static js_value_t *
init (js_env_t *env, js_value_t *exports) {
  int err = js_create_string_utf8(env, (utf8_t *) "Hello from addon", -1, &exports);
  assert(err == 0);

  return exports;
}

BARE_MODULE(test_binding, init)
