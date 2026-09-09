#include <assert.h>
#include <bare.h>
#include <js.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <uv.h>

// Publish handles that only the embedder can produce and retrieve them from an
// addon as it initialises, which is the point at which an addon needs them and
// has nothing but the thread it runs on to identify the process that loaded it.

static int bare_test__value = 42;

static int bare_test__found;
static void *bare_test__observed = NULL;

static int bare_test__missing;

static int bare_test__destroyed = 0;
static char bare_test__destroyed_key[64];

static js_value_t *
bare_test__exports(js_env_t *env, js_value_t *exports) {
  (void) env;

  bare_test__found = bare_context_get("bare.test.value.v1", &bare_test__observed);

  void *missing = NULL;

  bare_test__missing = bare_context_get("bare.test.missing.v1", &missing);

  assert(missing == NULL);

  return exports;
}

static void
bare_test__on_destroy(const char *key, void *value) {
  assert(value == (void *) &bare_test__value);

  bare_test__destroyed++;

  // The key is only alive for the duration of the call.
  snprintf(bare_test__destroyed_key, sizeof(bare_test__destroyed_key), "%s", key);
}

static const char *code =
  "const { Addon } = Bare\n"
  "new Addon(new URL('builtin:context-addon'))\n";

int
main(int argc, char *argv[]) {
  int e;

  argc = 0;
  argv = NULL;

  argv = uv_setup_args(argc, argv);

  bare_module_register(&(bare_module_t){
    .version = BARE_MODULE_VERSION,
    .name = "context-addon",
    .exports = bare_test__exports,
  });

  js_platform_t *platform;
  e = js_create_platform(uv_default_loop(), NULL, &platform);
  assert(e == 0);

  // No process is running on the thread, so there is nothing to resolve a
  // lookup against.
  void *value = NULL;
  assert(bare_context_get("bare.test.value.v1", &value) < 0);
  assert(value == NULL);

  bare_t *bare;
  e = bare_setup(uv_default_loop(), platform, NULL, argc, (const char **) argv, NULL, &bare);
  assert(e == 0);

  e = bare_context_set(bare, "bare.test.value.v1", &bare_test__value, bare_test__on_destroy);
  assert(e == 0);

  // Entries are immutable, so a key that is already published may not be taken
  // by another value.
  e = bare_context_set(bare, "bare.test.value.v1", NULL, NULL);
  assert(e < 0);

  // Deleting an entry that was never published is an ordinary failure.
  e = bare_context_delete(bare, "bare.test.missing.v1");
  assert(e < 0);

  // Published and deleted again before anything was loaded, which runs the
  // destructor with the key it was published under.
  e = bare_context_set(bare, "bare.test.transient.v1", &bare_test__value, bare_test__on_destroy);
  assert(e == 0);

  e = bare_context_delete(bare, "bare.test.transient.v1");
  assert(e == 0);

  assert(bare_test__destroyed == 1);
  assert(strcmp(bare_test__destroyed_key, "bare.test.transient.v1") == 0);

  // Run the script from within the working directory so that it may require the
  // modules it needs.
  char filename[4096];
  size_t len = sizeof(filename);

  e = uv_cwd(filename, &len);
  assert(e == 0);

  e = snprintf(&filename[len], sizeof(filename) - len, "/test.js");
  assert(e > 0);

  uv_buf_t source = uv_buf_init((char *) code, strlen(code));

  e = bare_load(bare, filename, &source, NULL);
  assert(e == 0);

  e = bare_run(bare, UV_RUN_DEFAULT);
  assert(e == 0);

  // The addon saw what the process published before it was loaded, and saw
  // nothing under a key that was never published.
  assert(bare_test__found == 0);
  assert(bare_test__observed == (void *) &bare_test__value);
  assert(bare_test__missing < 0);

  int exit_code = 0;

  e = bare_teardown(bare, UV_RUN_DEFAULT, &exit_code);
  assert(e == 0);
  assert(exit_code == 0);

  // Tearing down the process destroys the entry that was still published.
  assert(bare_test__destroyed == 2);
  assert(strcmp(bare_test__destroyed_key, "bare.test.value.v1") == 0);

  e = js_destroy_platform(platform);
  assert(e == 0);

  e = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
  assert(e == 0);

  return 0;
}
