#ifndef PEAR_ADDONS_H
#define PEAR_ADDONS_H

#include <uv.h>
#include <js.h>
#include <stdbool.h>

#define PEAR_ADDONS_DYNAMIC 1
#define PEAR_ADDONS_STATIC 2
#define PEAR_ADDONS_RESOLVE 4

void
pear_addons_set_static (bool is_static);

int
pear_addons_resolve (uv_loop_t *loop, const char *path, char *out);

js_value_t *
pear_addons_load (js_env_t *env, const char *path, int mode);

#endif
