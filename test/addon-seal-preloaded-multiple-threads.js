const url = require('bare-url')
const t = require('bare-tap')
const { Addon, Thread } = Bare

t.plan(2)

// Load the addon before sealing so it is resident in the process-wide registry.
const addon = new Addon(
  url.pathToFileURL(`./test/fixtures/addon/prebuilds/${Addon.host}/addon.bare`)
)

t.equal(addon.exports, 'Hello from addon')

Addon.seal()

const thread = new Thread(__filename, () => {
  const url = require('bare-url')
  const tap = require('bare-tap')
  const { Addon } = Bare

  const t = tap.subtest()

  t.plan(1)

  // The addon is already resident in the process-wide registry, so loading it
  // on a freshly spawned thread succeeds via a cache hit even though addon
  // loading is sealed.
  const addon = new Addon(
    url.pathToFileURL(`./test/fixtures/addon/prebuilds/${Addon.host}/addon.bare`)
  )

  t.equal(addon.exports, 'Hello from addon')
})

thread.join()
t.pass()
