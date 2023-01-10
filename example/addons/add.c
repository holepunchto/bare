#include <js.h>
#include <js/ffi.h>
#include <pearjs.h>

static uint32_t
add_numbers_fast (uint32_t a, uint32_t b) {
  return a + b;
}

static js_value_t *
add_numbers (js_env_t *env, js_callback_info_t *info) {
  js_value_t *argv[2];
  size_t argc = 2;

  js_get_callback_info(env, info, &argc, argv, NULL, NULL);

  uint32_t a;
  uint32_t b;

  js_get_value_uint32(env, argv[0], &a);
  js_get_value_uint32(env, argv[1], &b);

  js_value_t *result;
  js_create_uint32(env, a + b, &result);

  return result;
}

static js_value_t *
pearjs_init (js_env_t *env, js_value_t *exports) {
  {
    js_value_t *val;
    js_create_function(env, "addNumbersNativeSlow", -1, add_numbers, NULL, &val);
    js_set_named_property(env, exports, "addNumbersNativeSlow", val);
  }

  {
    js_ffi_type_info_t *return_info;
    js_ffi_create_type_info(js_ffi_uint32, &return_info);

    js_ffi_type_info_t *arg_info[2];
    js_ffi_create_type_info(js_ffi_uint32, &(arg_info[0]));
    js_ffi_create_type_info(js_ffi_uint32, &(arg_info[1]));

    js_ffi_function_info_t *function_info;
    js_ffi_create_function_info(return_info, arg_info, 2, &function_info);

    js_ffi_function_t *ffi;
    js_ffi_create_function(add_numbers_fast, function_info, &ffi);

    js_value_t *val;
    js_create_function_with_ffi(env, "addNumbersNativeFast", -1, add_numbers, NULL, ffi, &val);
    js_set_named_property(env, exports, "addNumbersNativeFast", val);
  }

  return exports;
}

PEARJS_MODULE("add_numbers", pearjs_init)
