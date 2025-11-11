import test from 'brittle'
const { Addon } = Bare

import cjs from './fixtures/cjs'
import mjs from './fixtures/esm'

test('cjs', (t) => {
  t.is(cjs, 'Hello from CJS')
})

test('dynamic cjs', async (t) => {
  t.plan(1)

  const { default: mod } = await import('./fixtures/cjs')

  t.is(mod, 'Hello from CJS')
})

test('mjs', (t) => {
  t.is(mjs, 'Hello from ESM')
})

test('dynamic mjs', async (t) => {
  const { default: mod } = await import('./fixtures/esm')

  t.is(mod, 'Hello from ESM')
})

test('addon', async (t) => {
  t.plan(1)

  const { default: mod } = await import(`./fixtures/addon/prebuilds/${Addon.host}/addon.bare`)

  t.is(mod, 'Hello from addon')
})
