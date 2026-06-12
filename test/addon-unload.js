const url = require('bare-url')
const t = require('bare-tap')
const { Addon } = Bare

const fileURL = url.pathToFileURL(`./test/fixtures/addon/prebuilds/${Addon.host}/addon.bare`)

t.plan(8)

const addon = Addon.load(fileURL)
t.equal(addon.exports, 'Hello from addon')
t.ok(Addon.cache[fileURL.href] === addon, 'addon is cached after load')

const removed = Addon.unload(fileURL)
t.ok(removed, 'unload returns true when the native node was found and removed')
t.ok(Addon.cache[fileURL.href] === undefined, 'cache entry cleared after unload')

const reloaded = Addon.load(fileURL)
t.equal(reloaded.exports, 'Hello from addon')
t.ok(reloaded !== addon, 'reload creates a fresh addon instance')

const unknown = url.pathToFileURL('./test/fixtures/addon/prebuilds/none/addon.bare')
t.ok(Addon.unload(unknown) === false, 'unloading an unknown url returns false')

let threw = false
try {
  Addon.unload(new URL('builtin:foo'))
} catch {
  threw = true
}
t.ok(threw, 'unloading a builtin url throws')

Addon.unload(fileURL)
