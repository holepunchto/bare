const url = require('bare-url')
const t = require('bare-tap')
const { Addon, Thread } = Bare

t.plan(2)

const addon = new Addon(
  url.pathToFileURL(
    `./test/fixtures/constructor-addon/prebuilds/${Addon.host}/constructor-addon.bare`
  )
)

t.equal(addon.exports, 'Hello from constructor addon')

const thread = new Thread(__filename, () => {
  const url = require('bare-url')
  const t = require('bare-tap')
  const { Addon } = Bare

  t.plan(1)

  const addon = new Addon(
    url.pathToFileURL(
      `./test/fixtures/constructor-addon/prebuilds/${Addon.host}/constructor-addon.bare`
    )
  )

  t.equal(addon.exports, 'Hello from constructor addon')
})

thread.join()
t.pass()
