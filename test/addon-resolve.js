const url = require('bare-url')
const path = require('bare-path')
const t = require('./harness')
const { Addon } = Bare

t.plan(1)

const resolved = Addon.resolve('.', url.pathToFileURL('./test/fixtures/addon/'))

t.equal(
  url.fileURLToPath(resolved),
  path.resolve('./test/fixtures/addon/prebuilds', Addon.host, 'addon.bare')
)
