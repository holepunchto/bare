#include <assert.h>
#include <bare.h>
#include <js.h>
#include <stdio.h>
#include <string.h>
#include <uv.h>

// Find a dynamically loaded addon from two processes running on the same
// thread. An addon is only ever found for the process that loaded it, so that
// a process can't bind to a library that another process may unload and a
// sealed process can't reach an addon that it never loaded.
//
// The lookup runs from a statically linked addon as that's the only place an
// addon can ask: the process is the one whose runtime is executing, which is
// well defined while an addon initializes but not on the thread at large.

static int bare_test__found = -1;

static js_value_t *
bare_test__probe(js_env_t *env, js_value_t *exports) {
  (void) env;

  bare_test__found = bare_module_find("addon@1") != NULL;

  return exports;
}

// Initializing the probe runs it again, so it reports on the process that
// loaded it rather than on the one that registered it.
static const char *probe =
  "const URL = require('bare-url')\n"
  "const { Addon } = Bare\n"
  "new Addon(new URL('builtin:probe@1.0.0'))\n";

static const char *load =
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

// The scripts are loaded from within the working directory so that they may
// require the modules they need. Each is given a distinct name as the module
// system will otherwise return the cached module rather than evaluate it again.
static void
bare_test__load(bare_t *bare, const char *name, const char *code) {
  int e;

  char filename[4096];

  bare_test__filename(name, filename, sizeof(filename));

  uv_buf_t source = uv_buf_init((char *) code, strlen(code));

  e = bare_load(bare, filename, &source, NULL);
  assert(e == 0);
}

static int
bare_test__probe_from(bare_t *bare, const char *name) {
  bare_test__found = -1;

  bare_test__load(bare, name, probe);

  assert(bare_test__found != -1);

  return bare_test__found;
}

int
main(int argc, char *argv[]) {
  int e;

  argc = 0;
  argv = NULL;

  argv = uv_setup_args(argc, argv);

  // Statically linked addons register before any process is set up.
  bare_module_register(&(bare_module_t){
    .version = BARE_MODULE_VERSION,
    .name = "probe@1.0.0",
    .exports = bare_test__probe,
  });

  js_platform_t *platform;
  e = js_create_platform(uv_default_loop(), NULL, &platform);
  assert(e == 0);

  uv_loop_t loop;
  e = uv_loop_init(&loop);
  assert(e == 0);

  bare_t *x;
  e = bare_setup(uv_default_loop(), platform, NULL, argc, (const char **) argv, NULL, &x);
  assert(e == 0);

  // Nothing has loaded the addon yet.
  assert(bare_test__probe_from(x, "first.js") == 0);

  bare_test__load(x, "second.js", load);

  // The first process has loaded the addon and so finds it.
  assert(bare_test__probe_from(x, "third.js") == 1);

  bare_t *y;
  e = bare_setup(&loop, platform, NULL, argc, (const char **) argv, NULL, &y);
  assert(e == 0);

  // The second process hasn't loaded the addon. The library is still loaded by
  // the first process, but it isn't the second process's to find.
  assert(bare_test__probe_from(y, "fourth.js") == 0);

  bare_test__load(y, "fifth.js", load);

  // The second process has now loaded the addon for itself.
  assert(bare_test__probe_from(y, "sixth.js") == 1);

  e = bare_run(x, UV_RUN_DEFAULT);
  assert(e == 0);

  int exit_code = 0;

  e = bare_teardown(x, UV_RUN_DEFAULT, &exit_code);
  assert(e == 0);
  assert(exit_code == 0);

  // Tearing down the first process unloads its copy of the addon, which must
  // leave the copy that the second process loaded alone. The first process was
  // also set up before the second, so tearing it down first must not leave the
  // thread without a process.
  assert(bare_test__probe_from(y, "seventh.js") == 1);

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
