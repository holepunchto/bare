<h1>
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="./media/logo-light.svg">
    <img alt="Bare" src="./media/logo-dark.svg" height="80">
  </picture>
</h1>

Small and modular JavaScript runtime for desktop and mobile. Like Node.js, it provides an asynchronous, event-driven architecture for writing applications in the lingua franca of modern software. Unlike Node.js, it makes embedding and cross-device support core use cases, aiming to run just as well on your phone as on your laptop. The result is a runtime ideal for networked, peer-to-peer applications that can run on a wide selection of hardware.

```sh
npm i -g bare
```

## Architecture

Bare is built on top of <https://github.com/holepunchto/libjs>, which provides low-level bindings to V8 in an engine independent manner, and <https://github.com/libuv/libuv>, which provides an asynchronous I/O event loop. Bare itself only adds a few missing pieces on top to support a wider ecosystem of modules:

1. A module system supporting both CJS and ESM with bidirectional interoperability between the two.
2. A native addon system supporting both statically and dynamically linked addons.
3. Light-weight threads with synchronous joins and `SharedArrayBuffer` support.

Everything else if left to userland modules to implement using these primitives, keeping the runtime itself succinct and _bare_. By abstracting over both the underlying JavaScript engine using `libjs` and platform I/O operations using `libuv`, Bare allows module authors to implement native addons that can run on any JavaScript engine that implements the `libjs` ABI and any system that `libuv` supports.

## Security

Bare is designed to be embedded alongside code the embedder may not fully trust, and `Bare.Addon.seal()` is the mechanism for freezing the set of native code a process may load. What that does and does not promise is written down in [`docs/threat-model.md`](docs/threat-model.md), which embedders should read before running untrusted JavaScript.

## API

The JavaScript API of Bare is available through the global `Bare` namespace and is documented in [`src/bare.d.ts`](src/bare.d.ts).

### Embedding

Bare can easily be embedded using the C API defined in [`include/bare.h`](include/bare.h):

```c
#include <bare.h>
#include <uv.h>

bare_t *bare;
bare_setup(uv_default_loop(), platform, &env /* Optional */, argc, argv, options, &bare);

bare_load(bare, filename, source, &module /* Optional */);

bare_run(bare, UV_RUN_DEFAULT);

int exit_code;
bare_teardown(bare, UV_RUN_DEFAULT, &exit_code);
```

If `source` is `NULL`, the contents of `filename` will instead be read at runtime. For examples of how to embed Bare on mobile platforms, see <https://github.com/holepunchto/bare-android> and <https://github.com/holepunchto/bare-ios>.

An embedder whose thread belongs to a host loop, such as the run loop of a user interface, drives the loop with `bare_poll()` rather than `bare_run()`. It runs the loop without blocking and reports how long the host may sleep before calling again:

```c
int timeout;
bare_poll(bare, &timeout);
```

A timeout of `-1` means that the host may sleep until the backend descriptor of the loop, as given by `uv_backend_fd()`, becomes readable. A host that sleeps on the timeout alone rather than on the descriptor will miss work that arrives from another thread.

### Attaching

`bare_run()` attaches the process to the thread while it runs, and so does loading an addon, so native code reached through either is already attached. A call the embedder makes itself is not, so attach the process around it:

```c
bare_t *previous;
bare_attach(bare, &previous);

js_call_function(env, receiver, fn, argc, argv, &result);

bare_detach(bare, previous);

bare_run(bare, UV_RUN_NOWAIT);
```

Attachments nest. `bare_attach()` hands back the process that was attached before, which `bare_detach()` attaches again, so detach on the same thread and in the reverse order of attaching. Detaching out of order returns `-1` and restores nothing.

Attaching does not run the loop. The call may leave work behind that only the loop will run, which is why it is run above. A process that has terminated or exited cannot be attached.

### Context

Some addons need a value that only the embedder has, such as a `JavaVM *` on Android. The embedder can pass such values to addons using `bare_context_set()`, with an optional callback that is called when the process is torn down:

```c
bare_context_set(bare, "bare.android.jvm.v1", jvm, NULL);
```

Addons can then look up the value using `bare_context_get()`:

```c
void *jvm;

if (bare_context_get("bare.android.jvm.v1", &jvm) == 0) {
  // Use the value.
}
```

Values are set per process, cannot be changed once set, and should all be set before the first call to `bare_load()`. Addons should handle a value not being set. Sealing the process also prevents setting any further values. See [`include/bare.h`](include/bare.h) for the details and [`docs/context-keys.md`](docs/context-keys.md) for the keys in use.

> [!NOTE]  
> Any value that is set is available to every addon in the process. See [`docs/threat-model.md`](docs/threat-model.md) before setting anything.

### Suspension

Bare provides a mechanism for implementing process suspension, which is needed for platforms with strict application lifecycle constraints, such as mobile platforms. When suspended, using either `bare_suspend()` from C or `Bare.suspend()` from JavaScript, a `suspend` event will be emitted on the `Bare` namespace. Then, when the loop has no work left and would otherwise exit, an `idle` event will be emitted and the loop blocked, keeping it from exiting. When the process is later resumed, using either `bare_resume()` from C or `Bare.resume()` from JavaScript, a `resume` event will be emitted and the loop unblocked, allowing it to exit when no work is left.

While suspended, the loop may also be woken up for limited periods of time to perform work, using either `bare_wakeup()` from C or `Bare.wakeup()` from JavaScript, which will emit a `wakeup` event. Each wakeup has an associated deadline after which the loop will be stopped and the process suspended again, emitting another `idle` event.

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

| Option              | Default                    | Description                                             |
| :------------------ | :------------------------- | :------------------------------------------------------ |
| `BARE_ENGINE`       | `github:holepunchto/libjs` | The JavaScript engine to use                            |
| `BARE_PREBUILDS`    | `ON`                       | Enable prebuilds for supported third-party dependencies |
| `BARE_MEMORY_LIMIT` | `0`                        | The default memory limit of each JavaScript heap        |

### Sanitizers

Bare can be compiled with a sanitizer by passing the `--sanitize` flag to the `bare-make generate` command:

```console
bare-make generate --sanitize address
```

This instruments Bare but not the engine. For that, build a sanitized V8 prebuild with <https://github.com/holepunchto/chromium-prebuilds> and add `--define BARE_PREBUILDS=OFF --define GN_DIR=<src> --define GN_OUT_DIR=<out>`.

## Platform support

Bare uses a tiered support system to manage expectations for the platforms that it targets. Targets may move between tiers between minor releases and as such a change in tier will not be considered a breaking change.

**Tier 1:** Platform targets for which prebuilds are provided as defined by the [`.github/workflows/prebuild.yml`](.github/workflows/prebuild.yml) workflow. Compilation and test failures for these targets will cause workflow runs to go red.

**Tier 2:** Platform targets for which Bare is known to work, but without automated compilation and testing. Regressions may occur between releases and will be considered bugs.

> [!NOTE]  
> Development happens primarily on Apple hardware with Linux and Windows systems running as virtual machines.

| Platform | Architecture | Version                              | Tier | Notes                                  |
| :------- | :----------- | :----------------------------------- | :--- | :------------------------------------- |
| Linux    | `arm64`      | >= Linux 5.15, >= GNU C Library 2.35 | 1    | Ubuntu 22.04, Debian 12, OpenWrt 23.05 |
| Linux    | `x64`        | >= Linux 5.15, >= GNU C Library 2.35 | 1    | Ubuntu 22.04, Debian 12, OpenWrt 23.05 |
| Linux    | `riscv64`    | >= Linux 6.8, >= GNU C Library 2.39  | 2    | Ubuntu 24.04, Debian 13                |
| Linux    | `arm`        | >= Linux 5.10, >= musl 1.2           | 2    | Alpine 3.13, OpenWrt 22.03             |
| Linux    | `arm64`      | >= Linux 5.10, >= musl 1.2           | 2    | Alpine 3.13, OpenWrt 22.03             |
| Linux    | `ia32`       | >= Linux 5.10, >= musl 1.2           | 2    | Alpine 3.13, OpenWrt 22.03             |
| Linux    | `x64`        | >= Linux 5.10, >= musl 1.2           | 2    | Alpine 3.13, OpenWrt 22.03             |
| Linux    | `riscv64`    | >= Linux 6.6, >= musl 1.2            | 2    | Alpine 3.20                            |
| Linux    | `mips`       | >= Linux 5.10, >= musl 1.2           | 2    | OpenWrt 22.03                          |
| Linux    | `mipsel`     | >= Linux 5.10, >= musl 1.2           | 2    | OpenWrt 22.03                          |
| Android  | `arm`        | >= 10                                | 1    |
| Android  | `arm64`      | >= 10                                | 1    |
| Android  | `ia32`       | >= 10                                | 1    |
| Android  | `x64`        | >= 10                                | 1    |
| macOS    | `arm64`      | >= 13.0                              | 1    |
| macOS    | `x64`        | >= 13.0                              | 1    |
| iOS      | `arm64`      | >= 15.0                              | 1    |
| iOS      | `x64`        | >= 15.0                              | 1    | Simulator only                         |
| Windows  | `arm64`      | >= Windows 11                        | 1    |
| Windows  | `x64`        | >= Windows 10                        | 1    |

## License

Apache-2.0
