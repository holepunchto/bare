#include <assert.h>
#include <bare.h>
#include <js.h>
#include <stdio.h>
#include <string.h>
#include <uv.h>

// Retrieve context from a dynamically loaded addon, which resolves
// `bare_context_get()` from the host as its library is loaded rather than
// having it linked in the way a statically registered addon does.

static int bare_test__value = 42;

static const char *code =
  "const url = require('bare-url')\n"
  "const { Addon } = Bare\n"
  "const addon = new Addon(\n"
  "  url.pathToFileURL(`./test/fixtures/context-addon/prebuilds/${Addon.host}/context-addon.bare`)\n"
  ")\n"
  "const { status, value, missing } = addon.exports\n"
  "if (status !== 0) throw new Error(`Context was not retrieved (${status})`)\n"
  "if (value !== 42) throw new Error('Context did not carry the published handle')\n"
  "if (missing !== -1) throw new Error(`Missing key was not reported as missing (${missing})`)\n";

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

  bare_t *bare;
  e = bare_setup(uv_default_loop(), platform, NULL, argc, (const char **) argv, NULL, &bare);
  assert(e == 0);

  e = bare_context_set(bare, "bare.test.value.v1", &bare_test__value, NULL);
  assert(e == 0);

  uv_buf_t source = uv_buf_init((char *) code, strlen(code));

  e = bare_load(bare, filename, &source, NULL);
  assert(e == 0);

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
