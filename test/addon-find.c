#include <assert.h>
#include <bare.h>
#include <js.h>
#include <stddef.h>

// Find statically registered addons by name. An addon may be found by a
// truncated version so that it can be looked up by its major version alone,
// which is how the delay loader on Windows resolves the addons that another
// addon links against. The truncation must fall on a version component boundary
// so that a major version doesn't match every major version it prefixes.

static js_value_t *
bare_test__exports(js_env_t *env, js_value_t *exports) {
  (void) env;

  return exports;
}

static void
bare_test__register(const char *name) {
  bare_module_register(&(bare_module_t){
    .version = BARE_MODULE_VERSION,
    .name = name,
    .exports = bare_test__exports,
  });
}

int
main(void) {
  // Statically linked addons register before any process is set up.
  bare_test__register("find-one@1.2.3");
  bare_test__register("find-ten@10.0.0");

  // The full name always matches, with or without the `.bare` suffix that the
  // delay loader queries by.
  assert(bare_module_find("find-one@1.2.3") != NULL);
  assert(bare_module_find("find-one@1.2.3.bare") != NULL);

  // The version may be truncated at a component boundary.
  assert(bare_module_find("find-one@1.2") != NULL);
  assert(bare_module_find("find-one@1") != NULL);
  assert(bare_module_find("find-one@1.bare") != NULL);

  // It may not be truncated within a component. Without this, an addon would
  // match every major version that its own is a prefix of.
  assert(bare_module_find("find-ten@1") == NULL);
  assert(bare_module_find("find-ten@1.bare") == NULL);
  assert(bare_module_find("find-ten@10") != NULL);
  assert(bare_module_find("find-ten@10.0.0") != NULL);

  // The name is separated from the version by `@` rather than by `.`, so an
  // addon can't be found without one.
  assert(bare_module_find("find-one") == NULL);
  assert(bare_module_find("find-one.bare") == NULL);

  // Nor can it be found by a name that it merely begins with.
  assert(bare_module_find("find-o") == NULL);
  assert(bare_module_find("find") == NULL);

  // A version that no addon carries matches nothing.
  assert(bare_module_find("find-one@2") == NULL);
  assert(bare_module_find("find-one@1.3") == NULL);

  // Neither does a query that is nothing but the suffix.
  assert(bare_module_find(".bare") == NULL);

  return 0;
}
