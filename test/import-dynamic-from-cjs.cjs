const assert = require('assert')

import('./fixtures/esm').then(({ default: mod }) =>
  assert(mod === 'Hello from ESM')
)
