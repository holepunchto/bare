#ifndef BARE_ADDON_H
#define BARE_ADDON_H

#include <js.h>
#include <uv.h>

#include "types.h"

bare_addon_t *
bare_addon_load_static(bare_runtime_t *runtime, const char *specifier);

bare_addon_t *
bare_addon_load_dynamic(bare_runtime_t *runtime, const char *specifier);

void
bare_addon_teardown(void);

#endif // BARE_ADDON_H
