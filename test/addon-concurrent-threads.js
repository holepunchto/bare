const url = require('bare-url')
const t = require('bare-tap')
const bundle = require('./helpers/bundle')
const { Addon, Thread } = Bare

// Load the same addons from several threads at once. The constructor addon in
// particular exercises recovering a registration from an addon that another
// thread published moments earlier.

t.plan(3)

const threads = []

for (let i = 0; i < 8; i++) {
  threads.push(
    new Thread(
      'bare:/thread.bundle',
      bundle(__filename, () => {
        const url = require('bare-url')
        const { Addon } = Bare

        const addon = new Addon(
          url.pathToFileURL(`./test/fixtures/addon/prebuilds/${Addon.host}/addon.bare`)
        )

        if (addon.exports !== 'Hello from addon') {
          throw new Error('Addon was not loaded')
        }

        const constructor = new Addon(
          url.pathToFileURL(
            `./test/fixtures/constructor-addon/prebuilds/${Addon.host}/constructor-addon.bare`
          )
        )

        if (constructor.exports !== 'Hello from constructor addon') {
          throw new Error('Constructor addon was not loaded')
        }
      })
    )
  )
}

const addon = new Addon(
  url.pathToFileURL(`./test/fixtures/addon/prebuilds/${Addon.host}/addon.bare`)
)

t.equal(addon.exports, 'Hello from addon')

const constructor = new Addon(
  url.pathToFileURL(
    `./test/fixtures/constructor-addon/prebuilds/${Addon.host}/constructor-addon.bare`
  )
)

t.equal(constructor.exports, 'Hello from constructor addon')

for (const thread of threads) thread.join()

t.pass()
