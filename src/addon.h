#ifndef BARE_ADDON_H
#define BARE_ADDON_H

#include <js.h>
#include <napi.h>
#include <stdbool.h>
#include <uv.h>

#include "../include/bare.h"

#include "types.h"

bare_module_t *
bare_addon_load_static (js_env_t *env, const char *specifier);

bare_module_t *
bare_addon_load_dynamic (js_env_t *env, const char *specifier);

bool
bare_addon_unload (js_env_t *env, bare_module_t *mod);

void
bare_module_register (bare_module_t *mod);

void
napi_module_register (napi_module *mod);

#endif // BARE_ADDON_H
