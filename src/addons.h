#ifndef PEARJS_ADDONS_H
#define PEARJS_ADDONS_H

#include <uv.h>
#include <js.h>

int
pearjs_addons_find (uv_loop_t *loop, const char *path, char *out);

js_value_t *
pearjs_addons_load (js_env_t *env, const char *path);

#endif
