# Bare

Small and modular JavaScript runtime for desktop and mobile. Like Node.js, it provides an asynchronous, event-driven architecture for writing applications in the lingua franca of modern software. Unlike Node.js, it makes embedding and cross-device support core use cases, aiming to run just as well on your phone as on your laptop. The result is a runtime ideal for networked, peer-to-peer applications that can run on a wide selection of hardware.

## Usage

```sh
$ bare [-m, --import-map <path>] [<filename>]
```

## API

### `process`

The core JavaScript API of Bare is available through the global `process` object, which is also available by importing the builtin `process` module. The `process` object provides information about, and control over, the Bare process and also provides access to core functionality like loading native addons.

#### `process.argv`

The command line arguments passed to the process when launched.

#### `process.versions`

An object containing the version strings of Bare and its dependencies.

#### `process.version`

The Bare version string.

#### `process.exitCode`

The code that will be returned once the process exits. If the process is exited using `process.exit()` without specifying a code, `process.exitCode` is used.

#### `process.suspended`

Whether or not the process is currently suspended.

#### `process.exiting`

Whether or not the process is currently exiting.

#### `process.exit([code])`

Forcefully terminate the process or current thread with an exit status of `code` which defaults to `process.exitCode`.

#### `process.suspend()`

Suspend the process and all threads. This will emit a `suspend` event signalling that all work should stop immediately. When all work has stopped and the process would otherwise exit, an `idle` event will be emitted. If the process is not resumed from an `idle` event listener and no additional work is scheduled, the loop will block until the process is resumed. If additional work is scheduled from an `idle` event, the `idle` event will be emitted again once all work has stopped unless the process was resumed.

#### `process.resume()`

Resume the process and all threads after suspension. This can be used to cancel suspension after the `suspend` event has been emitted and up until all `idle` event listeners have run.

#### `process.addon(specifier)`

Load a static or dynamic native addon identified by `specifier` using `Addon.load()`. See [Addons](#addons) for more information.

#### `process.on('uncaughtException', err)`

Emitted when a JavaScript exception is thrown within an exectuion context without being caught by any exception handlers within that execution context. By default, uncaught exceptions are printed to `stderr` and the processes exited with an exit status of 1. Adding an event listener for the `uncaughtException` event overrides the default behavior.

#### `process.on('unhandledRejection', reason, promise)`

Emitted when a JavaScript promise is rejected within an execution context without that rejection being handled within that execution context. By default, unhandled rejections are printed to `stderr` and the process exited with an exit status of 1. Adding an event listener for the `unhandledRejection` event overrides the default behavior.

#### `process.on('beforeExit', code)`

Emitted when the loop runs out of work and before the process or current thread exits. This provides a chance to schedule additional work and keep the process from exiting. If additional work is scheduled, `beforeExit` will be emitted again once the loop runs out of work.

If the process is exited explicitly, such as by calling `process.exit()` or as the result of an uncaught exception, the `beforeExit` event will not be emitted.

#### `process.on('exit', code)`

Emitted just before the process or current thread terminates. Additional work scheduled from an `exit` event listener will be given a chance to run after which the process will terminate. If the process is forcefully terminated from an `exit` event listener, the remaining listeners will not run.

#### `process.on('teardown')`

Emitted after the process or current thread has terminated and just before the JavaScript environment is torn down. Additional work must not be scheduled from a `teardown` event listener. Bare itself will register `teardown` event listeners to join dangling threads and unload native addons.

#### `process.on('suspend')`

Emitted when the process or current thread is suspended. Any in-progress or outstanding work, such as network activity or file system access, should be deferred, cancelled, or paused when the `suspend` event is emitted and no additional work may be scheduled.

#### `process.on('idle')`

Emitted when the process or current thread becomes idle after suspension. If no additional work is scheduled from this event, the loop will block and no additional work be performed until the process is resumed. An `idle` event listener may call `process.resume()` to cancel the suspension.

#### `process.on('resume')`

Emitted when the process or current thread resumes after suspension. Deferred and paused work should be continued when the `resume` event is emitted and new work may again be scheduled.

### Addons

The builtin `addon` module provides support for loading native addons, which are typically written in C/C++ and distributed as shared libraries.

#### `const exports = Addon.load(specifier)`

Load a static or dynamic native addon identified by `specifier`. If `specifier` is not a static native addon, Bare will instead look for a matching dynamic object library using `Addon.resolve()`. Modules with native addons can use this mechanism to export their bindings, such as by doing `module.exports = Addon.load(__dirname)` from the root of the module. This will allow them to be used in both static and dynamic contexts.

#### `specifier = Addon.resolve(specifier)`

Resolve a dynamic native addon specifier by searching for a dynamic object library matching `specifier`.

#### `const unloaded = Addon.unload(specifier)`

Unload a dynamic native addon identified by `specifier`, which must be fully resolved. If the function returns `true`, the addon was unloaded from memory. If it instead returns `false`, the addon is still in use by one or more threads and will only be unloaded from memory when those threads either exit or explicitly unload the addon.

### Threads

The builtin `thread` module provides support for lightweight threads. Threads are similar to workers in Node.js, but provide only minimal API surface for creating and joining threads.

#### `Thread.isMainThread`

`true` if the current thread is the main thread.

#### `Thread.self`

A reference to the current thread as a `ThreadProxy` object. Will be `null` on the main thread.

#### `Thread.self.data`

A copy of or, if shared, reference to the `data` buffer that was passed to the current thread on creation. Will be `null` if no buffer was passed.

#### `const thread = new Thread([filename][, options][, callback])`

Start a new thread that will run the contents of `filename`. If `callback` is provided, its function body will be treated as the contents of `filename` and invoked on the new thread with `Thread.self.data` passed as an argument.

Options include:

```js
{
  // Optional data to pass to the thread
  data: Buffer | ArrayBuffer | SharedArrayBuffer | External,
  // Optional file source, will be read from `filename` if neither `source` nor `callback` are provided
  source: string | Buffer,
  // Optional source encoding if `source` is a string
  encoding: 'utf8',
  // Optional stack size in bytes, pass 0 for default
  stackSize: 0
}
```

#### `const thread = Thread.create([filename][, options][, callback])`

Convenience method for the `new Thread()` constructor

#### `thread.joined`

Whether or not the thread has been joined with the current thread.

#### `thread.join()`

Block and wait for the thread to exit.

### Modules

In addition to the core `process`, `addon`, and `thread` modules, Bare provides a small selection of builtin modules to cover the most basic use cases, primarily those of the runtime itself:

- `assert` (<https://github.com/holepunchto/bare-assert>)
- `buffer` (<https://github.com/holepunchto/bare-buffer>)
- `console` (<https://github.com/holepunchto/bare-console>)
- `events` (<https://github.com/holepunchto/bare-events>)
- `module` (<https://github.com/holepunchto/bare-module>)
- `os` (<https://github.com/holepunchto/bare-os>)
- `path` (<https://github.com/holepunchto/bare-path>)
- `timers` (<https://github.com/holepunchto/bare-timers>)
- `url` (<https://github.com/holepunchto/bare-url>)

### Embedding

Bare can easily be embedded using the C API defined in [`include/bare.h`](include/bare.h):

```c
#include <bare.h>
#include <uv.h>

bare_t *bare;
bare_setup(uv_default_loop(), platform, &env, argc, argv, &bare);

bare_run(bare, filename, source);

int exit_code;
bare_teardown(bare, &exit_code);
```

If `source` is `NULL`, the contents of `filename` will instead be read at runtime.

### Suspension

Bare provides a mechanism for implementing process suspension, which is needed for platforms with strict application lifecycle constraints, such as mobile platforms. When suspended, a `suspend` event will be emitted on the `process` object. Then, when the loop has no work left and would otherwise exit, an `idle` event will be emitted and the loop blocked, keeping it from exiting. When the process is later resumed, a `resume` event will be emitted and the loop unblocked, allowing it to exit when no work is left.

The suspension API is available through `bare_suspend()` and `bare_resume()` from C and `process.suspend()` and `process.resume()` from JavaScript. See [`example/suspend.js`](example/suspend.js) for an example of using the suspension API from JavaScript.

## Building

To build Bare, start by installing the initial npm dependencies:

```sh
$ npm install
```

One of these dependencies is the `bare-dev` toolkit which we'll be invoking with `npx`. Next, synchronise the vendored dependencies, such as git submodules:

```sh
$ npx bare-dev vendor sync
```

You should repeat this whenever the vendored dependencies are updated. The vendored dependencies also include npm dependencies of their own, so make sure to `npm install` again as well. Then, configure the build tree before performing the first build:

```sh
$ npx bare-dev configure [--debug]
```

Finally, perform the build:

```sh
$ npx bare-dev build
```

When completed, the `bare` binary will be available in the `build/bin` directory and the `libbare.(a|lib)` and `(lib)bare.(dylib|dll)` libraries will be available in the root of the `build` directory.

### Linking

When linking against the static `libbare.(a|lib)` library, make sure to use whole archive linking as Bare relies on constructor functions for registering native addons. Without whole archive linking, the linker will remove the constructor functions as they aren't referenced by anything.

## License

Apache-2.0
