#include <assert.h>
#include <bare.h>
#include <js.h>
#include <stdio.h>
#include <string.h>
#include <uv.h>

// Tear down a process that has loaded an addon registering a destructor. The
// destructor must run when the process is torn down and its addon is unloaded.

// Defined here and incremented by the addon's unload destructor, which resolves
// the symbol from the host at load time. See test/fixtures/teardown-addon.
int bare_test_addon_unload_count = 0;

static const char *code =
  "const url = require('bare-url')\n"
  "const { Addon } = Bare\n"
  "const addon = new Addon(\n"
  "  url.pathToFileURL(`./test/fixtures/teardown-addon/prebuilds/${Addon.host}/teardown-addon.bare`)\n"
  ")\n"
  "if (addon.exports !== 'Hello from teardown addon') throw new Error('Addon was not loaded')\n";

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

  // The addon is still loaded, so its destructor must not have run.
  assert(bare_test_addon_unload_count == 0);

  int exit_code = 0;

  e = bare_teardown(bare, UV_RUN_DEFAULT, &exit_code);
  assert(e == 0);
  assert(exit_code == 0);

  // Tearing down the process unloads the addon and runs its destructor.
  assert(bare_test_addon_unload_count == 1);

  e = js_destroy_platform(platform);
  assert(e == 0);

  e = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
  assert(e == 0);

  return 0;
}
