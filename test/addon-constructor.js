const url = require('bare-url')
const t = require('bare-tap')
const { Addon } = Bare

t.plan(1)

const addon = new Addon(
  url.pathToFileURL(
    `./test/fixtures/constructor-addon/prebuilds/${Addon.host}/constructor-addon.bare`
  )
)

t.equal(addon.exports, 'Hello from constructor addon')
