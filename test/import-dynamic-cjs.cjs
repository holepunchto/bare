const assert = require('bare-assert')

import('./fixtures/cjs').then(({ default: mod }) =>
  assert(mod === 'Hello from CJS')
)
