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

#### `process.arch`

#### `process.execPath`

#### `process.argv`

#### `process.pid`

#### `process.versions`

#### `process.title`

#### `process.exitCode`

#### `process.env`

#### `process.data`

#### `process.cwd()`

#### `process.chdir(directory)`

#### `process.exit([code])`

#### `process.suspend()`

#### `process.resume()`

#### `process.addon(specifier)`

#### `process.addon.resolve(specifier)`

#### `process.hrtime([previous])`

#### `process.hrtime.bigint()`

#### `process.nextTick(callback[, ...args])`

#### `process.on('uncaughtException', err)`

#### `process.on('unhandledRejection', reason, promise)`

#### `process.on('beforeExit')`

#### `process.on('exit', code)`

#### `process.on('suspend')`

#### `process.on('idle')`

#### `process.on('resume')`

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

When completed, the `pear` binary will be available in the `build/bin` directory and the `libpear.a` and `libpear.(dylib|dll)` libraries will be available in the root of the `build` directory.

### Linking

When linking against the static `libpear.a` library, make sure to use whole archive linking as :pear:.js relies on constructor functions for registering native addons. Without whole archive linking, the linker will remove the constructor functions as they aren't referenced by anything.

## License

Apache-2.0
