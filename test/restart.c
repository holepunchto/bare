#include <assert.h>
#include <bare.h>
#include <js.h>
#include <string.h>
#include <uv.h>

int
main(int argc, char *argv[]) {
  int e;

  argc = 0;
  argv = NULL;

  argv = uv_setup_args(argc, argv);

  js_platform_t *platform;
  e = js_create_platform(uv_default_loop(), NULL, &platform);
  assert(e == 0);

  int exit_code = 0;

  do {
    bare_t *bare;
    e = bare_setup(uv_default_loop(), platform, NULL, argc, (const char **) argv, NULL, &bare);
    assert(e == 0);

    char *code;

    if (exit_code == 0) {
      code = "console.log('Restarting'); Bare.exit(1)";
    } else {
      code = "console.log('Restarted')";
    }

    uv_buf_t source = uv_buf_init(code, strlen(code));

    bare_load(bare, "/test.js", &source, NULL);

    e = bare_run(bare);
    assert(e == 0);

    e = bare_teardown(bare, &exit_code);
    assert(e == 0);
  } while (exit_code == 1);

  e = js_destroy_platform(platform);
  assert(e == 0);

  e = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
  assert(e == 0);

  return exit_code;
}
