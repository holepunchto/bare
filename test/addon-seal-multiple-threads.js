const t = require('bare-tap')
const { Addon, Thread } = Bare

t.plan(2)

Addon.seal()

t.equal(Addon.sealed, true)

const thread = new Thread(__filename, () => {
  const url = require('bare-url')
  const t = require('bare-tap')
  const { Addon } = Bare

  t.plan(2)

  // The seal is process-global, so a freshly spawned thread must observe it and
  // refuse to load addons too.
  t.equal(Addon.sealed, true)

  try {
    Addon.load(url.pathToFileURL(`./test/fixtures/addon/prebuilds/${Addon.host}/addon.bare`))

    t.fail('addon load should throw on a sealed thread')
  } catch {
    t.pass('addon load throws on a sealed thread')
  }
})

thread.join()
t.pass()
