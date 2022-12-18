#include <stdio.h>
#include <js.h>
#include <uv.h>

static js_ref_t *on_open;

typedef struct {
  uv_fs_t req;
  js_env_t *env;
  uint32_t id;
} pear_fs_req_t;

static void
on_fs_open (uv_fs_t *req) {
  pear_fs_req_t *p = (pear_fs_req_t *) req;

  js_handle_scope_t *scope;
  js_open_handle_scope(p->env, &scope);

  js_value_t *cb;
  js_get_reference_value(p->env, on_open, &cb);

  js_value_t *global;
  js_get_global(p->env, &global);

  js_value_t *argv[2];
  js_create_uint32(p->env, p->id, &(argv[0]));
  js_create_int32(p->env, req->result, &(argv[1]));

  uv_fs_req_cleanup(req);

  js_value_t *res;
  js_call_function(p->env, global, cb, 2, (const js_value_t **) argv, &res);

  js_close_handle_scope(p->env, scope);
}

static js_value_t *
fs_init (js_env_t *env, const js_callback_info_t *info) {
  js_value_t *argv[1];
  size_t argc = 1;

  js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  js_create_reference(env, argv[0], 1, &on_open);

  return NULL;
}

static js_value_t *
fs_open (js_env_t *env, const js_callback_info_t *info) {
  js_value_t *argv[2];
  size_t argc = 2;

  js_get_callback_info(env, info, &argc, argv, NULL, NULL);

  pear_fs_req_t *req;
  size_t req_len;

  js_get_typedarray_info(env, argv[0], NULL, &req_len, (void **) &req, NULL, NULL);

  char *name;
  size_t name_len;

  js_get_typedarray_info(env, argv[1], NULL, &name_len, (void **) &name, NULL, NULL);

  uv_loop_t *loop;
  js_get_env_loop(env, &loop);

  req->env = env;

  uv_fs_open(loop, (uv_fs_t *) req, name, UV_FS_O_RDONLY, 0, on_fs_open);

  return NULL;
}

void bootstrap_fs (js_env_t *env, js_value_t *addon) {
  {
    js_value_t *fn;
    js_create_function(env, "init", -1, fs_init, NULL, &fn);
    js_set_named_property(env, addon, "init", fn);
  }

  {
    js_value_t *fn;
    js_create_function(env, "open", -1, fs_open, NULL, &fn);
    js_set_named_property(env, addon, "open", fn);
  }

  {
    js_value_t *val;
    js_create_uint32(env, sizeof(pear_fs_req_t), &val);
    js_set_named_property(env, addon, "size_of_pearfs_req_t", val);
  }
}
