import test from 'brittle'
const { Addon } = Bare

import cjsMod from './fixtures/cjs'
import mjsMod from './fixtures/esm'

test('cjs', (t) => {
  t.is(cjsMod, 'Hello from CJS')
})

test('dynamic cjs', (t) => {
  t.plan(1)

  import('./fixtures/cjs').then(({ default: mod }) => t.is(mod, 'Hello from CJS'))
})

test('mjs', (t) => {
  t.is(mjsMod, 'Hello from ESM')
})

test('dynamic mjs', (t) => {
  t.plan(1)

  import('./fixtures/esm').then(({ default: mod }) => t.is(mod, 'Hello from ESM'))
})

test('addon', async (t) => {
  t.plan(1)

  const { default: mod } = await import(`./fixtures/addon/prebuilds/${Addon.host}/addon.bare`)

  t.is(mod, 'Hello from addon')
})
