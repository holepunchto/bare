#include <assert.h>
#include <bare.h>
#include <js.h>
#include <stdio.h>
#include <string.h>
#include <uv.h>

// Load an addon and seal the process, twice over. The seal is owned by the
// process and must not outlive it, so the second process must be able to load
// the addon again despite the first one having sealed.
static const char *code =
  "const url = require('bare-url')\n"
  "const { Addon } = Bare\n"
  "if (Addon.sealed) throw new Error('Process was sealed on startup')\n"
  "const addon = new Addon(\n"
  "  url.pathToFileURL(`./test/fixtures/addon/prebuilds/${Addon.host}/addon.bare`)\n"
  ")\n"
  "if (addon.exports !== 'Hello from addon') throw new Error('Addon was not loaded')\n"
  "Addon.seal()\n"
  "if (!Addon.sealed) throw new Error('Process was not sealed')\n";

int
main(int argc, char *argv[]) {
  int e;

  argc = 0;
  argv = NULL;

  argv = uv_setup_args(argc, argv);

  js_platform_t *platform;
  e = js_create_platform(uv_default_loop(), NULL, &platform);
  assert(e == 0);

  // Run the script from within the working directory so that it may require
  // the modules it needs.
  char filename[4096];
  size_t len = sizeof(filename);

  e = uv_cwd(filename, &len);
  assert(e == 0);

  e = snprintf(&filename[len], sizeof(filename) - len, "/test.js");
  assert(e > 0);

  for (int i = 0; i < 2; i++) {
    bare_t *bare;
    e = bare_setup(uv_default_loop(), platform, NULL, argc, (const char **) argv, NULL, &bare);
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
  }

  e = js_destroy_platform(platform);
  assert(e == 0);

  e = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
  assert(e == 0);

  return 0;
}
