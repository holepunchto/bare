#include <assert.h>
#include <bare.h>
#include <js.h>
#include <stdio.h>
#include <string.h>
#include <uv.h>

// Load an addon that registers a destructor into two processes and tear them
// down one at a time. Each process loads the addon for itself, so tearing down
// the first must not unload it from underneath the second; the destructor must
// run only once the last owner is gone.

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

static void
bare_test__filename(const char *name, char *result, size_t len) {
  int e;

  size_t n = len;

  e = uv_cwd(result, &n);
  assert(e == 0);

  e = snprintf(&result[n], len - n, "/%s", name);
  assert(e > 0);
}

int
main(int argc, char *argv[]) {
  int e;

  argc = 0;
  argv = NULL;

  argv = uv_setup_args(argc, argv);

  js_platform_t *platform;
  e = js_create_platform(uv_default_loop(), NULL, &platform);
  assert(e == 0);

  uv_loop_t loop;
  e = uv_loop_init(&loop);
  assert(e == 0);

  // The scripts are loaded from within the working directory so that they may
  // require the modules they need. Each is given a distinct name as the module
  // system will otherwise return the cached module rather than evaluate it
  // again.
  char first[4096], second[4096];

  bare_test__filename("first.js", first, sizeof(first));
  bare_test__filename("second.js", second, sizeof(second));

  uv_buf_t source = uv_buf_init((char *) code, strlen(code));

  bare_t *x;
  e = bare_setup(uv_default_loop(), platform, NULL, argc, (const char **) argv, NULL, &x);
  assert(e == 0);

  e = bare_load(x, first, &source, NULL);
  assert(e == 0);

  bare_t *y;
  e = bare_setup(&loop, platform, NULL, argc, (const char **) argv, NULL, &y);
  assert(e == 0);

  e = bare_load(y, second, &source, NULL);
  assert(e == 0);

  e = bare_run(x, UV_RUN_DEFAULT);
  assert(e == 0);

  int exit_code = 0;

  e = bare_teardown(x, UV_RUN_DEFAULT, &exit_code);
  assert(e == 0);
  assert(exit_code == 0);

  // The second process still owns the addon, so tearing down the first must not
  // have unloaded it.
  assert(bare_test_addon_unload_count == 0);

  e = bare_run(y, UV_RUN_DEFAULT);
  assert(e == 0);

  e = bare_teardown(y, UV_RUN_DEFAULT, &exit_code);
  assert(e == 0);
  assert(exit_code == 0);

  // The last owner is gone, so the addon is now unloaded exactly once.
  assert(bare_test_addon_unload_count == 1);

  e = uv_loop_close(&loop);
  assert(e == 0);

  e = js_destroy_platform(platform);
  assert(e == 0);

  e = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
  assert(e == 0);

  return 0;
}
