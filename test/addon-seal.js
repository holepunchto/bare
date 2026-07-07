const url = require('bare-url')
const t = require('bare-tap')
const { Addon } = Bare

t.plan(3)

t.equal(Addon.sealed, false)

Addon.seal()

t.equal(Addon.sealed, true)

try {
  Addon.load(url.pathToFileURL(`./test/fixtures/addon/prebuilds/${Addon.host}/addon.bare`))

  t.fail('addon load should throw after sealing')
} catch {
  t.pass('addon load throws after sealing')
}
