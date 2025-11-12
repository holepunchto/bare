const test = require('brittle')
const { Addon } = Bare

test('cjs', (t) => {
  const mod = require('./fixtures/cjs')

  t.is(mod, 'Hello from CJS')
})

test('dynamic cjs', async (t) => {
  const { default: mod } = await import('./fixtures/cjs')

  t.is(mod, 'Hello from CJS')
})

test('mjs', (t) => {
  const { default: mod } = require('./fixtures/esm')

  t.is(mod, 'Hello from ESM')
})

test('dynamic mjs', async (t) => {
  const { default: mod } = await import('./fixtures/esm')

  t.is(mod, 'Hello from ESM')
})

test('addon', (t) => {
  const mod = require(`./fixtures/addon/prebuilds/${Addon.host}/addon.bare`)

  t.is(mod, 'Hello from addon')
})

test('bundle', (t) => {
  const mod = require('./fixtures/bundle/mod.bundle')

  t.is(mod, 'Hello from bundle')
})
