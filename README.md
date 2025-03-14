# Bare

Small and modular JavaScript runtime for desktop and mobile. Like Node.js, it provides an asynchronous, event-driven architecture for writing applications in the lingua franca of modern software. Unlike Node.js, it makes embedding and cross-device support core use cases, aiming to run just as well on your phone as on your laptop. The result is a runtime ideal for networked, peer-to-peer applications that can run on a wide selection of hardware.

```sh
npm i -g bare
```

## Usage

```console
bare [flags] <filename> [...args]

Evaluate a script or start a REPL session if no script is provided.

Arguments:
  <filename>            The name of a script to evaluate
  [...args]             Additional arguments made available to the script

Flags:
  --version|-v          Print the Bare version
  --eval|-e <script>    Evaluate an inline script
  --print|-p <script>   Evaluate an inline script and print the result
  --inspect             Activate the inspector
  --help|-h             Show help
```

The specified `<script>` or `<filename>` is run using `Module.load()`. For more information on the module system and the supported formats, see <https://github.com/holepunchto/bare-module>.

## Architecture

Bare is built on top of <https://github.com/holepunchto/libjs>, which provides low-level bindings to V8 in an engine independent manner, and <https://github.com/libuv/libuv>, which provides an asynchronous I/O event loop. Bare itself only adds a few missing pieces on top to support a wider ecosystem of modules:

1. A module system supporting both CJS and ESM with bidirectional interoperability between the two.
2. A native addon system supporting both statically and dynamically linked addons.
3. Light-weight thread support with synchronous joins and shared array buffer support.

Everything else if left to userland modules to implement using these primitives, keeping the runtime itself succinct and _bare_. By abstracting over both the underlying JavaScript engine using `libjs` and platform I/O operations using `libuv`, Bare allows module authors to implement native addons that can run on any JavaScript engine that implements the `libjs` ABI and any system that `libuv` supports.

## API

### `Bare`

The core JavaScript API of Bare is available through the global `Bare` namespace.

#### `Bare.platform`

The identifier of the operating system for which Bare was compiled. The possible values are `android`, `darwin`, `ios`, `linux`, and `win32`.

#### `Bare.arch`

The identifier of the processor architecture for which Bare was compiled. The possible values are `arm`, `arm64`, `ia32`, `mips`, `mipsel`, and `x64`.

#### `Bare.simulator`

Whether or not Bare was compiled for a simulator.

#### `Bare.argv`

The command line arguments passed to the process when launched.

#### `Bare.pid`

The ID of the current process.

#### `Bare.exitCode`

The code that will be returned once the process exits. If the process is exited using `Bare.exit()` without specifying a code, `Bare.exitCode` is used.

#### `Bare.suspended`

Whether or not the process is currently suspended.

#### `Bare.exiting`

Whether or not the process is currently exiting.

#### `Bare.version`

The Bare version string.

#### `Bare.versions`

An object containing the version strings of Bare and its dependencies.

#### `Bare.exit([code])`

Immediately terminate the process or current thread with an exit status of `code` which defaults to `Bare.exitCode`.

#### `Bare.suspend([linger])`

Suspend the process and all threads. This will emit a `suspend` event signalling that all work should stop immediately. When all work has stopped and the process would otherwise exit, an `idle` event will be emitted. If the process is not resumed from an `idle` event listener and no additional work is scheduled, the loop will block until the process is resumed. If additional work is scheduled from an `idle` event, the `idle` event will be emitted again once all work has stopped unless the process was resumed.

#### `Bare.resume()`

Resume the process and all threads after suspension. This can be used to cancel suspension after the `suspend` event has been emitted and up until all `idle` event listeners have run.

#### `Bare.on('uncaughtException', err)`

Emitted when a JavaScript exception is thrown within an execution context without being caught by any exception handlers within that execution context. By default, uncaught exceptions are printed to `stderr` and the processes aborted. Adding an event listener for the `uncaughtException` event overrides the default behavior.

#### `Bare.on('unhandledRejection', reason, promise)`

Emitted when a JavaScript promise is rejected within an execution context without that rejection being handled within that execution context. By default, unhandled rejections are printed to `stderr` and the process aborted. Adding an event listener for the `unhandledRejection` event overrides the default behavior.

#### `Bare.on('beforeExit', code)`

Emitted when the loop runs out of work and before the process or current thread exits. This provides a chance to schedule additional work and keep the process from exiting. If additional work is scheduled, `beforeExit` will be emitted again once the loop runs out of work.

If the process is exited explicitly, such as by calling `Bare.exit()` or as the result of an uncaught exception, the `beforeExit` event will not be emitted.

#### `Bare.on('exit', code)`

Emitted before the process or current thread terminates. Additional work must not be scheduled from an `exit` event listener. If the process is forcefully terminated from an `exit` event listener, the remaining listeners will not run.

#### `Bare.on('teardown')`

Emitted after the process or current thread has terminated and before the JavaScript environment is torn down. Additional work must not be scheduled from a `teardown` event listener. Bare itself will register `teardown` event listeners to join dangling threads and unload native addons.

> [!IMPORTANT]
>
> ##### Teardown ordering
>
> `teardown` listeners **SHOULD** be prepended to have the listeners run in last in, first out order:
>
> ```js
> Bare.prependListener('teardown', () => { ... })
> ```

#### `Bare.on('suspend', linger)`

Emitted when the process or current thread is suspended. Any in-progress or outstanding work, such as network activity or file system access, should be deferred, cancelled, or paused when the `suspend` event is emitted and no additional work may be scheduled.

#### `Bare.on('idle')`

Emitted when the process or current thread becomes idle after suspension. If no additional work is scheduled from this event, the loop will block and no additional work be performed until the process is resumed. An `idle` event listener may call `Bare.resume()` to cancel the suspension.

#### `Bare.on('resume')`

Emitted when the process or current thread resumes after suspension. Deferred and paused work should be continued when the `resume` event is emitted and new work may again be scheduled.

### `Bare.Addon`

The `Bare.Addon` namespace provides support for loading native addons, which are typically written in C/C++ and distributed as shared libraries.

#### `const addon = Addon.load(url[, options])`

Load a static or dynamic native addon identified by `url`. If `url` is not a static native addon, Bare will instead look for a matching dynamic object library.

Options are reserved.

#### `const unloaded = Addon.unload(url[, options])`

Unload a dynamic native addon identified by `url`. If the function returns `true`, the addon was unloaded from memory. If it instead returns `false`, the addon is still in use by one or more threads and will only be unloaded from memory when those threads either exit or explicitly unload the addon.

Options are reserved.

#### `const url = Addon.resolve(specifier, parentURL[, options])`

Resolve a native addon specifier by searching for a static native addon or dynamic object library matching `specifier` imported from `parentURL`.

Options are reserved.

### `Bare.Thread`

The `Bare.Thread` provides support for lightweight threads. Threads are similar to workers in Node.js, but provide only minimal API surface for creating and joining threads.

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
  data: null,
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

### Embedding

Bare can easily be embedded using the C API defined in [`include/bare.h`](include/bare.h):

```c
#include <bare.h>
#include <uv.h>

bare_t *bare;
bare_setup(uv_default_loop(), platform, &env /* Optional */, argc, argv, options, &bare);

bare_load(bare, filename, source, &module /* Optional */);

bare_run(bare);

int exit_code;
bare_teardown(bare, &exit_code);
```

If `source` is `NULL`, the contents of `filename` will instead be read at runtime. For examples of how to embed Bare on mobile platforms, see <https://github.com/holepunchto/bare-android> and <https://github.com/holepunchto/bare-ios>.

### Suspension

Bare provides a mechanism for implementing process suspension, which is needed for platforms with strict application lifecycle constraints, such as mobile platforms. When suspended, a `suspend` event will be emitted on the `Bare` namespace. Then, when the loop has no work left and would otherwise exit, an `idle` event will be emitted and the loop blocked, keeping it from exiting. When the process is later resumed, a `resume` event will be emitted and the loop unblocked, allowing it to exit when no work is left.

The suspension API is available through `bare_suspend()` and `bare_resume()` from C and `Bare.suspend()` and `Bare.resume()` from JavaScript.

## Building

<https://github.com/holepunchto/bare-make> is used for compiling Bare. Start by installing the tool globally:

```console
npm i -g bare-make
```

Next, install the required build and runtime dependencies:

```console
npm i
```

Then, generate the build system:

```console
bare-make generate
```

This only has to be run once per repository checkout. When updating `bare-make` or your compiler toolchain it might also be necessary to regenerate the build system. To do so, run the command again with the `--no-cache` flag set to disregard the existing build system cache:

```console
bare-make generate --no-cache
```

With a build system generated, Bare can be compiled:

```console
bare-make build
```

When completed, the `bare(.exe)` binary will be available in the `build/bin` directory and the `libbare.(a|lib)` and `(lib)bare.(dylib|dll|lib)` libraries will be available in the root of the `build` directory.

### Linking

When linking against the static `libbare.(a|lib)` library, make sure to use whole archive linking as Bare relies on constructor functions for registering native addons. Without whole archive linking, the linker will remove the constructor functions as they aren't referenced by anything.

### Options

Bare provides a few compile options that can be configured to customize various aspects of the runtime. Compile options may be set by passing the `--define option=value` flag to the `bare-make generate` command when generating the build system.

> [!WARNING]  
> The compile options are not covered by semantic versioning and are subject to change without warning.

| Option           | Default                    | Description                                             |
| :--------------- | :------------------------- | :------------------------------------------------------ |
| `BARE_ENGINE`    | `github:holepunchto/libjs` | The JavaScript engine to use                            |
| `BARE_PREBUILDS` | `ON`                       | Enable prebuilds for supported third-party dependencies |

## Platform support

Bare uses a tiered support system to manage expectations for the platforms that it targets. Targets may move between tiers between minor releases and as such a change in tier will not be considered a breaking change.

**Tier 1:** Platform targets for which prebuilds are provided as defined by the [`.github/workflows/prebuild.yml`](.github/workflows/prebuild.yml) workflow. Compilation and test failures for these targets will cause workflow runs to go red.

**Tier 2:** Platform targets for which Bare is known to work, but without automated compilation and testing. Regressions may occur between releases and will be considered bugs.

> [!NOTE]  
> Development happens primarily on Apple hardware with Linux and Windows systems running as virtual machines.

| Platform  | Architecture | Version                              | Tier | Notes                       |
| :-------- | :----------- | :----------------------------------- | :--- | :-------------------------- |
| GNU/Linux | `arm64`      | >= Linux 5.15, >= GNU C Library 2.35 | 1    | Ubuntu 22.04, OpenWrt 23.05 |
| GNU/Linux | `x64`        | >= Linux 5.15, >= GNU C Library 2.35 | 1    | Ubuntu 22.04, OpenWrt 23.05 |
| GNU/Linux | `arm64`      | >= Linux 5.10, >= musl 1.2           | 2    | Alpine 3.13, OpenWrt 22.03  |
| GNU/Linux | `x64`        | >= Linux 5.10, >= musl 1.2           | 2    | Alpine 3.13, OpenWrt 22.03  |
| GNU/Linux | `mips`       | >= Linux 5.10, >= musl 1.2           | 2    | OpenWrt 22.03               |
| GNU/Linux | `mipsel`     | >= Linux 5.10, >= musl 1.2           | 2    | OpenWrt 22.03               |
| Android   | `arm`        | >= 9                                 | 1    |
| Android   | `arm64`      | >= 9                                 | 1    |
| Android   | `ia32`       | >= 9                                 | 1    |
| Android   | `x64`        | >= 9                                 | 1    |
| macOS     | `arm64`      | >= 11.0                              | 1    |
| macOS     | `x64`        | >= 11.0                              | 1    |
| iOS       | `arm64`      | >= 14.0                              | 1    |
| iOS       | `x64`        | >= 14.0                              | 1    | Simulator only              |
| Windows   | `arm64`      | >= Windows 11                        | 1    |
| Windows   | `x64`        | >= Windows 10                        | 1    |

## Modules

Bare provides no standard library beyond the core JavaScript API available through the `Bare` namespace. Instead, we maintain a comprehensive collection of external modules built specifically for Bare:

| Module                                                 |
| :----------------------------------------------------- |
| <https://github.com/holepunchto/bare-abort>            |
| <https://github.com/holepunchto/bare-assert>           |
| <https://github.com/holepunchto/bare-atomics>          |
| <https://github.com/holepunchto/bare-buffer>           |
| <https://github.com/holepunchto/bare-bundle>           |
| <https://github.com/holepunchto/bare-channel>          |
| <https://github.com/holepunchto/bare-console>          |
| <https://github.com/holepunchto/bare-crypto>           |
| <https://github.com/holepunchto/bare-dgram>            |
| <https://github.com/holepunchto/bare-dns>              |
| <https://github.com/holepunchto/bare-env>              |
| <https://github.com/holepunchto/bare-events>           |
| <https://github.com/holepunchto/bare-fetch>            |
| <https://github.com/holepunchto/bare-format>           |
| <https://github.com/holepunchto/bare-fs>               |
| <https://github.com/holepunchto/bare-hrtime>           |
| <https://github.com/holepunchto/bare-http1>            |
| <https://github.com/holepunchto/bare-https>            |
| <https://github.com/holepunchto/bare-inspect>          |
| <https://github.com/holepunchto/bare-inspector>        |
| <https://github.com/holepunchto/bare-ipc>              |
| <https://github.com/holepunchto/bare-module>           |
| <https://github.com/holepunchto/bare-net>              |
| <https://github.com/holepunchto/bare-os>               |
| <https://github.com/holepunchto/bare-path>             |
| <https://github.com/holepunchto/bare-pipe>             |
| <https://github.com/holepunchto/bare-process>          |
| <https://github.com/holepunchto/bare-querystring>      |
| <https://github.com/holepunchto/bare-readline>         |
| <https://github.com/holepunchto/bare-realm>            |
| <https://github.com/holepunchto/bare-repl>             |
| <https://github.com/holepunchto/bare-rpc>              |
| <https://github.com/holepunchto/bare-semver>           |
| <https://github.com/holepunchto/bare-signals>          |
| <https://github.com/holepunchto/bare-stream>           |
| <https://github.com/holepunchto/bare-structured-clone> |
| <https://github.com/holepunchto/bare-subprocess>       |
| <https://github.com/holepunchto/bare-tcp>              |
| <https://github.com/holepunchto/bare-timers>           |
| <https://github.com/holepunchto/bare-tls>              |
| <https://github.com/holepunchto/bare-tty>              |
| <https://github.com/holepunchto/bare-type>             |
| <https://github.com/holepunchto/bare-url>              |
| <https://github.com/holepunchto/bare-vm>               |
| <https://github.com/holepunchto/bare-worker>           |
| <https://github.com/holepunchto/bare-ws>               |
| <https://github.com/holepunchto/bare-zlib>             |

## License

Apache-2.0
