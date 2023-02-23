# :pear:.js

Small and modular JavaScript runtime for desktop and mobile.

## Usage

```sh
$ pear <filename>
```

## API

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

:pear:.js provides a mechanism for implementing process suspension, which is needed for platforms with strict application lifecycle constraints, such as mobile platforms. When suspended, a `suspend` event will be emitted on the `process` object. Then, when the loop has no work left and would otherwise exit, an `idle` event will be emitted and an idle handler started, keeping the loop from exiting. When the process is later resumed, a `resume` event will be emitted and the idle handler stopped, allowing the loop to exit when no work is left.

The suspension API is available through `pear_suspend()` and `pear_resume()` from C and `process.suspend()` and `process.resume()` from JavaScript. See [`example/suspend.js`](example/suspend.js) for an example of using the suspension API from JavaScript.

### User data

:pear:.js provides a common API for simple user data exchange between the embedder, the JavaScript layer, and the native addon layer. The API provides a key/value store that associates string keys with arbitrary pointers, leaving it up to the embedder to decide on the meaning of the keys. Keys can be added and retrieved through `pear_set_data()` and `pear_get_data()` from C, and also retrived through `process.data()` from JavaScript which yields values of type `js_external`. 

While the JavaScript layer cannot directly access the underlying user data pointers, it _can_ forward these as `js_external` values to native addons.

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

MIT
