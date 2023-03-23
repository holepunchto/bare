# :pear:.js

Small and modular JavaScript runtime for desktop and mobile. Like Node.js, it provides an asynchronous, event-driven architecture for writing applications in the lingua franca of modern software. Unlike Node.js, it makes embedding and cross-device support core use cases, aiming to run just as well on your phone as on your laptop. The result is a runtime ideal for networked, peer-to-peer applications that can run on a wide selection of hardware.

## Usage

```sh
$ pear [-m, --import-map <path>] <filename>
```

## API

### `process`

The core JavaScript API of :pear:.js is available through the global `process` object, which is also available by importing the builtin `process` module. The `process` object provides information about, and control over, the :pear:.js process and also provides access to core functionality like loading native addons.

#### `process.platform`

The identifier of the operating system for which :pear:.js was compiled. The possible values are `android`, `darwin`, `ios`, `linux`, and `win32`.

#### `process.arch`

The identifier of the processor architecture for which :pear:.js was compiled. The possible values are `arm`, `arm64`, `ia32`, and `x64`.

#### `process.execPath`

The absolute path, with any symbolic links resolved, to the binary that started the current process.

#### `process.argv`

The command line arguments passed to the process when launched.

#### `process.pid`

The ID of the current process.

#### `process.title`

The title of the current process. Assigning a new value will change the title of the process. On platforms where the process title is backed by a buffer with a fixed size, the new title will be truncated if larger than the previous title. See <http://docs.libuv.org/en/v1.x/misc.html#c.uv_set_process_title> for more information.

#### `process.exitCode`

The code that will be returned once the process exits. If the process is exited using `process.exit()` without specifying a code, `process.exitCode` is used.

#### `process.env`

The current user environment. If modified, the changes will be visible to JavaScript and native code running in the current process, but will not be reflected outside of it.

#### `process.data`

The current user data. If modified, the changes will be visible to JavaScript and native code running in the current process. See [User data](##user-data) for more information.

#### `process.versions`

An object containing the version strings of :pear:.js and its dependencies.

#### `process.cwd()`

Get the current working directory of the process.

#### `process.chdir(directory)`

Change the current working directory of the process.

#### `process.exit([code])`

Synchronously exit the current process with an exit status of `code` which defaults to `process.exitCode`. The process will not terminate until all `exit` event listeners have been called.

#### `process.suspend()`

Suspend the current process. This will emit a `suspend` event signalling that all work should stop immediately. When all work has stopped and the process would otherwise exit, an `idle` event will be emitted. If the process is not resumed from an `idle` event listener, the loop will block and allow no further work until the process is resumed.

#### `process.resume()`

Resume the process after suspension. This can be used to cancel process suspension after the `suspend` event has been emitted and up until all `idle` event listeners have run.

#### `process.addon(specifier)`

Load a static or dynamic native addon identified by `specifier`. If `specifier` is not a static native addon, :pear:.js will instead look for a matching dynamic object library using `process.addon.resolve()`. Modules with native addons can use this mechanism to export their bindings, such as by doing `module.exports = process.addon(__dirname)` from the root of the module. This will allow them to be used in both static and dynamic contexts.

#### `process.addon.resolve(specifier)`

Resolve a dynamic native addon specifier by searching for a dynamic object library matching `specifier`.

#### `process.hrtime([previous])`

Get the current high-resolution real time as a `[seconds, nanoseconds]` 32-bit unsigned integer tuple. If `previous` is specified, the returned tuple will contain the delta from the `previous` time.

#### `process.hrtime.bigint()`

Get the current high-resolution real time in nanoseconds as a `bigint`.

#### `process.nextTick(callback[, ...args])`

A wrapper around `queueMicrotask()` for compatibility purposes.

#### `process.on('uncaughtException', err)`

Emitted when a JavaScript exception is thrown within an exectuion context without being caught by any exception handlers within that execution context. By default, uncaught exceptions are printed to `stderr` and the processes exited with an exit status of 1. Adding an event listener for the `uncaughtException` event overrides the default behavior.

#### `process.on('unhandledRejection', reason, promise)`

Emitted when a JavaScript promise is rejected within an execution context without that rejection being handled within that execution context. By default, unhandled rejections are printed to `stderr` and the process exited with an exit status of 1. Adding an event listener for the `unhandledRejection` event overrides the default behavior.

#### `process.on('beforeExit', code)`

Emitted when the loop runs out of work and before the process exits. This provides a chance to schedule additional work and keep the process from exiting. If additional work is scheduled, `beforeExit` will be emitted again once the loop runs out of work.

If the process is exited explicitly, such as by calling `process.exit()` or as the result of an uncaught exception, the `beforeExit` event will not be emitted.

#### `process.on('exit', code)`

Emitted just before the process terminates. Any additional work scheduled after `exit` has been emitted will not be performed and so event listeners can only perform synchronous operations.

All registered `exit` event listeners will be called before the process terminates. An event listener may override the final exit code by setting `process.exitCode` or by calling `process.exit(code)`. Calling `process.exit()` from an `exit` event listener will not prevent the remaining event listeners from running.

#### `process.on('suspend')`

Emitted when the process is suspended. Any in-progress or outstanding work, such as network activity or file system access, should be deferred, cancelled, or paused when the `suspend` event is emitted and no additional work may be scheduled.

#### `process.on('idle')`

Emitted when the process becomes idle after suspension. After this, the loop will block and no additional work be performed until the process is resumed. An `idle` event listener may call `process.resume()` to cancel the suspension.

#### `process.on('resume')`

Emitted when the process resumes after suspension. Deferred and paused work should be continued when the `resume` event is emitted and new work may again be scheduled.

### Modules

In addition to the core `process` module, :pear:.js provides a small selection of builtin modules to cover the most basic use cases, primarily those of the runtime itself:

- `assert` (<https://github.com/holepunchto/pearjs-assert>)
- `buffer` (<https://github.com/holepunchto/pearjs-buffer>)
- `console` (<https://github.com/holepunchto/pearjs-console>)
- `events` (<https://github.com/holepunchto/pearjs-events>)
- `module` (<https://github.com/holepunchto/pearjs-module>)
- `path` (<https://github.com/holepunchto/pearjs-path>)
- `timers` (<https://github.com/holepunchto/pearjs-timers>)

### Embedding

:pear:.js can easily be embedded using the C API defined in [`include/pear.h`](include/pear.h):

```c
#include <pear.h>
#include <uv.h>

pear_t pear;
pear_setup(uv_default_loop(), &pear, argc, argv);

pear_run(&pear, filename, source);

int exit_code;
pear_teardown(&pear, &exit_code);
```

If `source` is `NULL`, the contents of `filename` will instead be read at runtime.

### Suspension

:pear:.js provides a mechanism for implementing process suspension, which is needed for platforms with strict application lifecycle constraints, such as mobile platforms. When suspended, a `suspend` event will be emitted on the `process` object. Then, when the loop has no work left and would otherwise exit, an `idle` event will be emitted and the loop blocked, keeping it from exiting. When the process is later resumed, a `resume` event will be emitted and the loop unblocked, allowing it to exit when no work is left.

The suspension API is available through `pear_suspend()` and `pear_resume()` from C and `process.suspend()` and `process.resume()` from JavaScript. See [`example/suspend.js`](example/suspend.js) for an example of using the suspension API from JavaScript.

### User data

:pear:.js provides a common API for simple user data exchange between the embedder, the JavaScript layer, and the native addon layer. The API provides a key/value store that associates string keys with JavaScript values, leaving it up to the embedder to decide on the meaning of the keys. Keys can be added and retrieved through `pear_set_data()` and `pear_get_data()` from C, and also accessed through `process.data` from JavaScript. 

## Building

To build :pear:.js, start by installing the dependencies:

```sh
$ npm install
```

Next, configure the build tree before performing the first build:

```sh
$ npm run configure -- [--debug]
```

Finally, perform the build:

```sh
$ npm run build
```

When completed, the `pear` binary will be available in the `build/bin` directory and the `libpear.(a|lib)` and `(lib)pear.(dylib|dll)` libraries will be available in the root of the `build` directory.

### Linking

When linking against the static `libpear.(a|lib)` library, make sure to use whole archive linking as :pear:.js relies on constructor functions for registering native addons. Without whole archive linking, the linker will remove the constructor functions as they aren't referenced by anything.

## License

Apache-2.0
