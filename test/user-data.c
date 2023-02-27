#include <assert.h>
#include <pear.h>
#include <uv.h>

bool callback_called = false;

static js_value_t *
on_callback (js_env_t *env, js_callback_info_t *info) {
  callback_called = true;

  size_t argc = 1;
  js_value_t *argv[1];
  js_get_callback_info(env, info, &argc, argv, NULL, NULL);

  js_value_type_t type;
  js_typeof(env, argv[0], &type);

  assert(type == js_external);

  void *data;
  js_get_value_external(env, argv[0], &data);

  assert(data == (void *) 42);

  return NULL;
}

int
main (int argc, char *argv[]) {
  argv = uv_setup_args(argc, argv);

  pear_t pear;
  pear_setup(uv_default_loop(), &pear, argc, argv);

  pear_set_data_external(&pear, "hello", (void *) 42);

  js_value_t *global;
  js_get_global(pear.env, &global);

  js_value_t *fn;
  js_create_function(pear.env, "callback", -1, on_callback, NULL, &fn);
  js_set_named_property(pear.env, global, "callback", fn);

  pear_run(&pear, "./test/fixtures/user-data.js", NULL);

  assert(callback_called);

  int exit_code;
  pear_teardown(&pear, &exit_code);

  return exit_code;
}
