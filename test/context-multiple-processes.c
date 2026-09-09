#include <assert.h>
#include <bare.h>
#include <js.h>
#include <stdio.h>
#include <string.h>
#include <uv.h>

// Context is scoped to the Bare process rather than to the operating system
// process, several of which may share one. An addon loaded by two processes
// sees what each of them published and nothing of the other.

static int bare_test__values[2] = {1, 2};

static int bare_test__loaded = 0;
static void *bare_test__observed[2] = {NULL, NULL};

static js_value_t *
bare_test__exports(js_env_t *env, js_value_t *exports) {
  (void) env;

  int e = bare_context_get("bare.test.value.v1", &bare_test__observed[bare_test__loaded++]);
  assert(e == 0);

  return exports;
}

static const char *code =
  "const { Addon } = Bare\n"
  "new Addon(new URL('builtin:context-addon-multiple-processes'))\n";

int
main(int argc, char *argv[]) {
  int e;

  argc = 0;
  argv = NULL;

  argv = uv_setup_args(argc, argv);

  bare_module_register(&(bare_module_t){
    .version = BARE_MODULE_VERSION,
    .name = "context-addon-multiple-processes",
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

  for (int i = 0; i < 2; i++) {
    bare_t *bare;
    e = bare_setup(uv_default_loop(), platform, NULL, argc, (const char **) argv, NULL, &bare);
    assert(e == 0);

    e = bare_context_set(bare, "bare.test.value.v1", &bare_test__values[i], NULL);
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

  assert(bare_test__loaded == 2);

  assert(bare_test__observed[0] == (void *) &bare_test__values[0]);
  assert(bare_test__observed[1] == (void *) &bare_test__values[1]);

  e = js_destroy_platform(platform);
  assert(e == 0);

  e = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
  assert(e == 0);

  return 0;
}
