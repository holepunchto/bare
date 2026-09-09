#include <assert.h>
#include <bare.h>
#include <js.h>
#include <stdio.h>
#include <string.h>
#include <uv.h>

// Threads share the process that spawned them, so an addon initialising on one
// resolves against the same registry as it would on the main thread.

static int bare_test__value = 42;

static uv_mutex_t bare_test__lock;

static int bare_test__loaded = 0;
static void *bare_test__observed[2] = {NULL, NULL};

static js_value_t *
bare_test__exports(js_env_t *env, js_value_t *exports) {
  (void) env;

  uv_mutex_lock(&bare_test__lock);

  int e = bare_context_get("bare.test.value.v1", &bare_test__observed[bare_test__loaded++]);
  assert(e == 0);

  uv_mutex_unlock(&bare_test__lock);

  return exports;
}

static const char *code =
  "const { Addon, Thread } = Bare\n"
  "new Addon(new URL('builtin:context-addon-thread'))\n"
  "const thread = new Thread('/thread.js', `\n"
  "  const { Addon } = Bare\n"
  "  new Addon(new URL('builtin:context-addon-thread'))\n"
  "`)\n"
  "thread.join()\n";

int
main(int argc, char *argv[]) {
  int e;

  argc = 0;
  argv = NULL;

  argv = uv_setup_args(argc, argv);

  e = uv_mutex_init(&bare_test__lock);
  assert(e == 0);

  bare_module_register(&(bare_module_t){
    .version = BARE_MODULE_VERSION,
    .name = "context-addon-thread",
    .exports = bare_test__exports,
  });

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

  // Both the main thread and the spawned thread resolved the same handle.
  assert(bare_test__loaded == 2);
  assert(bare_test__observed[0] == (void *) &bare_test__value);
  assert(bare_test__observed[1] == (void *) &bare_test__value);

  e = js_destroy_platform(platform);
  assert(e == 0);

  e = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
  assert(e == 0);

  uv_mutex_destroy(&bare_test__lock);

  return 0;
}
