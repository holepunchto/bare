#include <assert.h>
#include <bare.h>
#include <js.h>
#include <stdio.h>
#include <string.h>
#include <uv.h>

// Context is scoped to the Bare process rather than to the operating system
// process, several of which may share one. Both processes are set up before
// either loads the addon, so a registry that was shared would refuse the second
// key as already published and hand the second process the first one's handle.

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

static void
bare_test__filename(const char *name, char *result, size_t len) {
  int e;

  size_t n = len;

  e = uv_cwd(result, &n);
  assert(e == 0);

  e = snprintf(&result[n], len - n, "/%s", name);
  assert(e > 0);
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

  bare_t *y;
  e = bare_setup(&loop, platform, NULL, argc, (const char **) argv, NULL, &y);
  assert(e == 0);

  e = bare_context_set(x, "bare.test.value.v1", &bare_test__values[0], NULL);
  assert(e == 0);

  // Published while the first process still holds the same key, which only one
  // registry per process makes room for.
  e = bare_context_set(y, "bare.test.value.v1", &bare_test__values[1], NULL);
  assert(e == 0);

  e = bare_load(x, first, &source, NULL);
  assert(e == 0);

  e = bare_load(y, second, &source, NULL);
  assert(e == 0);

  e = bare_run(x, UV_RUN_DEFAULT);
  assert(e == 0);

  e = bare_run(y, UV_RUN_DEFAULT);
  assert(e == 0);

  int exit_code = 0;

  e = bare_teardown(x, UV_RUN_DEFAULT, &exit_code);
  assert(e == 0);
  assert(exit_code == 0);

  e = bare_teardown(y, UV_RUN_DEFAULT, &exit_code);
  assert(e == 0);
  assert(exit_code == 0);

  e = uv_loop_close(&loop);
  assert(e == 0);

  // Each process resolved the handle it published for itself.
  assert(bare_test__loaded == 2);

  assert(bare_test__observed[0] == (void *) &bare_test__values[0]);
  assert(bare_test__observed[1] == (void *) &bare_test__values[1]);

  e = js_destroy_platform(platform);
  assert(e == 0);

  e = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
  assert(e == 0);

  return 0;
}
