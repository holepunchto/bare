# Bare

Small and modular JavaScript runtime for desktop and mobile. Like Node.js, it provides an asynchronous, event-driven architecture for JavaScript applications. Unlike Node.js, it treats embedding and cross-device support as core use cases, aiming to run just as well on your phone as on your laptop.

```sh
npm i -g bare
bare script.js
```

Run `bare` without a script to start a REPL. Scripts run as CommonJS or ESM; see [`bare-module`](https://docs.pears.com/bare/reference/bare/modules/bare-module/) for the supported formats and resolution rules.

## API

Importing the package returns the global `Bare` namespace. TypeScript types for it are included:

```js
const Bare = require('bare') // or: import Bare from 'bare'
```

The API is documented on <https://docs.pears.com/bare/>:

- [Runtime API](https://docs.pears.com/bare/reference/bare/runtime/): the `Bare` namespace
- [CLI](https://docs.pears.com/bare/reference/bare/cli/): flags and the REPL
- [Modules](https://docs.pears.com/bare/reference/modules/bare-modules/): the `bare-*` standard library

Source code, build instructions, and supported platforms are on [GitHub](https://github.com/holepunchto/bare).

## Credits

The `bare` package name on npm was kindly donated by the folks at [Accosine](https://github.com/accosine).

## License

Apache-2.0
