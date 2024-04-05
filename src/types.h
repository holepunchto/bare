#ifndef BARE_TYPES_H
#define BARE_TYPES_H

#include <js.h>
#include <stdbool.h>
#include <uv.h>

#include "../include/bare.h"

typedef struct bare_runtime_s bare_runtime_t;
typedef struct bare_process_s bare_process_t;
typedef struct bare_source_s bare_source_t;
typedef struct bare_data_s bare_data_t;
typedef struct bare_thread_s bare_thread_t;
typedef struct bare_thread_list_s bare_thread_list_t;
typedef struct bare_module_list_s bare_module_list_t;

struct bare_runtime_s {
  uv_loop_t *loop;

  bare_process_t *process;

  js_env_t *env;
  js_ref_t *exports;

  struct {
    uv_async_t suspend;
    uv_async_t resume;
  } signals;

  int active_handles;

  bool suspended;
  bool exiting;
};

struct bare_process_s {
  bare_runtime_t *runtime;

  bare_options_t options;

  js_platform_t *platform;

  int argc;
  char **argv;

  bare_before_exit_cb on_before_exit;
  bare_exit_cb on_exit;
  bare_teardown_cb on_teardown;
  bare_suspend_cb on_suspend;
  bare_idle_cb on_idle;
  bare_resume_cb on_resume;
  bare_thread_cb on_thread;
};

struct bare_source_s {
  enum {
    bare_source_none,
    bare_source_buffer,
  } type;

  union {
    uv_buf_t buffer;
  };
};

struct bare_data_s {
  enum {
    bare_data_none,
    bare_data_sharedarraybuffer,
  } type;

  union {
    js_arraybuffer_backing_store_t *backing_store;
  };
};

struct bare_thread_s {
  bare_runtime_t *runtime;

  uv_thread_t id;
  uv_sem_t lock;

  char *filename;

  bare_source_t source;
  bare_data_t data;

  bool exited;
};

struct bare_thread_list_s {
  bare_thread_t thread;

  bare_thread_list_t *previous;
  bare_thread_list_t *next;
};

struct bare_module_list_s {
  bare_module_t mod;
  char *resolved;
  bool pending;
  int refs;
  uv_lib_t *lib;

  bare_module_list_t *previous;
  bare_module_list_t *next;
};

#endif // BARE_TYPES_H
