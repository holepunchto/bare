#ifndef BARE_ADDON_H
#define BARE_ADDON_H

#include <js.h>
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

bare_process_t *
bare_addon_attach(bare_runtime_t *runtime);

void
bare_addon_detach(bare_process_t *previous);

void
bare_addon_seal(bare_process_t *process);

bool
bare_addon_sealed(bare_process_t *process);

void
bare_addon_teardown(bare_process_t *process);

#endif // BARE_ADDON_H
