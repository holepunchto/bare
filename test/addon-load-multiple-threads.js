const url = require('bare-url')
const t = require('bare-tap')
const { Addon, Thread } = Bare

t.plan(2)

const addon = Addon.load(
  url.pathToFileURL(`./test/fixtures/addon/prebuilds/${Addon.host}/addon.bare`)
)

t.equal(addon.exports, 'Hello from addon')

const thread = new Thread(__filename, () => {
  const url = require('bare-url')
  const t = require('bare-tap')
  const { Addon } = Bare

  t.plan(1)

  const addon = Addon.load(
    url.pathToFileURL(`./test/fixtures/addon/prebuilds/${Addon.host}/addon.bare`)
  )

  t.equal(addon.exports, 'Hello from addon')
})

thread.join()
t.pass()
