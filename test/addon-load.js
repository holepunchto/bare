const url = require('bare-url')
const t = require('./harness')
const { Addon } = Bare

t.plan(1)

const addon = Addon.load(
  url.pathToFileURL(`./test/fixtures/addon/prebuilds/${Addon.host}/addon.bare`)
)

t.equal(addon.exports, 'Hello from addon')
