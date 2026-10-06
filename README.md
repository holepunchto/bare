<h1>
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="./media/logo-light.svg">
    <img alt="Bare" src="./media/logo-dark.svg" height="80">
  </picture>
</h1>

Small and modular JavaScript runtime for desktop and mobile. Like Node.js, it provides an asynchronous, event-driven architecture for JavaScript applications. Unlike Node.js, it treats embedding and cross-device support as core use cases, aiming to run just as well on your phone as on your laptop.

```sh
npm i -g bare
```

## Usage

```sh
bare script.js
```

Run `bare` without a script to start a REPL. See the [CLI reference](https://docs.pears.com/bare/reference/bare/cli/) for all flags.

## API

The API reference is on <https://docs.pears.com/bare/>:

- [Runtime API](https://docs.pears.com/bare/reference/bare/runtime/): the global `Bare` namespace, including lifecycle and suspension, `Bare.Addon`, `Bare.Thread`, `Bare.IPC`, and the C embedding API
- [CLI](https://docs.pears.com/bare/reference/bare/cli/): running scripts, the REPL, and flags
- [Modules](https://docs.pears.com/bare/reference/modules/bare-modules/): the `bare-*` standard library
- [Bare Kit](https://docs.pears.com/bare/reference/bare/bare-kit/): embedding Bare in iOS, Android, and React Native apps
- [Embedder context](https://docs.pears.com/bare/reference/bare/embedder-context/): passing native handles from an embedder to addons

## Architecture

Bare is built on <https://github.com/holepunchto/libjs>, which provides low-level bindings to V8 in an engine-independent manner, and <https://github.com/libuv/libuv>, which provides an asynchronous I/O event loop. On top of these, Bare only adds:

1. A module system supporting both CJS and ESM with bidirectional interoperability between the two.
2. A native addon system supporting both statically and dynamically linked addons.
3. Lightweight threads with synchronous joins and `SharedArrayBuffer` support.

Everything else is left to userland [modules](https://docs.pears.com/bare/reference/modules/bare-modules/), keeping the runtime succinct and _bare_. Because the engine and platform I/O are abstracted by `libjs` and `libuv`, native addons can run on any JavaScript engine that implements the `libjs` ABI and any system that `libuv` supports. See [Inside Bare](https://docs.pears.com/bare/explanation/bare-runtime/) for more.

## Security

Bare is designed to be embedded alongside code the embedder may not fully trust. [`Bare.Addon.seal()`](https://docs.pears.com/bare/reference/bare/runtime/#bareaddon) freezes the set of native code a process may load, and [`docs/threat-model.md`](docs/threat-model.md) states what that does and does not promise. Embedders should read it before running untrusted JavaScript.

## Embedding

Bare is embedded using the C API in [`include/bare.h`](include/bare.h), documented in the [runtime API reference](https://docs.pears.com/bare/reference/bare/runtime/#embedding). For examples of how to embed Bare on mobile platforms, see <https://github.com/holepunchto/bare-android> and <https://github.com/holepunchto/bare-ios>.

## Building

<https://github.com/holepunchto/bare-make> is used for compiling Bare. Install it and the dependencies, then generate the build system and build:

```console
npm i -g bare-make
npm i
bare-make generate
bare-make build
```

Generating only has to be done once per repository checkout. After updating `bare-make` or your compiler toolchain, run `bare-make generate --no-cache` to disregard the existing build system cache.

When completed, the `bare(.exe)` binary is available in the `build/bin` directory and the `libbare.(a|lib)` and `(lib)bare.(dylib|dll|lib)` libraries are available in the root of the `build` directory.

### Linking

When linking against the static `libbare.(a|lib)` library, use whole archive linking. Bare relies on constructor functions for registering native addons, and without whole archive linking the linker removes them as unreferenced.

### Options

Compile options are set by passing `--define option=value` to `bare-make generate`.

> [!WARNING]  
> The compile options are not covered by semantic versioning and are subject to change without warning.

| Option              | Default                               | Description                                              |
| :------------------ | :------------------------------------ | :------------------------------------------------------- |
| `BARE_ENGINE`       | `github:holepunchto/libjs#<revision>` | The JavaScript engine to use, pinned in `CMakeLists.txt` |
| `BARE_PREBUILDS`    | `ON`                                  | Enable prebuilds for supported third-party dependencies  |
| `BARE_MEMORY_LIMIT` | `0`                                   | The default memory limit of each JavaScript heap         |

### Sanitizers

Bare can be compiled with a sanitizer by passing the `--sanitize` flag to `bare-make generate`:

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
