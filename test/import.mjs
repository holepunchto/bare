import test from 'brittle'
const { Addon } = Bare

import cjsMod from './fixtures/cjs'
import mjsMod from './fixtures/esm'

test('cjs', function (t) {
  t.is(cjsMod, 'Hello from CJS')
})

test('dynamic cjs', function (t) {
  t.plan(1)

  import('./fixtures/cjs').then(({ default: mod }) => t.is(mod, 'Hello from CJS'))
})

test('mjs', function (t) {
  t.is(mjsMod, 'Hello from ESM')
})

test('dynamic mjs', function (t) {
  t.plan(1)

  import('./fixtures/esm').then(({ default: mod }) => t.is(mod, 'Hello from ESM'))
})

test('addon', async function (t) {
  t.plan(1)

  const { default: mod } = await import(`./fixtures/addon/prebuilds/${Addon.host}/addon.bare`)

  t.is(mod, 'Hello from addon')
})
