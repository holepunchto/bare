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

pear_run(&pear, filename, source, len);

int exit_code;
pear_teardown(&pear, &exit_code);
```

If `source` is `NULL`, the contents of `filename` will instead be read at runtime.

## Building

To build :pear:.js, start by installing the JavaScript dependencies:

```sh
$ npm install
```

Next, run the build script:

```sh
$ npm run build
```

When completed, the `pear` binary will be available in the `build/bin` directory.

## License

MIT
