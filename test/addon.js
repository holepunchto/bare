const test = require('brittle')
const path = require('bare-path')
const url = require('bare-url')
const assert = require('bare-assert')
const { Addon, Thread } = Bare

test('load', function (t) {
  t.plan(1)

  const addon = Addon.load(
    url.pathToFileURL(`./test/fixtures/addon/prebuilds/${Addon.host}/addon.bare`)
  )

  t.is(addon.exports, 'Hello from addon')
})

test('resolve', function (t) {
  t.plan(1)

  const resolved = Addon.resolve('.', url.pathToFileURL('./test/fixtures/addon/'))

  t.is(
    url.fileURLToPath(resolved),
    path.resolve('./test/fixtures/addon/prebuilds', Addon.host, 'addon.bare')
  )
})

test('dependent', function (t) {
  t.plan(2)

  const a = Addon.load(
    url.pathToFileURL(`./test/fixtures/dependent-addon/a/prebuilds/${Addon.host}/a.bare`)
  )
  const b = Addon.load(
    url.pathToFileURL(`./test/fixtures/dependent-addon/b/prebuilds/${Addon.host}/b.bare`)
  )

  t.ok(a)
  t.is(b.exports, 42)
})

test('scoped', function (t) {
  t.plan(2)

  const resolution = Addon.resolve('.', url.pathToFileURL(`./test/fixtures/scoped-addon/`))

  const expected = url.pathToFileURL(
    `./test/fixtures/scoped-addon/prebuilds/${Addon.host}/bare__addon.bare`
  )

  t.is(resolution.href, expected.href)

  const addon = Addon.load(expected)

  t.is(addon.exports, 'Hello from scoped addon')
})

test('load multiple threads', function (t) {
  t.plan(2)

  const addon = Addon.load(
    url.pathToFileURL(`./test/fixtures/addon/prebuilds/${Addon.host}/addon.bare`)
  )

  t.is(addon.exports, 'Hello from addon')

  const thread = new Thread(__filename, () => {
    const assert = require('bare-assert')
    const url = require('bare-url')
    const { Addon } = Bare

    const addon = Addon.load(
      url.pathToFileURL(`./test/fixtures/addon/prebuilds/${Addon.host}/addon.bare`)
    )

    assert(addon.exports === 'Hello from addon')
  })

  thread.join()

  t.pass()
})
