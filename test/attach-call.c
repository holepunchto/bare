#include <assert.h>
#include <bare.h>
#include <js.h>
#include <stdio.h>
#include <string.h>
#include <uv.h>

// Running the loop attaches the process, and so does loading an addon, so
// native code reached through either is already attached. Native code reached
// from a call the embedder makes itself is not, which is what attaching is
// for.

static int bare_test__value = 42;

static int bare_test__found = 1;
static void *bare_test__observed = NULL;

static js_value_t *
bare_test__lookup(js_env_t *env, js_callback_info_t *info) {
  (void) env;
  (void) info;

  bare_test__found = bare_context_get("bare.test.value.v1", &bare_test__observed);

  return NULL;
}

static js_value_t *
bare_test__exports(js_env_t *env, js_value_t *exports) {
  int e;

  js_value_t *lookup;
  e = js_create_function(env, "lookup", -1, bare_test__lookup, NULL, &lookup);
  assert(e == 0);

  e = js_set_named_property(env, exports, "lookup", lookup);
  assert(e == 0);

  return exports;
}

static const char *code =
  "const { Addon } = Bare\n"
  "const addon = new Addon(new URL('builtin:attach-call'))\n"
  "module.exports = function () {\n"
  "  addon.exports.lookup()\n"
  "}\n";

int
main(int argc, char *argv[]) {
  int e;

  argc = 0;
  argv = NULL;

  argv = uv_setup_args(argc, argv);

  bare_module_register(&(bare_module_t){
    .version = BARE_MODULE_VERSION,
    .name = "attach-call",
    .exports = bare_test__exports,
  });

  js_platform_t *platform;
  e = js_create_platform(uv_default_loop(), NULL, &platform);
  assert(e == 0);

  char filename[4096];
  size_t len = sizeof(filename);

  e = uv_cwd(filename, &len);
  assert(e == 0);

  e = snprintf(&filename[len], sizeof(filename) - len, "/test.js");
  assert(e > 0);

  js_env_t *env;

  bare_t *bare;
  e = bare_setup(uv_default_loop(), platform, &env, argc, (const char **) argv, NULL, &bare);
  assert(e == 0);

  e = bare_context_set(bare, "bare.test.value.v1", &bare_test__value, NULL);
  assert(e == 0);

  js_handle_scope_t *handles;
  e = js_open_handle_scope(env, &handles);
  assert(e == 0);

  uv_buf_t source = uv_buf_init((char *) code, strlen(code));

  js_value_t *module;
  e = bare_load(bare, filename, &source, &module);
  assert(e == 0);

  js_value_t *exports;
  e = js_get_named_property(env, module, "exports", &exports);
  assert(e == 0);

  js_value_t *receiver;
  e = js_get_undefined(env, &receiver);
  assert(e == 0);

  // Unattached, the call has no process to resolve against.
  e = js_call_function(env, receiver, exports, 0, NULL, NULL);
  assert(e == 0);

  assert(bare_test__found == -2);
  assert(bare_test__observed == NULL);

  bare_t *previous;
  e = bare_attach(bare, &previous);
  assert(e == 0);

  e = js_call_function(env, receiver, exports, 0, NULL, NULL);
  assert(e == 0);

  e = bare_detach(bare, previous);
  assert(e == 0);

  assert(bare_test__found == 0);
  assert(bare_test__observed == (void *) &bare_test__value);

  e = js_close_handle_scope(env, handles);
  assert(e == 0);

  // The call may leave work behind that only the loop will run.
  e = bare_run(bare, UV_RUN_DEFAULT);
  assert(e == 0);

  int exit_code = 0;

  e = bare_teardown(bare, UV_RUN_DEFAULT, &exit_code);
  assert(e == 0);
  assert(exit_code == 0);

  e = js_destroy_platform(platform);
  assert(e == 0);

  e = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
  assert(e == 0);

  return 0;
}
