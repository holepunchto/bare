#include <assert.h>
#include <bare.h>
#include <js.h>
#include <stdio.h>
#include <string.h>
#include <uv.h>

// A handle returned by `bare_module_find()` must stay valid after the runtime
// that loaded the addon is torn down. Looking the addon up pins it, so its node
// is kept alive rather than freed, and the returned handle does not dangle.

static const char *code =
  "const url = require('bare-url')\n"
  "const { Addon } = Bare\n"
  "const a = new Addon(url.pathToFileURL(`./test/fixtures/addon/prebuilds/${Addon.host}/addon.bare`))\n"
  "if (a.exports !== 'Hello from addon') throw new Error('Addon was not loaded')\n";

int
main(int argc, char *argv[]) {
  int e;

  argc = 0;
  argv = NULL;

  argv = uv_setup_args(argc, argv);

  js_platform_t *platform;
  e = js_create_platform(uv_default_loop(), NULL, &platform);
  assert(e == 0);

  char filename[4096];
  size_t len = sizeof(filename);

  e = uv_cwd(filename, &len);
  assert(e == 0);

  e = snprintf(&filename[len], sizeof(filename) - len, "/test.js");
  assert(e > 0);

  bare_t *bare;
  e = bare_setup(uv_default_loop(), platform, NULL, argc, (const char **) argv, NULL, &bare);
  assert(e == 0);

  uv_buf_t source = uv_buf_init((char *) code, strlen(code));

  e = bare_load(bare, filename, &source, NULL);
  assert(e == 0);

  uv_lib_t *lib = bare_module_find("addon");
  assert(lib != NULL);

  void *handle = lib->handle;
  assert(handle != NULL);

  e = bare_run(bare, UV_RUN_DEFAULT);
  assert(e == 0);

  int exit_code = 0;

  e = bare_teardown(bare, UV_RUN_DEFAULT, &exit_code);
  assert(e == 0);
  assert(exit_code == 0);

  // The addon was pinned, so its node was not freed: the handle is unchanged and
  // still resolves symbols, and the addon is still discoverable.
  assert(lib->handle == handle);
  assert(bare_module_find("addon") != NULL);

  void *sym = NULL;
  e = uv_dlsym(lib, "bare_register_module_v0", &sym);
  assert(e == 0);
  assert(sym != NULL);

  e = js_destroy_platform(platform);
  assert(e == 0);

  e = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
  assert(e == 0);

  return 0;
}
