const url = require('bare-url')
const t = require('./harness')
const { Addon } = Bare

t.plan(2)

const resolution = Addon.resolve('.', url.pathToFileURL(`./test/fixtures/scoped-addon/`))

const expected = url.pathToFileURL(
  `./test/fixtures/scoped-addon/prebuilds/${Addon.host}/bare__addon.bare`
)

t.equal(resolution.href, expected.href)

const addon = Addon.load(expected)

t.equal(addon.exports, 'Hello from scoped addon')
