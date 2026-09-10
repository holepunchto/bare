#include <assert.h>
#include <bare.h>
#include <js.h>
#include <stdbool.h>
#include <uv.h>

int
main(int argc, char *argv[]) {
  int e;

  argv = uv_setup_args(argc, argv);

  js_platform_t *platform;
  e = js_create_platform(uv_default_loop(), NULL, &platform);
  assert(e == 0);

  js_env_t *env;

  bare_t *bare;
  e = bare_setup(uv_default_loop(), platform, &env, argc, (const char **) argv, NULL, &bare);
  assert(e == 0);

  uv_buf_t source = uv_buf_init("module.exports = 42", 19);

  js_handle_scope_t *scope;
  e = js_open_handle_scope(env, &scope);
  assert(e == 0);

  // Loading is synchronous and hands back the module itself, not a promise for
  // it, as the caller has nowhere to await one.

  js_value_t *result;
  e = bare_load(bare, "/test.js", &source, &result);
  assert(e == 0);

  bool is_promise;
  e = js_is_promise(env, result, &is_promise);
  assert(e == 0);

  assert(!is_promise);

  js_value_t *exports;
  e = js_get_named_property(env, result, "exports", &exports);
  assert(e == 0);

  int32_t value;
  e = js_get_value_int32(env, exports, &value);
  assert(e == 0);

  assert(value == 42);

  e = js_close_handle_scope(env, scope);
  assert(e == 0);

  e = bare_run(bare, UV_RUN_DEFAULT);
  assert(e == 0);

  int exit_code;
  e = bare_teardown(bare, UV_RUN_DEFAULT, &exit_code);
  assert(e == 0);

  e = js_destroy_platform(platform);
  assert(e == 0);

  e = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
  assert(e == 0);

  return exit_code;
}
