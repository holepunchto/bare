#include <assert.h>
#include <bare.h>
#include <js.h>
#include <stdio.h>
#include <string.h>
#include <uv.h>

// Register a statically linked addon on a thread that has already loaded a
// dynamic one. Loading leaves no registration state behind on the thread, so
// the addon is registered as statically linked rather than attributed to the
// load that came before it and torn down along with it.

static js_value_t *
bare_test__exports(js_env_t *env, js_value_t *exports) {
  (void) env;

  return exports;
}

// Exported by the test so that it can be resolved through the library of an
// addon that was linked into it.
void
bare_test_register_symbol(void) {}

static const char *code =
  "const url = require('bare-url')\n"
  "const { Addon } = Bare\n"
  "const addon = new Addon(\n"
  "  url.pathToFileURL(`./test/fixtures/addon/prebuilds/${Addon.host}/addon.bare`)\n"
  ")\n"
  "if (addon.exports !== 'Hello from addon') throw new Error('Addon was not loaded')\n";

int
main(int argc, char *argv[]) {
  int e;

  argc = 0;
  argv = NULL;

  argv = uv_setup_args(argc, argv);

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

  uv_buf_t source = uv_buf_init((char *) code, strlen(code));

  bare_t *bare;
  e = bare_setup(uv_default_loop(), platform, NULL, argc, (const char **) argv, NULL, &bare);
  assert(e == 0);

  e = bare_load(bare, filename, &source, NULL);
  assert(e == 0);

  e = bare_run(bare, UV_RUN_DEFAULT);
  assert(e == 0);

  // Registered after the load rather than before it, which must neither pick up
  // the library of the addon that was loaded nor the process that loaded it.
  bare_module_register(&(bare_module_t){
    .version = BARE_MODULE_VERSION,
    .name = "late@1.0.0",
    .exports = bare_test__exports,
  });

  uv_lib_t *lib = bare_module_find("late@1");
  assert(lib != NULL);

  // The addon was linked into the test rather than into the library that the
  // process loaded, so the symbols of the test resolve through it.
  void *symbol;
  e = uv_dlsym(lib, "bare_test_register_symbol", &symbol);
  assert(e == 0);
  assert(symbol == (void *) bare_test_register_symbol);

  int exit_code = 0;

  e = bare_teardown(bare, UV_RUN_DEFAULT, &exit_code);
  assert(e == 0);
  assert(exit_code == 0);

  // Statically linked addons are owned by no process and so outlive the one
  // that happened to be running when they registered.
  assert(bare_module_find("late@1") != NULL);

  e = js_destroy_platform(platform);
  assert(e == 0);

  e = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
  assert(e == 0);

  return 0;
}
