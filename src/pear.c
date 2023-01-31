#include <assert.h>
#include <js.h>
#include <uv.h>

#include "../include/pear.h"
#include "addons.h"
#include "runtime.h"

int
pear_setup (uv_loop_t *loop, pear_t *pear, int argc, char **argv) {
  pear_addons_init();

  pear->loop = loop;

  int err;

  err = js_create_platform(pear->loop, NULL, &pear->platform);
  assert(err == 0);

  err = js_create_env(pear->loop, pear->platform, &pear->env);
  assert(err == 0);

  pear->runtime.argc = argc;
  pear->runtime.argv = argv;

  err = pear_runtime_setup(pear);
  assert(err == 0);

  return 0;
}

int
pear_teardown (pear_t *pear, int *exit_code) {
  pear_runtime_teardown(pear, exit_code);

  js_destroy_env(pear->env);

  js_destroy_platform(pear->platform);

  return 0;
}

int
pear_run (pear_t *pear, const char *filename, const char *source) {
  int err;

  js_value_t *bootstrap;
  err = js_get_named_property(pear->env, pear->runtime.exports, "bootstrap", &bootstrap);
  assert(err == 0);

  js_value_t *args[2];
  err = js_create_string_utf8(pear->env, filename, -1, &args[0]);
  if (err < 0) return err;

  if (source) {
    err = js_create_string_utf8(pear->env, source, -1, &args[1]);
    if (err < 0) return err;
  } else {
    js_get_undefined(pear->env, &args[1]);
  }

  err = js_call_function(pear->env, pear->runtime.exports, bootstrap, 2, args, NULL);
  if (err < 0) return err;

  do {
    uv_run(pear->loop, UV_RUN_DEFAULT);

    pear_runtime_before_teardown(pear);
  } while (uv_loop_alive(pear->loop));

  return 0;
}
