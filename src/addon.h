#ifndef BARE_ADDON_H
#define BARE_ADDON_H

#include <napi.h>
#include <stdbool.h>
#include <uv.h>

#include "types.h"

const char *
bare_addon_resolve_static (bare_runtime_t *runtime, const char *specifier);

bare_module_t *
bare_addon_load_static (bare_runtime_t *runtime, const char *specifier);

bare_module_t *
bare_addon_load_dynamic (bare_runtime_t *runtime, const char *specifier);

bool
bare_addon_unload (bare_runtime_t *runtime, bare_module_t *mod);

#endif // BARE_ADDON_H
