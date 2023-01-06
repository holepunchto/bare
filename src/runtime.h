#ifndef PEARJS_RUNTIME_H
#define PEARJS_RUNTIME_H

#include <uv.h>
#include <js.h>

int
pearjs_runtime_setup (js_env_t *env, const char *entry_point);

void
pearjs_runtime_teardown (js_env_t *env);

#endif
