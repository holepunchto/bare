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
typedef struct bare_addon_s bare_addon_t;

typedef enum {
  bare_runtime_state_active = 0,
  bare_runtime_state_suspending = 1,
  bare_runtime_state_idle = 2,
  bare_runtime_state_suspended = 3,
  bare_runtime_state_awake = 4,
  bare_runtime_state_terminated = 5,
  bare_runtime_state_exiting = 6,
  bare_runtime_state_exited = 7,
} bare_runtime_state_t;

struct bare_runtime_s {
  uv_loop_t *loop;

  bare_process_t *process;
  bare_thread_t *threads;

  js_env_t *env;
  js_ref_t *exports;

  uv_async_t signal;
  uv_timer_t timeout;
  uv_mutex_t lock;

  bare_runtime_state_t state;

  int linger;
  int deadline;

  struct {
    bool suspend;
    bool resuspend;
    bool wakeup;
    bool resume;
    bool terminate;
  } transitions;
};

struct bare_process_s {
  bare_runtime_t runtime;

  bare_options_t options;

  js_platform_t *platform;

  struct {
    bare_before_exit_cb before_exit;
    void *before_exit_data;

    bare_exit_cb exit;
    void *exit_data;

    bare_suspend_cb suspend;
    void *suspend_data;

    bare_wakeup_cb wakeup;
    void *wakeup_data;

    bare_idle_cb idle;
    void *idle_data;

    bare_resume_cb resume;
    void *resume_data;

    bare_thread_cb thread;
    void *thread_data;
  } callbacks;

  int argc;
  char *argv[];
};

struct bare_s {
  bare_process_t process;
};

struct bare_source_s {
  enum {
    bare_source_none,
    bare_source_buffer,
    bare_source_sharedarraybuffer,
  } type;

  union {
    uv_buf_t buffer;
    js_arraybuffer_backing_store_t *backing_store;
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
  bare_process_t *process;
  bare_runtime_t *runtime;

  uv_thread_t id;
  uv_mutex_t lock;
  uv_barrier_t *ready;

  const char *filename;
  bare_source_t *source;
  bare_data_t *data;

  bool exited;

  bare_thread_t *previous;
  bare_thread_t *next;
};

struct bare_addon_s {
  char *name;
  char *specifier;

  bare_module_register_cb exports;

  uv_lib_t lib;

  bare_addon_t *next;
};

#endif // BARE_TYPES_H
