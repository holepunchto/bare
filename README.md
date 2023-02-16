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

:pear:.js provides a mechanism for implementing process suspension, which is needed for platforms with strict application lifecycle constraints, such as mobile platforms. When suspended, a `suspend` event will be emitted on the `process` object and an idle handler started on the event loop, keeping it from exiting. When the process is later resumed, a `resume` event will be emitted and the idle handler stopped, allowing the loop to exit when no work is left.

The suspension API is available through `pear_suspend()` and `pear_resume()` from C and `process.suspend()` and `process.resume()` from JavaScript. See [`example/suspend.js`](example/suspend.js) for an example of using the suspension API from JavaScript.

## Building

To build :pear:.js, start by installing the dependencies:

```sh
$ npm install
```

Next, run the build script:

```sh
$ npm run build -- [--debug]
```

When completed, the `pear` binary will be available in the `build/bin` directory.

## License

MIT
