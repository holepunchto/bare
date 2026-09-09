#ifndef BARE_H
#define BARE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <js.h>
#include <stddef.h>
#include <uv.h>

#include "bare/module.h"
#include "bare/target.h"
#include "bare/version.h"

typedef struct bare_s bare_t;
typedef struct bare_options_s bare_options_t;

typedef void (*bare_before_exit_cb)(bare_t *, void *data);
typedef void (*bare_exit_cb)(bare_t *, void *data);
typedef void (*bare_suspend_cb)(bare_t *, int linger, void *data);
typedef void (*bare_wakeup_cb)(bare_t *, int deadline, void *data);
typedef void (*bare_idle_cb)(bare_t *, void *data);
typedef void (*bare_resume_cb)(bare_t *, void *data);
typedef void (*bare_thread_cb)(bare_t *, js_env_t *, void *data);
typedef void (*bare_context_destroy_cb)(const char *key, void *value);

/** @version 0 */
struct bare_options_s {
  int version;

  /**
   * The memory limit of each JavaScript heap. By default, the limit will be
   * inferred based on the amount of physical memory of the device.
   *
   * Note that the limit applies individually to each thread, including the
   * main thread.
   *
   * @since 0
   */
  size_t memory_limit;
};

/**
 * Get the current Bare version. Useful for cases where embedders are
 * dynamically linking Bare.
 */
int
bare_version(int *major, int *minor, int *patch);

/**
 * Set up the Bare process. To get a reference to the JavaScript environment of
 * the process, pass the `env` pointer.
 */
int
bare_setup(uv_loop_t *loop, js_platform_t *platform, js_env_t **env, int argc, const char *argv[], const bare_options_t *options, bare_t **result);

/**
 * Tear down the Bare process. The exit code will be stored in `exit_code` if
 * provided. The JavaScript environment of the process must not be used after
 * this function returns.
 */
int
bare_teardown(bare_t *bare, uv_run_mode mode, int *exit_code);

/**
 * Immediately terminate the process with an exit status of `exit_code`.
 */
int
bare_exit(bare_t *bare, int exit_code);

/**
 * Seal addon loading for the process. Once sealed, the process may not load
 * any further dynamic addons on any of its threads, now or in the future;
 * attempting to do so throws. Statically linked addons are compiled in and
 * remain available.
 *
 * The seal applies to the process alone and is lifted when it is torn down.
 * Other processes running in the same operating system process are unaffected
 * and may continue to load addons of their own.
 *
 * Sealing also freezes the context registry of the process, which may not gain
 * an entry once sealed. Publishing context is adjacent to loading native code,
 * as an addon that could publish a handle could feed it to another addon, so
 * one call establishes the whole boundary.
 *
 * Equivalent to `Bare.Addon.seal()`.
 */
int
bare_seal(bare_t *bare);

/**
 * Publish `value` under `key` for the addons of the process to retrieve with
 * `bare_context_get()`. Use this to hand an addon a handle that only the
 * embedder can produce, such as a `JavaVM *` on Android.
 *
 * The registry stores the pointer and never interprets it. An entry lasts for
 * as long as the process, which is what makes it safe to hand the pointer to an
 * addon: nothing can withdraw it while the addon is using it.
 *
 * If `destroy` is given it's called with the key and the value when the process
 * is torn down. It runs on the main thread after every thread has been joined
 * and after the JavaScript environment has been destroyed, so it must not call
 * into the environment or back into the registry, and the key it's handed only
 * stays alive for the duration of the call.
 *
 * Keys are compared by their contents and are namespaced by whoever owns the
 * handle, such as `bare.android.jvm.v1`. The `bare.` namespace is Bare's own
 * and is where the handles that Bare and its addons agree on live; anyone else
 * should pick a namespace of their own. Version the key rather than the value,
 * as an addon that needs a different contract can then ask for a different key.
 * The key is copied and need not outlive the call. See `docs/context-keys.md`
 * for the keys in use and what each of them points at.
 *
 * Entries are immutable: setting a key that's already published fails rather
 * than replacing it. There is no way to withdraw or replace one, as an addon
 * that has been handed a pointer has no way of hearing that it went away. An
 * embedder that needs a handle to change publishes the change under a key of
 * its own and lets the addon go looking.
 *
 * Publish everything the addons of the process need before the first
 * `bare_load()`. An addon only ever sees what was published before it was
 * loaded, and addons are loaded when the module graph first reaches them rather
 * than at a point the embedder can predict, so publishing any later doesn't
 * merely risk being late: it's seen by some addons and not others, and by some
 * threads and not others, depending on the shape of the graph. Nothing reports
 * this, which is why the rule is to publish up front.
 *
 * This half of the registry is for the embedder alone; addons hold no `bare_t *`
 * and are expected to use `bare_context_get()` only. The split is a convention
 * rather than a boundary, and sealing the process is what closes the door.
 *
 * Returns `-1` if the key is already published, if the process has been sealed,
 * or if the entry couldn't be allocated.
 */
int
bare_context_set(bare_t *bare, const char *key, void *value, bare_context_destroy_cb destroy);

/**
 * Retrieve the value published under `key` by the embedder of the process
 * running on the calling thread. This is the addon facing half of the registry
 * and takes no handle, as an addon has none; the process is instead the one
 * whose runtime the thread has entered, which is well defined for as long as
 * an addon can be called into, including while it initialises.
 *
 * A missing key is an ordinary outcome rather than a fatal one, and addons are
 * expected to degrade rather than fail. Being asked from the wrong thread is
 * not, so the two are reported apart: an addon that retrieves the handle from a
 * thread of its own gets `-2` rather than a missing key it would otherwise
 * degrade over silently and for good. Retrieve and stash the handle while the
 * addon initialises if a thread of its own is going to need it.
 *
 * `result` may be `NULL` to test for a key without retrieving it, and is left
 * untouched unless `0` is returned.
 *
 * Returns `-1` if no entry is published under the key, and `-2` if no process
 * is running on the calling thread.
 */
int
bare_context_get(const char *key, void **result);

/**
 * Load the module identified by `filename`, which may be any of the formats
 * supported by the module system. Unless `source` is provided, the contents
 * of `filename` will be read from disk. If `source` is provided, its contents
 * must remain alive until after `bare_teardown()` has returned.
 *
 * See https://github.com/holepunchto/bare-module for more information on the
 * supported module formats.
 */
int
bare_load(bare_t *bare, const char *filename, const uv_buf_t *source, js_value_t **result);

/**
 * Run the I/O event loop.
 */
int
bare_run(bare_t *bare, uv_run_mode mode);

/**
 * Suspend the process as soon as possible. Once the process has suspended
 * successfully, `bare_run()` will not return until another thread resumes the
 * process. It's safe to call this function from any thread.
 */
int
bare_suspend(bare_t *bare, int linger);

/**
 * Wake up the process if suspended and give it time to perform background work.
 * It's safe to call this function from any thread.
 */
int
bare_wakeup(bare_t *bare, int deadline);

/**
 * Resume the process as soon as possible. If the process is not yet idle after
 * being suspended the suspension will be cancelled. It's safe to call this
 * function from any thread.
 */
int
bare_resume(bare_t *bare);

/**
 * Terminate the process as soon as possible. It's safe to call this function
 * from any thread.
 */
int
bare_terminate(bare_t *bare);

/**
 * Equivalent to `Bare.on('beforeExit', cb)`.
 */
int
bare_on_before_exit(bare_t *bare, bare_before_exit_cb cb, void *data);

/**
 * Equivalent to `Bare.on('exit', cb)`.
 */
int
bare_on_exit(bare_t *bare, bare_exit_cb cb, void *data);

/**
 * Equivalent to `Bare.on('suspend', cb)`.
 */
int
bare_on_suspend(bare_t *bare, bare_suspend_cb cb, void *data);

/**
 * Equivalent to `Bare.on('wakeup', cb)`.
 */
int
bare_on_wakeup(bare_t *bare, bare_wakeup_cb cb, void *data);

/**
 * Equivalent to `Bare.on('idle', cb)`.
 */
int
bare_on_idle(bare_t *bare, bare_idle_cb cb, void *data);

/**
 * Equivalent to `Bare.on('resume', cb)`.
 */
int
bare_on_resume(bare_t *bare, bare_resume_cb cb, void *data);

/**
 * Attach a thread listener which will invoked with the JavaScript environment
 * of each thread created with the `Thread` constructor. Use this to modify the
 * environment of the thread before it runs any JavaScript.
 */
int
bare_on_thread(bare_t *bare, bare_thread_cb cb, void *data);

#ifdef __cplusplus
}
#endif

#endif // BARE_H
