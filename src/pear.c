#include <assert.h>
#include <js.h>
#include <uv.h>

#include "../include/pear.h"
#include "addons.h"

int
pear_setup (pear_t *env) {
  pear_addons_init();

  env->loop = uv_default_loop();

  int err;

  err = js_create_platform(env->loop, NULL, &env->platform);
  assert(err == 0);

  err = js_create_env(env->loop, env->platform, &env->env);
  assert(err == 0);

  return 0;
}

int
pear_teardown (pear_t *pear) {
  js_destroy_env(pear->env);

  js_destroy_platform(pear->platform);

  return 0;
}
