#include <assert.h>
#include <bare.h>
#include <js.h>
#include <stddef.h>

// Report what the embedder published through the exports rather than through a
// symbol defined by the host, which a prebuilt addon can't resolve on Windows.

static js_value_t *
context_addon_exports(js_env_t *env, js_value_t *exports) {
  int err;

  void *value = NULL;

  int status = bare_context_get("bare.test.value.v1", &value);

  err = js_create_object(env, &exports);
  assert(err == 0);

  js_value_t *result;

  err = js_create_int32(env, status, &result);
  assert(err == 0);

  err = js_set_named_property(env, exports, "status", result);
  assert(err == 0);

  // Dereferenced so that the test observes the handle itself rather than only
  // that a lookup succeeded.
  err = js_create_int32(env, status == 0 ? *(int *) value : 0, &result);
  assert(err == 0);

  err = js_set_named_property(env, exports, "value", result);
  assert(err == 0);

  // A key that was never published is an ordinary miss, which is reported apart
  // from being asked from a thread that has entered no process.
  err = js_create_int32(env, bare_context_get("bare.test.missing.v1", NULL), &result);
  assert(err == 0);

  err = js_set_named_property(env, exports, "missing", result);
  assert(err == 0);

  return exports;
}

BARE_MODULE(context_addon, context_addon_exports)
