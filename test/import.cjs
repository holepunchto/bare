const test = require('brittle')
const { Addon } = Bare

test('cjs', function (t) {
  t.plan(1)

  const mod = require('./fixtures/cjs')

  t.is(mod, 'Hello from CJS')
})

test('dynamic cjs', function (t) {
  t.plan(1)

  import('./fixtures/cjs').then(({ default: mod }) => t.is(mod, 'Hello from CJS'))
})

test('mjs', function (t) {
  t.plan(1)

  const { default: mod } = require('./fixtures/esm')

  t.is(mod, 'Hello from ESM')
})

test('dynamic mjs', function (t) {
  t.plan(1)

  import('./fixtures/esm').then(({ default: mod }) => t.is(mod, 'Hello from ESM'))
})

test('addon', function (t) {
  t.plan(1)

  const mod = require(`./fixtures/addon/prebuilds/${Addon.host}/addon.bare`)

  t.is(mod, 'Hello from addon')
})

test('bundle', function (t) {
  const mod = require('./fixtures/bundle/mod.bundle')

  t.is(mod, 'Hello from bundle')
})
