const assert = require('bare-assert')
const url = require('bare-url')
const { Addon, Thread } = Bare

const addon = Addon.load(
  url.pathToFileURL(`./test/fixtures/addon/prebuilds/${Addon.host}/addon.bare`)
)

assert(addon.exports === 'Hello from addon')

const thread = new Thread(__filename, () => {
  const assert = require('bare-assert')
  const url = require('bare-url')
  const { Addon, Thread } = Bare

  const addon = Addon.load(
    url.pathToFileURL(
      `./test/fixtures/addon/prebuilds/${Addon.host}/addon.bare`
    )
  )

  assert(addon.exports === 'Hello from addon')
})

thread.join()
