const assert = require('bare-assert')
const path = require('bare-path')
const url = require('bare-url')
const { Addon } = Bare

const resolved = Addon.resolve('.', url.pathToFileURL('./test/fixtures/addon/'))

assert(
  url.fileURLToPath(resolved) ===
    path.resolve('./test/fixtures/addon/prebuilds', Addon.host, 'addon.bare')
)
