import assert from 'assert'

import('./fixtures/cjs').then(({ default: mod }) =>
  assert(mod === 'Hello from CJS')
)
