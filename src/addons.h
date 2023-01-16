#ifndef PEAR_ADDONS_H
#define PEAR_ADDONS_H

#include <uv.h>
#include <js.h>

int
pear_addons_resolve (uv_loop_t *loop, const char *path, char *out);

js_value_t *
pear_addons_load (js_env_t *env, const char *path, bool resolve);

#endif
