import assert from 'assert'

import('./fixtures/esm').then(({ default: mod }) =>
  assert(mod === 'Hello from ESM')
)
