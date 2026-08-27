const url = require('bare-url')
const t = require('bare-tap')
const bundle = require('./helpers/bundle')
const { Addon, Thread } = Bare

t.plan(2)

Addon.seal()

t.equal(Addon.sealed, true)

const addonURL = url.pathToFileURL(`./test/fixtures/addon/prebuilds/${Addon.host}/addon.bare`).href

// A thread reaches no further than what it was handed, so the addon URL travels
// with it as data rather than being resolved on the thread.
const thread = new Thread(
  'bare:/thread.bundle',
  bundle(__filename, (href) => {
    const { Addon } = Bare

    // The seal is process-global, so a freshly spawned thread must observe it
    // and refuse to load addons too.
    if (Addon.sealed !== true) throw new Error('Thread did not observe the seal')

    try {
      new Addon(new URL(href))

      throw new Error('Addon load should throw on a sealed thread')
    } catch (err) {
      if (err.code !== 'CANNOT_LOAD') throw err
    }
  }),
  {
    data: addonURL
  }
)

thread.join()
t.pass()
