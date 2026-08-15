#include <assert.h>
#include <bare.h>
#include <js.h>
#include <stdio.h>
#include <string.h>
#include <uv.h>

// Load the same addon into two processes and tear down the first while the
// second is still running. Each process loads the addon for itself, so the
// first tearing down must not unload the addon from underneath the second.
static const char *code =
  "const url = require('bare-url')\n"
  "const { Addon } = Bare\n"
  "const addon = new Addon(\n"
  "  url.pathToFileURL(`./test/fixtures/addon/prebuilds/${Addon.host}/addon.bare`)\n"
  ")\n"
  "if (addon.exports !== 'Hello from addon') throw new Error('Addon was not loaded')\n";

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
  char first[4096], second[4096], third[4096];

  bare_test__filename("first.js", first, sizeof(first));
  bare_test__filename("second.js", second, sizeof(second));
  bare_test__filename("third.js", third, sizeof(third));

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

  // Load the addon again now that the first process is gone. The second
  // process already owns the addon and so will reuse it, calling back into the
  // library to initialize its exports.
  e = bare_load(y, third, &source, NULL);
  assert(e == 0);

  e = bare_run(y, UV_RUN_DEFAULT);
  assert(e == 0);

  e = bare_teardown(y, UV_RUN_DEFAULT, &exit_code);
  assert(e == 0);
  assert(exit_code == 0);

  e = uv_loop_close(&loop);
  assert(e == 0);

  e = js_destroy_platform(platform);
  assert(e == 0);

  e = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
  assert(e == 0);

  return 0;
}
