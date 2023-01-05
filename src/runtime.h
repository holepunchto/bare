#ifndef PEARJS_RUNTIME_H
#define PEARJS_RUNTIME_H

#include <uv.h>
#include <js.h>

int
pearjs_runtime_setup (uv_loop_t *loop, js_env_t *env, const char *entry_point);

#endif
