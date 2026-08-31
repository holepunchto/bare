const url = require('bare-url')
const t = require('bare-tap')
const bundle = require('./helpers/bundle')
const { Addon, Thread } = Bare

t.plan(2)

// Load the addon before sealing so it is resident in the process-wide registry.
const addon = new Addon(
  url.pathToFileURL(`./test/fixtures/addon/prebuilds/${Addon.host}/addon.bare`)
)

t.equal(addon.exports, 'Hello from addon')

Addon.seal()

const thread = new Thread(
  'bare:/thread.bundle',
  bundle(__filename, (href) => {
    const { Addon } = Bare

    // The addon is already resident in the process-wide registry, so loading it
    // on a freshly spawned thread succeeds via a cache hit even though addon
    // loading is sealed.
    const addon = new Addon(new URL(href))

    if (addon.exports !== 'Hello from addon') {
      throw new Error('Addon was not loaded')
    }
  }),
  {
    data: addon.url.href
  }
)

thread.join()
t.pass()
