# Bare

Small and modular JavaScript runtime for desktop and mobile. Like Node.js, it provides an asynchronous, event-driven architecture for writing applications in the lingua franca of modern software. Unlike Node.js, it makes embedding and cross-device support core use cases, aiming to run just as well on your phone as on your laptop. The result is a runtime ideal for networked, peer-to-peer applications that can run on a wide selection of hardware.

```sh
npm i -g bare
```

## Usage

```sh
bare [flags] <filename> [...args]

Arguments:
  <filename>
  [...args]

Flags:
  --version|-v
  --eval|-e <script>
  --print|-p <script>
  --help|-h   Show help
```

The specified `<script>` or `<filename>` is run using `Module.load()`. For more information on the module system and the supported formats, see <https://github.com/holepunchto/bare-module>.

When imported, the module exports the global `Bare` namespace. See <https://github.com/holepunchto/bare#api> for the API documentation.

```js
const Bare = require('bare')
```

## Credits

The `bare` package name on npm was kindly donated by the folks at [Accosine](https://github.com/accosine).

## License

Apache-2.0
