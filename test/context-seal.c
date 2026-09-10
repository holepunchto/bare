#include <assert.h>
#include <bare.h>
#include <js.h>
#include <stdio.h>
#include <string.h>
#include <uv.h>

// Sealing a process freezes its context registry along with its addons, so that
// one call establishes the whole boundary. What was published beforehand stays
// readable; nothing may be published after. The seal is a single process-wide
// flag, so neither half can be sealed without the other.

static int bare_test__value = 42;

static int bare_test__found;
static void *bare_test__observed = NULL;

static js_value_t *
bare_test__exports(js_env_t *env, js_value_t *exports) {
  (void) env;

  bare_test__found = bare_context_get("bare.test.value.v1", &bare_test__observed);

  return exports;
}

// Sealed from JavaScript, which must freeze the registry just as `bare_seal()`
// does, the two being documented as equivalent.
static const char *code =
  "const { Addon } = Bare\n"
  "Addon.seal()\n"
  "new Addon(new URL('builtin:context-addon-seal'))\n";

int
main(int argc, char *argv[]) {
  int e;

  argc = 0;
  argv = NULL;

  argv = uv_setup_args(argc, argv);

  bare_module_register(&(bare_module_t){
    .version = BARE_MODULE_VERSION,
    .name = "context-addon-seal",
    .exports = bare_test__exports,
  });

  js_platform_t *platform;
  e = js_create_platform(uv_default_loop(), NULL, &platform);
  assert(e == 0);

  // Run the script from within the working directory so that it may require the
  // modules it needs.
  char filename[4096];
  size_t len = sizeof(filename);

  e = uv_cwd(filename, &len);
  assert(e == 0);

  e = snprintf(&filename[len], sizeof(filename) - len, "/test.js");
  assert(e > 0);

  bare_t *bare;
  e = bare_setup(uv_default_loop(), platform, NULL, argc, (const char **) argv, NULL, &bare);
  assert(e == 0);

  e = bare_context_set(bare, "bare.test.value.v1", &bare_test__value, NULL);
  assert(e == 0);

  e = bare_seal(bare);
  assert(e == 0);

  // Nothing may be published once the process is sealed.
  e = bare_context_set(bare, "bare.test.late.v1", &bare_test__value, NULL);
  assert(e < 0);

  uv_buf_t source = uv_buf_init((char *) code, strlen(code));

  e = bare_load(bare, filename, &source, NULL);
  assert(e == 0);

  e = bare_run(bare, UV_RUN_DEFAULT);
  assert(e == 0);

  // A statically linked addon remains loadable when sealed, and still sees what
  // was published before the seal.
  assert(bare_test__found == 0);
  assert(bare_test__observed == (void *) &bare_test__value);

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
