#ifndef BARE_ADDON_H
#define BARE_ADDON_H

#include <napi.h>
#include <stdbool.h>
#include <uv.h>

#include "types.h"

js_value_t *
bare_addon_get_static(bare_runtime_t *runtime);

js_value_t *
bare_addon_get_dynamic(bare_runtime_t *runtime);

bare_addon_t *
bare_addon_load_static(bare_runtime_t *runtime, const char *specifier);

bare_addon_t *
bare_addon_load_dynamic(bare_runtime_t *runtime, const char *specifier);

bool
bare_addon_unload(bare_runtime_t *runtime, bare_addon_t *addon);

void
bare_addon_teardown(void);

#endif // BARE_ADDON_H
