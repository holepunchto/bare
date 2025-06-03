const assert = require('bare-assert')
const url = require('bare-url')
const { Addon } = Bare

const resolution = Addon.resolve(
  '.',
  url.pathToFileURL(`./test/fixtures/scoped-addon/`)
)

const expected = url.pathToFileURL(
  `./test/fixtures/scoped-addon/prebuilds/${Addon.host}/bare__addon.bare`
)

assert(resolution.href === expected.href)

const addon = Addon.load(expected)

assert(addon.exports === 'Hello from scoped addon')
