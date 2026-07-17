const url = require('bare-url')
const t = require('bare-tap')
const { Addon } = Bare

t.plan(2)

const a = new Addon(url.pathToFileURL(`./test/fixtures/addon/prebuilds/${Addon.host}/addon.bare`))

const b = new Addon(url.pathToFileURL(`./test/fixtures/addon/prebuilds/${Addon.host}/addon.bare`))

t.equal(a.exports, b.exports)
t.notEqual(a, b)
