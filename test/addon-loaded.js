const url = require('bare-url')
const t = require('bare-tap')
const { Addon } = Bare

t.plan(5)

const href = url.pathToFileURL(`./test/fixtures/addon/prebuilds/${Addon.host}/addon.bare`)

t.equal(Addon.loaded(href), false)

new Addon(href)

t.equal(Addon.loaded(href), true)

Addon.seal()

t.equal(Addon.loaded(href), true, 'still loaded after sealing')
t.equal(Addon.loaded(url.pathToFileURL('./test/fixtures/addon/missing.bare')), false)
t.equal(Addon.loaded(new URL('builtin:bare-buffer')), false)
