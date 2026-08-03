const url = require('bare-url')
const t = require('bare-tap')
const { Addon } = Bare

t.plan(1)

const addon = new Addon(
  url.pathToFileURL(`./test/fixtures/scoped-addon/prebuilds/${Addon.host}/bare__addon.bare`)
)

t.equal(addon.exports, 'Hello from scoped addon')
