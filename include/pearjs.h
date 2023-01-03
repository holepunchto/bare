#ifndef PEARJS_H
#define PEARJS_H

typedef js_value_t * (*pearjs_addon_register)(js_env_t *env, js_value_t *exports);

typedef struct {
  const char *name;
  pearjs_addon_register register_addon;

  void *next_addon;
} pearjs_module_t;

void
pearjs_module_register (pearjs_module_t *mod);

#endif
