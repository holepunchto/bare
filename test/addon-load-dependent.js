const url = require('bare-url')
const t = require('bare-tap')
const { Addon } = Bare

t.plan(2)

const a = Addon.load(
  url.pathToFileURL(`./test/fixtures/dependent-addon/a/prebuilds/${Addon.host}/a.bare`)
)

const b = Addon.load(
  url.pathToFileURL(`./test/fixtures/dependent-addon/b/prebuilds/${Addon.host}/b.bare`)
)

t.ok(a)
t.equal(b.exports, 42)
